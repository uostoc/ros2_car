"""Action server that sends a route's goals to Nav2 and returns to the start."""

from __future__ import annotations

import math
import time

from action_msgs.msg import GoalStatus
from ament_index_python.packages import get_package_share_directory
from car_control_interfaces.action import RunPatrol
from geometry_msgs.msg import PoseStamped, PoseWithCovarianceStamped
from nav2_msgs.action import NavigateToPose
import rclpy
from rclpy.action import ActionClient, ActionServer, CancelResponse, GoalResponse
from rclpy.callback_groups import ReentrantCallbackGroup
from rclpy.executors import MultiThreadedExecutor
from rclpy.node import Node

from .route_loader import RoutePoint, load_routes


class PatrolServer(Node):
    def __init__(self) -> None:
        super().__init__('car_patrol')
        default_routes = get_package_share_directory('car_patrol') + '/config/patrol_routes.yaml'
        self.declare_parameter('routes_file', default_routes)
        self.declare_parameter('navigation_action', 'navigate_to_pose')
        self.declare_parameter('map_frame', 'map')
        self._group = ReentrantCallbackGroup()
        self._routes = load_routes(self.get_parameter('routes_file').value)
        self._initial_pose_publisher = self.create_publisher(PoseWithCovarianceStamped, '/initialpose', 10)
        self._nav_client = ActionClient(
            self,
            NavigateToPose,
            self.get_parameter('navigation_action').value,
            callback_group=self._group,
        )
        self._server = ActionServer(
            self,
            RunPatrol,
            '/car_patrol/run',
            execute_callback=self._execute,
            goal_callback=self._accept_goal,
            cancel_callback=self._accept_cancel,
            callback_group=self._group,
        )

    def _accept_goal(self, request: RunPatrol.Goal) -> GoalResponse:
        if request.route_name in self._routes:
            return GoalResponse.ACCEPT
        self.get_logger().warning(f'Unknown patrol route: {request.route_name}')
        return GoalResponse.REJECT

    @staticmethod
    def _accept_cancel(_: object) -> CancelResponse:
        return CancelResponse.ACCEPT

    def _pose(self, point: RoutePoint) -> PoseStamped:
        pose = PoseStamped()
        pose.header.frame_id = self.get_parameter('map_frame').value
        pose.header.stamp = self.get_clock().now().to_msg()
        pose.pose.position.x = point.x
        pose.pose.position.y = point.y
        yaw_radians = math.radians(point.yaw_degrees)
        pose.pose.orientation.z = math.sin(yaw_radians / 2.0)
        pose.pose.orientation.w = math.cos(yaw_radians / 2.0)
        return pose

    def _publish_initial_pose(self, point: RoutePoint) -> None:
        pose = PoseWithCovarianceStamped()
        pose.header.frame_id = self.get_parameter('map_frame').value
        pose.header.stamp = self.get_clock().now().to_msg()
        pose.pose.pose = self._pose(point).pose
        pose.pose.covariance[0] = 0.25
        pose.pose.covariance[7] = 0.25
        pose.pose.covariance[35] = 0.0685
        self._initial_pose_publisher.publish(pose)

    def _execute(self, goal_handle: object) -> RunPatrol.Result:
        route = self._routes[goal_handle.request.route_name]
        result = RunPatrol.Result()
        if not self._nav_client.wait_for_server(timeout_sec=5.0):
            result.completed = False
            result.message = 'Nav2 NavigateToPose action is unavailable'
            goal_handle.abort()
            return result

        self._publish_initial_pose(route.start)
        time.sleep(0.5)
        all_points = list(route.points) + [route.start]
        for index, point in enumerate(all_points):
            if goal_handle.is_cancel_requested:
                result.completed = False
                result.message = 'patrol cancelled'
                goal_handle.canceled()
                return result

            goal = NavigateToPose.Goal()
            goal.pose = self._pose(point)
            goal_future = self._nav_client.send_goal_async(goal)
            while not goal_future.done():
                if goal_handle.is_cancel_requested:
                    result.completed = False
                    result.message = 'patrol cancelled before goal was accepted'
                    goal_handle.canceled()
                    return result
                time.sleep(0.05)
            nav_goal = goal_future.result()
            if not nav_goal.accepted:
                result.completed = False
                result.message = f'Nav2 rejected point {index + 1}'
                goal_handle.abort()
                return result

            feedback = RunPatrol.Feedback()
            feedback.current_index = index + 1
            feedback.total_points = len(all_points)
            feedback.current_point = f'{point.x:.2f}, {point.y:.2f}, {point.yaw_degrees:.1f} deg'
            goal_handle.publish_feedback(feedback)

            result_future = nav_goal.get_result_async()
            while not result_future.done():
                if goal_handle.is_cancel_requested:
                    nav_goal.cancel_goal_async()
                    result.completed = False
                    result.message = 'patrol cancelled'
                    goal_handle.canceled()
                    return result
                time.sleep(0.1)
            nav_result = result_future.result()
            if nav_result.status != GoalStatus.STATUS_SUCCEEDED:
                result.completed = False
                result.message = f'Nav2 failed at point {index + 1} with status {nav_result.status}'
                goal_handle.abort()
                return result

        result.completed = True
        result.message = 'patrol completed and returned to start'
        goal_handle.succeed()
        return result

    def destroy_node(self) -> bool:
        self._server.destroy()
        return super().destroy_node()


def main(args=None) -> None:
    rclpy.init(args=args)
    node = PatrolServer()
    executor = MultiThreadedExecutor(num_threads=2)
    executor.add_node(node)
    try:
        executor.spin()
    except KeyboardInterrupt:
        pass
    finally:
        executor.shutdown()
        node.destroy_node()
        rclpy.shutdown()
