# ROS 2 + Qt 5 Car Console

[English](README.md) | [简体中文](README.zh-CN.md)

This repository contains a single ROS 2 Jazzy workspace for the Fishbot
platform, micro-ROS Agent, a vehicle-side stack manager, a route patrol node,
and a Qt 5 operator console with embedded RViz. The original material remains in the ignored
`source/` directory as a local reference only.

## Layout

- `ros2_qt6_ws/src/fishbot`: migrated Fishbot packages, drivers, maps, and
  exploration packages.
- `ros2_qt6_ws/src/micro-ROS-Agent` and `micro_ros_msgs`: migrated micro-ROS
  Agent sources.
- `ros2_qt6_ws/src/car_*`: maintained vehicle control packages.
- `ros2_qt6_ws/config/vehicle.example.yaml`: safe, versioned deployment
  defaults. Copy it to `vehicle.local.yaml` before changing device paths.
- `deploy/`: systemd and serial-device deployment examples.

## Build on Ubuntu 24.04

Install ROS 2 Jazzy, Qt 5 Widgets and Qt Linguist development tools, RViz, and the ROS packages
listed in `docs/operations.md`. Then run:

```bash
source /opt/ros/jazzy/setup.bash
cd ros2_qt6_ws
rosdep install --from-paths src --ignore-src -r -y --skip-keys ament_python
colcon build --symlink-install
source install/setup.bash
```

Start the vehicle manager locally with the local configuration file:

```bash
# First-time setup only: preserve an existing machine-specific configuration.
test -f config/vehicle.local.yaml || cp config/vehicle.example.yaml config/vehicle.local.yaml
ros2 run car_stack_manager car_stack_manager --ros-args \
  --params-file config/vehicle.local.yaml
```

## Run in a New Terminal

Every new shell must load both the ROS 2 Jazzy underlay and this workspace
before running either executable. On the vehicle computer or another Ubuntu
24.04 computer on the same ROS 2 DDS network, run:

```bash
source /opt/ros/jazzy/setup.bash
cd /path/to/ros2_qt6_ws
source install/setup.bash
ros2 run car_operator_console car_operator_console
```

Use the **Language** menu in the console to switch between English and
Simplified Chinese. The selection takes effect immediately and is remembered
for future launches on that computer. The **Map view** tab starts an embedded
RViz view with map, laser, TF, robot model, initial-pose, and Nav2-goal tools.

`ros2_qt6_ws` is a retained workspace directory name for compatibility; the
operator console itself uses Qt 5.
