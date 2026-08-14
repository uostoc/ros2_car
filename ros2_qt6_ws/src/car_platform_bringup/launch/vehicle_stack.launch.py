"""Manual unified launch entry point for the physical car stack."""

import os

from ament_index_python.packages import get_package_share_directory
from launch import LaunchDescription
from launch.actions import DeclareLaunchArgument, IncludeLaunchDescription
from launch.conditions import IfCondition
from launch.launch_description_sources import PythonLaunchDescriptionSource
from launch.substitutions import LaunchConfiguration, PythonExpression
from launch_ros.actions import Node


def generate_launch_description():
    mode = LaunchConfiguration('mode')
    micro_ros_device = LaunchConfiguration('micro_ros_device')
    micro_ros_baud = LaunchConfiguration('micro_ros_baud')
    lidar_device = LaunchConfiguration('lidar_device')
    lidar_baud = LaunchConfiguration('lidar_baud')
    map_file = LaunchConfiguration('map_file')
    use_sim_time = LaunchConfiguration('use_sim_time')

    nav_share = get_package_share_directory('fishbot_navigation2')
    nav_launch = os.path.join(
        get_package_share_directory('nav2_bringup'), 'launch', 'bringup_launch.py')
    nav_params = os.path.join(nav_share, 'param', 'fishbot.yaml')
    default_map = os.path.join(nav_share, 'maps', 'bot202505_map.yaml')
    cartographer_launch = os.path.join(
        get_package_share_directory('fishbot_cartographer'),
        'launch',
        'cartographer.launch.py',
    )

    is_navigation = IfCondition(PythonExpression(["'", mode, "' == 'navigation'"]))
    is_mapping = IfCondition(PythonExpression(["'", mode, "' == 'mapping'"]))

    return LaunchDescription([
        DeclareLaunchArgument('mode', default_value='base', description='base, navigation, or mapping'),
        DeclareLaunchArgument('micro_ros_device', default_value='/dev/ttyACM0'),
        DeclareLaunchArgument('micro_ros_baud', default_value='921600'),
        DeclareLaunchArgument('lidar_device', default_value='/dev/ttyACM1'),
        DeclareLaunchArgument('lidar_baud', default_value='460800'),
        DeclareLaunchArgument('map_file', default_value=default_map),
        DeclareLaunchArgument('use_sim_time', default_value='false'),
        Node(
            package='micro_ros_agent',
            executable='micro_ros_agent',
            name='micro_ros_agent',
            arguments=['serial', '-b', micro_ros_baud, '--dev', micro_ros_device, '-v4'],
            output='screen',
        ),
        IncludeLaunchDescription(
            PythonLaunchDescriptionSource([
                os.path.join(get_package_share_directory('sllidar_ros2'), 'launch', 'sllidar_c1_launch.py')
            ]),
            launch_arguments={
                'serial_port': lidar_device,
                'serial_baudrate': lidar_baud,
            }.items(),
        ),
        IncludeLaunchDescription(
            PythonLaunchDescriptionSource([
                os.path.join(get_package_share_directory('fishbot_bringup'), 'launch', 'fishbot_bringup.launch.py')
            ]),
        ),
        IncludeLaunchDescription(
            PythonLaunchDescriptionSource([nav_launch]),
            condition=is_navigation,
            launch_arguments={
                'map': map_file,
                'params_file': nav_params,
                'use_sim_time': use_sim_time,
            }.items(),
        ),
        Node(
            package='car_patrol',
            executable='car_patrol',
            name='car_patrol',
            condition=is_navigation,
            output='screen',
        ),
        IncludeLaunchDescription(
            PythonLaunchDescriptionSource([cartographer_launch]),
            condition=is_mapping,
            launch_arguments={'use_sim_time': use_sim_time}.items(),
        ),
    ])
