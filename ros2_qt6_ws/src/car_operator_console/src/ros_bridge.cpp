#include "car_operator_console/ros_bridge.hpp"

#include <cmath>
#include <utility>

#include <QMetaObject>

namespace car_operator_console {

RosBridge::RosBridge(QObject *parent) : QObject(parent) {}

RosBridge::~RosBridge() { stop(); }

void RosBridge::start() {
  if (running_.exchange(true)) {
    return;
  }

  node_ = std::make_shared<rclcpp::Node>("car_operator_console");
  manager_client_ = node_->create_client<ManageStack>("/car_manager/manage_stack");
  navigation_client_ = rclcpp_action::create_client<NavigateToPose>(node_, "navigate_to_pose");
  patrol_client_ = rclcpp_action::create_client<RunPatrol>(node_, "/car_patrol/run");
  initial_pose_publisher_ = node_->create_publisher<geometry_msgs::msg::PoseWithCovarianceStamped>(
      "/initialpose", 10);

  subscriptions_.push_back(node_->create_subscription<car_control_interfaces::msg::ComponentStatus>(
      "/car_manager/component_status", 20,
      [this](const car_control_interfaces::msg::ComponentStatus::SharedPtr message) {
        emit component_status(QString::fromStdString(message->component), message->state, message->pid,
                              QString::fromStdString(message->detail));
      }));
  subscriptions_.push_back(node_->create_subscription<sensor_msgs::msg::LaserScan>(
      "/scan", 10, [this](const sensor_msgs::msg::LaserScan::SharedPtr) { emit topic_health("/scan", true); }));
  subscriptions_.push_back(node_->create_subscription<nav_msgs::msg::Odometry>(
      "/odom", 10, [this](const nav_msgs::msg::Odometry::SharedPtr) { emit topic_health("/odom", true); }));
  subscriptions_.push_back(node_->create_subscription<sensor_msgs::msg::Imu>(
      "/imu", 10, [this](const sensor_msgs::msg::Imu::SharedPtr) { emit topic_health("/imu", true); }));
  subscriptions_.push_back(node_->create_subscription<nav_msgs::msg::OccupancyGrid>(
      "/map", 10, [this](const nav_msgs::msg::OccupancyGrid::SharedPtr) { emit topic_health("/map", true); }));

  executor_ = std::make_shared<rclcpp::executors::MultiThreadedExecutor>();
  executor_->add_node(node_);
  spin_thread_ = std::thread([this]() { executor_->spin(); });
  emit navigation_status(tr("ROS 2 bridge started"));
}

void RosBridge::stop() {
  if (!running_.exchange(false)) {
    return;
  }
  if (executor_) {
    executor_->cancel();
  }
  if (spin_thread_.joinable()) {
    spin_thread_.join();
  }
  if (executor_ && node_) {
    executor_->remove_node(node_);
  }
  patrol_client_.reset();
  navigation_client_.reset();
  manager_client_.reset();
  initial_pose_publisher_.reset();
  subscriptions_.clear();
  executor_.reset();
  node_.reset();
}

void RosBridge::request_stack(
    std::uint8_t command, const QString &component, const QString &profile) {
  if (!manager_client_ || !manager_client_->service_is_ready()) {
    emit manager_reply(false, tr("Vehicle stack manager is unavailable"));
    return;
  }
  auto request = std::make_shared<ManageStack::Request>();
  request->command = command;
  request->component = component.toStdString();
  request->profile = profile.toStdString();
  manager_client_->async_send_request(
      request, [this](rclcpp::Client<ManageStack>::SharedFuture future) {
        const auto response = future.get();
        emit manager_reply(response->success, QString::fromStdString(response->message));
      });
}

void RosBridge::start_stack(const QString &component, const QString &profile) {
  request_stack(ManageStack::Request::START, component, profile);
}

void RosBridge::stop_stack(const QString &component) {
  request_stack(ManageStack::Request::STOP, component);
}

geometry_msgs::msg::PoseWithCovarianceStamped RosBridge::make_initial_pose(
    double x, double y, double yaw_degrees) const {
  geometry_msgs::msg::PoseWithCovarianceStamped pose;
  pose.header.frame_id = "map";
  pose.header.stamp = static_cast<builtin_interfaces::msg::Time>(node_->get_clock()->now());
  pose.pose.pose.position.x = x;
  pose.pose.pose.position.y = y;
  const double yaw = yaw_degrees * std::acos(-1.0) / 180.0;
  pose.pose.pose.orientation.z = std::sin(yaw / 2.0);
  pose.pose.pose.orientation.w = std::cos(yaw / 2.0);
  pose.pose.covariance[0] = 0.25;
  pose.pose.covariance[7] = 0.25;
  pose.pose.covariance[35] = 0.0685;
  return pose;
}

void RosBridge::set_initial_pose(double x, double y, double yaw_degrees) {
  if (!initial_pose_publisher_) {
    emit navigation_status(tr("ROS 2 bridge is not running"));
    return;
  }
  initial_pose_publisher_->publish(make_initial_pose(x, y, yaw_degrees));
  emit navigation_status(tr("Initial pose published"));
}

void RosBridge::send_navigation_goal(double x, double y, double yaw_degrees) {
  if (!navigation_client_ || !navigation_client_->action_server_is_ready()) {
    emit navigation_status(tr("Nav2 NavigateToPose action is unavailable"));
    return;
  }
  NavigateToPose::Goal goal;
  const auto initial_pose = make_initial_pose(x, y, yaw_degrees);
  goal.pose.header = initial_pose.header;
  goal.pose.pose = initial_pose.pose.pose;
  rclcpp_action::Client<NavigateToPose>::SendGoalOptions options;
  options.goal_response_callback = [this](const auto &goal_handle) {
    std::lock_guard<std::mutex> lock(goal_mutex_);
    navigation_goal_ = goal_handle;
    emit navigation_status(goal_handle ? tr("Navigation goal accepted") : tr("Navigation goal rejected"));
  };
  options.result_callback = [this](const auto &result) {
    std::lock_guard<std::mutex> lock(goal_mutex_);
    navigation_goal_.reset();
    emit navigation_status(tr("Navigation finished with result %1").arg(static_cast<int>(result.code)));
  };
  navigation_client_->async_send_goal(goal, options);
  emit navigation_status(tr("Navigation goal sent"));
}

void RosBridge::cancel_navigation() {
  std::lock_guard<std::mutex> lock(goal_mutex_);
  if (!navigation_client_ || !navigation_goal_) {
    emit navigation_status(tr("No active navigation goal"));
    return;
  }
  navigation_client_->async_cancel_goal(navigation_goal_);
  emit navigation_status(tr("Navigation cancel requested"));
}

void RosBridge::start_patrol(const QString &route_name) {
  if (!patrol_client_ || !patrol_client_->action_server_is_ready()) {
    emit patrol_status(tr("Patrol action server is unavailable"));
    return;
  }
  RunPatrol::Goal goal;
  goal.route_name = route_name.toStdString();
  rclcpp_action::Client<RunPatrol>::SendGoalOptions options;
  options.goal_response_callback = [this](const auto &goal_handle) {
    std::lock_guard<std::mutex> lock(goal_mutex_);
    patrol_goal_ = goal_handle;
    emit patrol_status(goal_handle ? tr("Patrol accepted") : tr("Patrol rejected"));
  };
  options.feedback_callback = [this](auto, const auto feedback) {
    emit patrol_progress(feedback->current_index, feedback->total_points,
                         QString::fromStdString(feedback->current_point));
  };
  options.result_callback = [this](const auto &result) {
    std::lock_guard<std::mutex> lock(goal_mutex_);
    patrol_goal_.reset();
    emit patrol_status(QString::fromStdString(result.result->message));
  };
  patrol_client_->async_send_goal(goal, options);
}

void RosBridge::cancel_patrol() {
  std::lock_guard<std::mutex> lock(goal_mutex_);
  if (!patrol_client_ || !patrol_goal_) {
    emit patrol_status(tr("No active patrol"));
    return;
  }
  patrol_client_->async_cancel_goal(patrol_goal_);
  emit patrol_status(tr("Patrol cancel requested"));
}

}  // namespace car_operator_console
