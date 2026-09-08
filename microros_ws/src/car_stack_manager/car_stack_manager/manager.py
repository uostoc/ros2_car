"""ROS 2 service that owns the physical-car process stack."""

from __future__ import annotations

import os

from car_control_interfaces.msg import ComponentStatus
from car_control_interfaces.srv import ManageStack
from nav_msgs.msg import Odometry
import rclpy
from rclpy.node import Node
from sensor_msgs.msg import Imu, LaserScan

from .component_policy import BASE_COMPONENTS, UNSUPPORTED_COMPONENTS, unsupported_message
from .process_registry import ProcessRegistry


class CarStackManager(Node):
    """Starts mutually exclusive mapping/navigation modes on the vehicle."""

    _STATE_BY_NAME = {
        'stopped': ComponentStatus.STOPPED,
        'starting': ComponentStatus.STARTING,
        'running': ComponentStatus.RUNNING,
        'stopping': ComponentStatus.STOPPING,
        'error': ComponentStatus.ERROR,
    }
    _BASE_COMPONENTS = BASE_COMPONENTS
    _UNSUPPORTED_COMPONENTS = UNSUPPORTED_COMPONENTS

    def __init__(self) -> None:
        super().__init__('car_stack_manager')
        self.declare_parameter('ros_domain_id', 0)
        self.declare_parameter('micro_ros_device', '/dev/ttyACM0')
        self.declare_parameter('micro_ros_baud', 921600)
        self.declare_parameter('lidar_device', '/dev/ttyACM1')
        self.declare_parameter('lidar_baud', 460800)
        self.declare_parameter('lidar_frame_id', 'laser')

        self._status_publisher = self.create_publisher(
            ComponentStatus, '/car_manager/component_status', 20)
        self._service = self.create_service(
            ManageStack, '/car_manager/manage_stack', self._handle_manage_stack)
        self._registry = ProcessRegistry(self._publish_process_update)
        self._health_last_seen: dict[str, int] = {}
        self.create_subscription(LaserScan, '/scan', lambda _: self._seen('/scan'), 10)
        self.create_subscription(Odometry, '/odom', lambda _: self._seen('/odom'), 10)
        self.create_subscription(Imu, '/imu', lambda _: self._seen('/imu'), 10)
        self.create_timer(1.0, self._on_timer)

    def _seen(self, component: str) -> None:
        now = self.get_clock().now().nanoseconds
        if component not in self._health_last_seen:
            self._publish_status(component, ComponentStatus.RUNNING, 0, 'topic is active')
        self._health_last_seen[component] = now

    def _on_timer(self) -> None:
        self._registry.poll()
        now = self.get_clock().now().nanoseconds
        for component, seen_at in list(self._health_last_seen.items()):
            if now - seen_at > 3_000_000_000:
                self._publish_status(component, ComponentStatus.ERROR, 0, 'no recent messages')
                self._health_last_seen.pop(component, None)

    def _environment(self) -> dict[str, str]:
        environment = dict(os.environ)
        environment['ROS_DOMAIN_ID'] = str(self.get_parameter('ros_domain_id').value)
        return environment

    def _commands(self) -> dict[str, list[str]]:
        device = str(self.get_parameter('micro_ros_device').value)
        micro_ros_baud = str(self.get_parameter('micro_ros_baud').value)
        lidar_device = str(self.get_parameter('lidar_device').value)
        lidar_baud = str(self.get_parameter('lidar_baud').value)
        lidar_frame_id = str(self.get_parameter('lidar_frame_id').value)
        return {
            'agent': [
                'ros2', 'run', 'micro_ros_agent', 'micro_ros_agent', 'serial', '-b',
                micro_ros_baud, '--dev', device, '-v4',
            ],
            'lidar': [
                'ros2', 'launch', 'sllidar_ros2', 'sllidar_c1_launch.py',
                f'serial_port:={lidar_device}', f'serial_baudrate:={lidar_baud}',
                f'frame_id:={lidar_frame_id}',
            ],
            'bringup': [
                'ros2', 'launch', 'fishbot_bringup', 'fishbot_bringup.launch.py',
            ],
        }

    def _start_component(self, component: str) -> tuple[bool, str]:
        command = self._commands().get(component)
        if command is None:
            return False, self._unsupported_message(component)
        return self._registry.start(component, command, self._environment())

    def _start_base(self) -> tuple[bool, str]:
        results = [self._start_component(component) for component in self._BASE_COMPONENTS]
        failures = [message for success, message in results if not success and 'already running' not in message]
        return not failures, '; '.join(message for _, message in results)

    def _stop_all(self) -> tuple[bool, str]:
        messages = []
        for component in ('bringup', 'lidar', 'agent'):
            _, message = self._registry.stop(component)
            messages.append(message)
        return True, '; '.join(messages)

    def _handle_manage_stack(self, request: ManageStack.Request, response: ManageStack.Response) -> ManageStack.Response:
        component = request.component.strip().lower()
        if request.command == ManageStack.Request.QUERY:
            response.success = True
            response.message = 'status is published on /car_manager/component_status'
            return response

        if request.command == ManageStack.Request.START:
            if component in ('base', 'all'):
                response.success, response.message = self._start_base()
            elif component in self._BASE_COMPONENTS:
                response.success, response.message = self._start_component(component)
            else:
                response.success, response.message = False, self._unsupported_message(component)
            return response

        if request.command == ManageStack.Request.STOP:
            if component == 'all':
                response.success, response.message = self._stop_all()
            elif component in self._BASE_COMPONENTS:
                response.success, response.message = self._registry.stop(component)
            else:
                response.success, response.message = False, self._unsupported_message(component)
            return response

        response.success = False
        response.message = f'unsupported command: {request.command}'
        return response

    def _unsupported_message(self, component: str) -> str:
        return unsupported_message(component)

    def _publish_process_update(self, component: str, state: str, pid: int, detail: str) -> None:
        self._publish_status(component, self._STATE_BY_NAME[state], pid, detail)

    def _publish_status(self, component: str, state: int, pid: int, detail: str) -> None:
        # Ctrl+C can invalidate the global ROS context before final cleanup.
        # Continue stopping child processes, but do not publish on an invalid context.
        if not rclpy.ok():
            return
        message = ComponentStatus()
        message.component = component
        message.state = state
        message.pid = pid
        message.detail = detail
        message.stamp = self.get_clock().now().to_msg()
        self._status_publisher.publish(message)

    def shutdown(self) -> None:
        self._stop_all()


def main(args=None) -> None:
    rclpy.init(args=args)
    manager = CarStackManager()
    try:
        rclpy.spin(manager)
    except KeyboardInterrupt:
        pass
    finally:
        manager.shutdown()
        manager.destroy_node()
        if rclpy.ok():
            rclpy.shutdown()
