# Vehicle deployment

[English](operations.md) | [简体中文](operations.zh-CN.md)

## Prerequisites

The supported runtime is Ubuntu 24.04 with ROS 2 Jazzy, Qt 5, and embedded RViz.

```bash
sudo apt install qtbase5-dev qttools5-dev qttools5-dev-tools \
  ros-jazzy-navigation2 ros-jazzy-robot-localization ros-jazzy-cartographer \
  ros-jazzy-cartographer-ros ros-jazzy-rviz2 ros-jazzy-nav2-rviz-plugins
```

Run `rosdep install --from-paths src --ignore-src -r -y --skip-keys ament_python`
in `ros2_qt6_ws` to install package-level dependencies as well. The directory
name is retained for compatibility; the operator console itself uses Qt 5.
`ament_python`
is supplied by the sourced ROS 2 environment and has no Ubuntu rosdep key.

## Device configuration

Copy `ros2_qt6_ws/config/vehicle.example.yaml` to
`ros2_qt6_ws/config/vehicle.local.yaml` and set the actual stable device paths.
The defaults preserve the existing setup: the micro-ROS controller is
`/dev/ttyACM0` at 921600 baud and the C1 lidar is `/dev/ttyACM1` at 460800 baud.

Add the runtime user to `dialout`; do not grant global read/write access with
`chmod 666`:

```bash
sudo usermod -aG dialout ros
```

If stable `/dev/serial/by-id/...` paths are available, use those in the local
configuration. An optional udev template is in `deploy/udev`.

## Vehicle service and remote console

Install `deploy/systemd/car-stack-manager.service`, adjust its workspace and
configuration paths, and enable it with `systemctl enable --now
car-stack-manager`. The manager exposes `/car_manager/manage_stack` and
`/car_manager/component_status` over DDS.

When starting Navigation from the Qt console, choose one of the maps included
by `fishbot_navigation2`. The selected map filename is validated by the vehicle
manager and resolved inside that package; it cannot be used to run arbitrary
paths or commands.

The vehicle and operator computers must use the same `ROS_DOMAIN_ID` and be on
a network that permits ROS 2 DDS discovery. The Qt console has no SSH password
or private-key configuration: it controls the vehicle through these ROS 2
interfaces.

The console menu bar provides **Language**, with English and Simplified Chinese
options. The selected language is applied immediately and remembered for the
next launch on that computer; the first launch uses English. ROS service and
action error details are shown as received from the vehicle.

The **Map view** tab lazily starts an embedded RViz instance. It uses `map` as
the fixed frame and includes `/map`, `/scan`, TF, and robot-model displays.
Use RViz **2D Pose Estimate** to publish `/initialpose` and **Nav2 Goal** to
send `navigate_to_pose` goals. The console coordinate controls remain available
for precise input.

## Operating modes

- **Base** starts micro-ROS Agent, C1 lidar, and Fishbot TF bringup.
- **Navigation** starts Base and Nav2 with the configured map.
- **Mapping** is an independent Jazzy-operator workflow. Prepare Base manually on the
  vehicle, then use the console's **Mapping** tab to start local Cartographer and save
  named YAML/PGM maps into `fishbot_navigation2/maps`.

The Mapping tab provides constrained `/cmd_vel` teleoperation only while local
mapping is running and `/scan`, `/odom`, `/imu`, and `/tf` are active. Movement
buttons are press-and-hold: releasing a button, clicking **Stop**, stopping
mapping or all components, and closing the console all publish a zero velocity.
It has no obstacle avoidance, so retain a physical e-stop and an attentive
operator; never run another `/cmd_vel` command source at the same time. Patrol
sends Nav2 goals for each configured route point and then returns to its declared
start pose.
