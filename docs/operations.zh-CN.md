# 车辆部署

[English](operations.md) | [简体中文](operations.zh-CN.md)

## 前置条件

支持的运行环境为 Ubuntu 22.04、ROS 2 Humble 和 Qt 6。

```bash
sudo apt install qt6-base-dev qt6-base-dev-tools qt6-tools-dev qt6-tools-dev-tools qt6-l10n-tools \
  ros-humble-nav2-bringup ros-humble-nav2-map-server \
  ros-humble-robot-localization ros-humble-cartographer \
  ros-humble-cartographer-ros
```

在 `ros2_qt6_ws` 中执行
`rosdep install --from-paths src --ignore-src -r -y --skip-keys ament_python`
安装各功能包的依赖。`ament_python` 由已加载的 ROS 2 环境提供，没有对应的
Ubuntu rosdep 键。

## 设备配置

将 `ros2_qt6_ws/config/vehicle.example.yaml` 复制为
`ros2_qt6_ws/config/vehicle.local.yaml`，再填入实际稳定的设备路径。默认配置沿用
现有设备：micro-ROS 控制器使用 `/dev/ttyACM0`、波特率 921600；C1 雷达使用
`/dev/ttyACM1`、波特率 460800。

将运行用户加入 `dialout` 组；不要通过 `chmod 666` 赋予所有用户串口读写权限：

```bash
sudo usermod -aG dialout ros
```

若能使用稳定的 `/dev/serial/by-id/...` 路径，请将它们写入本地配置。可选的 udev
规则模板位于 `deploy/udev`。

## 车载服务与远程控制台

安装 `deploy/systemd/car-stack-manager.service`，调整其中的工作区和配置文件路径，
然后通过 `systemctl enable --now car-stack-manager` 启用服务。管理服务通过 DDS
提供 `/car_manager/manage_stack` 和 `/car_manager/component_status`。

从 Qt 控制台启动导航时，请选择 `fishbot_navigation2` 内置的地图。车载管理服务会
校验选择的地图文件名，并在该功能包内解析它，因此不能借由地图参数执行任意路径或
命令。

车载端和操作员端必须使用相同的 `ROS_DOMAIN_ID`，并处在允许 ROS 2 DDS 发现的
网络中。Qt 控制台不使用 SSH 密码或私钥配置，而是通过这些 ROS 2 接口控制车辆。

控制台菜单栏提供**语言**菜单，可选择英文和简体中文。语言切换会立即生效，并在
同一台电脑上记住；首次启动时使用英文。ROS 服务和 Action 的错误详情会按车载端
返回的内容显示。

## 运行模式

- **基础模式（Base）**：启动 micro-ROS Agent、C1 雷达和 Fishbot TF bringup。
- **导航模式（Navigation）**：启动基础模式和使用所选地图的 Nav2。
- **建图模式（Mapping）**：启动基础模式和 Cartographer，并自动停止导航模式。

首版刻意不提供手动 `/cmd_vel` 控制，也不集成相机。巡检会为路线中的每个点发送
Nav2 目标，最后返回配置的起始点。
