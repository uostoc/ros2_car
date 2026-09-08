# ROS 2 Humble vehicle base workspace

[English](README.md) | [简体中文](README.zh-CN.md)

This workspace is the Ubuntu 22.04 / ROS 2 Humble source tree for the physical
vehicle's base hardware only. It contains the micro-ROS Agent, C1 lidar driver,
Fishbot URDF/TF bringup, vehicle process manager, and the DDS interfaces shared
with the Jazzy operator console. It intentionally excludes Qt, RViz, Nav2,
Cartographer, mapping, navigation, patrol, and exploration packages.

## Install dependencies

On the vehicle computer, install ROS 2 Humble and the base-stack dependencies:

```bash
sudo apt update
sudo apt install ros-humble-ros-base ros-humble-robot-state-publisher \
  ros-humble-joint-state-publisher ros-humble-rmw-fastrtps-cpp \
  python3-colcon-common-extensions python3-rosdep build-essential cmake git
sudo rosdep init  # only if rosdep has not been initialized on this computer
rosdep update
```

The first build needs Internet access because `micro_ros_agent` downloads its
pinned Micro-XRCE-DDS-Agent dependency (`v2.4.2`).

## Build

```bash
source /opt/ros/humble/setup.bash
cd /opt/ros2-car/microros_ws
rosdep install --from-paths src --ignore-src -r -y --skip-keys ament_python
colcon build --symlink-install
source install/setup.bash
```

To start the base stack manually, use the same device paths as the manager:

```bash
ros2 launch car_vehicle_bringup vehicle_base.launch.py \
  micro_ros_device:=/dev/ttyACM0 micro_ros_baud:=921600 \
  lidar_device:=/dev/ttyACM1 lidar_baud:=460800 lidar_frame_id:=laser
```

For normal use, start only the manager; it owns the child processes:

```bash
cp config/vehicle.example.yaml /tmp/vehicle.yaml
ros2 run car_stack_manager car_stack_manager --ros-args --params-file /tmp/vehicle.yaml
```

Then use `/car_manager/manage_stack` with `base`, `agent`, `lidar`, `bringup`,
or `all`. `navigation`, `mapping`, and `patrol` intentionally return an error.

## Service installation

Copy `config/vehicle.example.yaml` to `/etc/ros2-car/vehicle.yaml`, adjust
device paths, and install `deploy/systemd/car-stack-manager-humble.service` as
`/etc/systemd/system/car-stack-manager.service`. Set its workspace path if the
workspace is not `/opt/ros2-car/microros_ws`. Copy
`deploy/systemd/vehicle.env.example` to `/etc/ros2-car/vehicle.env` and keep
its domain id equal to `ros_domain_id` in `vehicle.yaml`, then run:

```bash
sudo systemctl daemon-reload
sudo systemctl enable --now car-stack-manager
```

The service account must be in the `dialout` group. Prefer stable
`/dev/serial/by-id/...` paths over `/dev/ttyACM*` numbers.

## Jazzy operator-console interoperation

Both computers must use the same `ROS_DOMAIN_ID` and `RMW_IMPLEMENTATION`
(recommended: `rmw_fastrtps_cpp`). Build the identical
`car_control_interfaces` IDL on both sides. Verify the connection from the
Jazzy operator computer:

```bash
ros2 service call /car_manager/manage_stack \
  car_control_interfaces/srv/ManageStack "{command: 3, component: '', profile: ''}"
ros2 topic echo /car_manager/component_status
ros2 topic echo /scan --once
ros2 topic echo /odom --once
ros2 topic echo /imu --once
```
