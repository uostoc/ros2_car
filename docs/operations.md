# Vehicle deployment

## Prerequisites

The supported runtime is Ubuntu 22.04 with ROS 2 Humble and Qt 6.

```bash
sudo apt install qt6-base-dev qt6-base-dev-tools qt6-tools-dev qt6-l10n-tools \
  ros-humble-nav2-bringup ros-humble-nav2-map-server \
  ros-humble-robot-localization ros-humble-cartographer \
  ros-humble-cartographer-ros
```

Run `rosdep install --from-paths src --ignore-src -r -y` in `ros2_qt6_ws` to
install package-level dependencies as well.

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

## Operating modes

- **Base** starts micro-ROS Agent, C1 lidar, and Fishbot TF bringup.
- **Navigation** starts Base and Nav2 with the configured map.
- **Mapping** starts Base and Cartographer. It automatically stops Navigation.

The first release intentionally has no manual `/cmd_vel` control and no camera
integration. Patrol sends Nav2 goals for each configured route point and then
returns to its declared start pose.
