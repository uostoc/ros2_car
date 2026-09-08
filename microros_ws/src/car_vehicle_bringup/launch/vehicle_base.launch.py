"""Launch the Humble vehicle base hardware stack without navigation or mapping."""

import os

from ament_index_python.packages import get_package_share_directory
from launch import LaunchDescription
from launch.actions import DeclareLaunchArgument, IncludeLaunchDescription
from launch.launch_description_sources import PythonLaunchDescriptionSource
from launch.substitutions import LaunchConfiguration
from launch_ros.actions import Node


def generate_launch_description():
    micro_ros_device = LaunchConfiguration('micro_ros_device')
    micro_ros_baud = LaunchConfiguration('micro_ros_baud')
    lidar_device = LaunchConfiguration('lidar_device')
    lidar_baud = LaunchConfiguration('lidar_baud')
    lidar_frame_id = LaunchConfiguration('lidar_frame_id')
    use_sim_time = LaunchConfiguration('use_sim_time')

    description_share = get_package_share_directory('fishbot_description')
    urdf_path = os.path.join(description_share, 'urdf', 'fishbot_v1.0.0.urdf')
    lidar_launch = os.path.join(
        get_package_share_directory('sllidar_ros2'), 'launch', 'sllidar_c1_launch.py')
    common_parameters = [{'use_sim_time': use_sim_time}]

    return LaunchDescription([
        DeclareLaunchArgument('micro_ros_device', default_value='/dev/ttyACM0'),
        DeclareLaunchArgument('micro_ros_baud', default_value='921600'),
        DeclareLaunchArgument('lidar_device', default_value='/dev/ttyACM1'),
        DeclareLaunchArgument('lidar_baud', default_value='460800'),
        DeclareLaunchArgument('lidar_frame_id', default_value='laser'),
        DeclareLaunchArgument('use_sim_time', default_value='false'),
        Node(
            package='micro_ros_agent',
            executable='micro_ros_agent',
            name='micro_ros_agent',
            arguments=['serial', '-b', micro_ros_baud, '--dev', micro_ros_device, '-v4'],
            output='screen',
        ),
        IncludeLaunchDescription(
            PythonLaunchDescriptionSource([lidar_launch]),
            launch_arguments={
                'serial_port': lidar_device,
                'serial_baudrate': lidar_baud,
                'frame_id': lidar_frame_id,
            }.items(),
        ),
        Node(
            package='robot_state_publisher',
            executable='robot_state_publisher',
            name='robot_state_publisher',
            arguments=[urdf_path],
            parameters=common_parameters,
            output='screen',
        ),
        Node(
            package='joint_state_publisher',
            executable='joint_state_publisher',
            name='joint_state_publisher',
            arguments=[urdf_path],
            parameters=common_parameters,
            output='screen',
        ),
        Node(
            package='fishbot_bringup',
            executable='fishbot_bringup',
            name='fishbot_bringup',
            parameters=common_parameters,
            output='screen',
        ),
    ])
