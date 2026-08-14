"""ROS 2 service that owns the physical-car process stack."""

from __future__ import annotations

import os
from typing import Sequence

from ament_index_python.packages import get_package_share_directory
from car_control_interfaces.msg import ComponentStatus
from car_control_interfaces.srv import ManageStack
from nav_msgs.msg import OccupancyGrid, Odometry
import rclpy
from rclpy.node import Node
from sensor_msgs.msg import Imu, LaserScan

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
    _BASE_COMPONENTS = ('agent', 'lidar', 'bringup')
    _MODE_COMPONENTS = ('navigation', 'mapping')
    _PATROL_COMPONENT = 'patrol'

    def __init__(self) -> None:
        super().__init__('car_stack_manager')
        self.declare_parameter('ros_domain_id', 0)
        self.declare_parameter('micro_ros_device', '/dev/ttyACM0')
        self.declare_parameter('micro_ros_baud', 921600)
        self.declare_parameter('lidar_device', '/dev/ttyACM1')
        self.declare_parameter('lidar_baud', 460800)
        self.declare_parameter('lidar_frame_id', 'laser')
        self.declare_parameter('map_file', '')
        self.declare_parameter('nav_params_file', '')
        self.declare_parameter('use_sim_time', False)

        self._status_publisher = self.create_publisher(
            ComponentStatus, '/car_manager/component_status', 20)
        self._service = self.create_service(
            ManageStack, '/car_manager/manage_stack', self._handle_manage_stack)
        self._registry = ProcessRegistry(self._publish_process_update)
        self._health_last_seen: dict[str, int] = {}
        self.create_subscription(LaserScan, '/scan', lambda _: self._seen('/scan'), 10)
        self.create_subscription(Odometry, '/odom', lambda _: self._seen('/odom'), 10)
        self.create_subscription(Imu, '/imu', lambda _: self._seen('/imu'), 10)
        self.create_subscription(OccupancyGrid, '/map', lambda _: self._seen('/map'), 10)
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

    def _path_or_default(self, parameter_name: str, package: str, relative_path: Sequence[str]) -> str:
        configured = str(self.get_parameter(parameter_name).value)
        if configured:
            return configured
        return os.path.join(get_package_share_directory(package), *relative_path)

    def _map_file(self, profile: str) -> str:
        if not profile:
            return self._path_or_default(
                'map_file', 'fishbot_navigation2', ('maps', 'bot202505_map.yaml'))
        filename = os.path.basename(profile)
        if filename != profile or not filename.endswith('.yaml'):
            raise ValueError('map profile must be a map filename ending in .yaml')
        candidate = os.path.join(get_package_share_directory('fishbot_navigation2'), 'maps', filename)
        if not os.path.isfile(candidate):
            raise ValueError(f'unknown map profile: {profile}')
        return candidate

    def _commands(self, map_profile: str = '') -> dict[str, list[str]]:
        device = str(self.get_parameter('micro_ros_device').value)
        micro_ros_baud = str(self.get_parameter('micro_ros_baud').value)
        lidar_device = str(self.get_parameter('lidar_device').value)
        lidar_baud = str(self.get_parameter('lidar_baud').value)
        lidar_frame_id = str(self.get_parameter('lidar_frame_id').value)
        use_sim_time = str(bool(self.get_parameter('use_sim_time').value)).lower()
        map_file = self._map_file(map_profile)
        nav_params = self._path_or_default(
            'nav_params_file', 'fishbot_navigation2', ('param', 'fishbot.yaml'))

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
            'navigation': [
                'ros2', 'launch', 'fishbot_navigation2', 'navigation2.launch.py',
                f'map:={map_file}', f'params_file:={nav_params}',
                f'use_sim_time:={use_sim_time}',
            ],
            'mapping': [
                'ros2', 'launch', 'fishbot_cartographer', 'cartographer.launch.py',
                f'use_sim_time:={use_sim_time}',
            ],
            'patrol': ['ros2', 'run', 'car_patrol', 'car_patrol'],
        }

    def _start_component(self, component: str, profile: str = '') -> tuple[bool, str]:
        try:
            command = self._commands(profile if component == 'navigation' else '').get(component)
        except ValueError as error:
            return False, str(error)
        if command is None:
            return False, f'unknown component: {component}'
        return self._registry.start(component, command, self._environment())

    def _start_base(self) -> tuple[bool, str]:
        results = [self._start_component(component) for component in self._BASE_COMPONENTS]
        failures = [message for success, message in results if not success and 'already running' not in message]
        return not failures, '; '.join(message for _, message in results)

    def _start_mode(self, mode: str, profile: str = '') -> tuple[bool, str]:
        base_success, base_message = self._start_base()
        if not base_success:
            return False, base_message
        other_mode = 'mapping' if mode == 'navigation' else 'navigation'
        if mode == 'mapping':
            self._registry.stop(self._PATROL_COMPONENT)
        self._registry.stop(other_mode)
        mode_success, mode_message = self._start_component(mode, profile)
        if mode != 'navigation' or not mode_success:
            return mode_success, f'{base_message}; {mode_message}'
        patrol_success, patrol_message = self._start_component(self._PATROL_COMPONENT)
        return patrol_success, f'{base_message}; {mode_message}; {patrol_message}'

    def _stop_all(self) -> tuple[bool, str]:
        messages = []
        for component in ('patrol', 'navigation', 'mapping', 'bringup', 'lidar', 'agent'):
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
            if component == 'base':
                response.success, response.message = self._start_base()
            elif component in self._MODE_COMPONENTS:
                response.success, response.message = self._start_mode(component, request.profile.strip())
            elif component == 'all':
                profile = request.profile.strip()
                mode = profile.lower() if profile.lower() in self._MODE_COMPONENTS else 'navigation'
                map_profile = '' if mode == profile.lower() else profile
                response.success, response.message = self._start_mode(mode, map_profile)
            elif component in self._BASE_COMPONENTS:
                response.success, response.message = self._start_component(component)
            else:
                response.success, response.message = False, f'unknown component: {component}'
            return response

        if request.command == ManageStack.Request.STOP:
            if component == 'all':
                response.success, response.message = self._stop_all()
            elif component == 'navigation':
                self._registry.stop(self._PATROL_COMPONENT)
                response.success, response.message = self._registry.stop(component)
            elif component in self._BASE_COMPONENTS + self._MODE_COMPONENTS + (self._PATROL_COMPONENT,):
                response.success, response.message = self._registry.stop(component)
            else:
                response.success, response.message = False, f'unknown component: {component}'
            return response

        response.success = False
        response.message = f'unsupported command: {request.command}'
        return response

    def _publish_process_update(self, component: str, state: str, pid: int, detail: str) -> None:
        self._publish_status(component, self._STATE_BY_NAME[state], pid, detail)

    def _publish_status(self, component: str, state: int, pid: int, detail: str) -> None:
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
        rclpy.shutdown()
