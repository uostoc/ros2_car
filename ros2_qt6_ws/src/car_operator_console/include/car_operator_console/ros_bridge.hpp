#pragma once

#include <atomic>
#include <chrono>
#include <cstdint>
#include <map>
#include <memory>
#include <mutex>
#include <thread>
#include <vector>

#include <QObject>
#include <QString>

class QTimer;

#include <car_control_interfaces/action/run_patrol.hpp>
#include <car_control_interfaces/msg/component_status.hpp>
#include <car_control_interfaces/srv/manage_stack.hpp>
#include <geometry_msgs/msg/pose_with_covariance_stamped.hpp>
#include <geometry_msgs/msg/twist.hpp>
#include <nav2_msgs/action/navigate_to_pose.hpp>
#include <nav_msgs/msg/occupancy_grid.hpp>
#include <nav_msgs/msg/odometry.hpp>
#include <rclcpp/rclcpp.hpp>
#include <rclcpp_action/rclcpp_action.hpp>
#include <sensor_msgs/msg/imu.hpp>
#include <sensor_msgs/msg/laser_scan.hpp>
#include <tf2_msgs/msg/tf_message.hpp>

namespace car_operator_console {

class RosBridge final : public QObject {
  Q_OBJECT

 public:
  explicit RosBridge(QObject *parent = nullptr);
  ~RosBridge() override;

  void start();
  void stop();

 public slots:
  void start_stack(const QString &component, const QString &profile = {});
  void stop_stack(const QString &component);
  void set_initial_pose(double x, double y, double yaw_degrees);
  void send_navigation_goal(double x, double y, double yaw_degrees);
  void cancel_navigation();
  void start_patrol(const QString &route_name);
  void cancel_patrol();
  void publish_teleop_velocity(double linear_x, double angular_z);
  void stop_teleop();

 signals:
  void component_status(QString component, int state, int pid, QString detail);
  void topic_health(QString topic, bool active);
  void manager_reply(bool success, QString message);
  void navigation_status(QString message);
  void patrol_progress(unsigned int current, unsigned int total, QString point);
  void patrol_status(QString message);

 private:
  using ManageStack = car_control_interfaces::srv::ManageStack;
  using NavigateToPose = nav2_msgs::action::NavigateToPose;
  using RunPatrol = car_control_interfaces::action::RunPatrol;

  void request_stack(std::uint8_t command, const QString &component, const QString &profile = {});
  geometry_msgs::msg::PoseWithCovarianceStamped make_initial_pose(
      double x, double y, double yaw_degrees) const;
  void record_topic_activity(const char *topic);
  void monitor_topic_health();

  rclcpp::Node::SharedPtr node_;
  std::shared_ptr<rclcpp::executors::MultiThreadedExecutor> executor_;
  rclcpp::Client<ManageStack>::SharedPtr manager_client_;
  rclcpp_action::Client<NavigateToPose>::SharedPtr navigation_client_;
  rclcpp_action::Client<RunPatrol>::SharedPtr patrol_client_;
  rclcpp::Publisher<geometry_msgs::msg::PoseWithCovarianceStamped>::SharedPtr initial_pose_publisher_;
  rclcpp::Publisher<geometry_msgs::msg::Twist>::SharedPtr teleop_publisher_;
  std::vector<rclcpp::SubscriptionBase::SharedPtr> subscriptions_;
  QTimer *health_timer_{};
  std::thread spin_thread_;
  std::atomic_bool running_{false};
  std::mutex goal_mutex_;
  std::mutex health_mutex_;
  std::map<std::string, std::chrono::steady_clock::time_point> topic_activity_;
  std::map<std::string, bool> topic_active_;
  rclcpp_action::ClientGoalHandle<NavigateToPose>::SharedPtr navigation_goal_;
  rclcpp_action::ClientGoalHandle<RunPatrol>::SharedPtr patrol_goal_;
};

}  // namespace car_operator_console
