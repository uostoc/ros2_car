# ROS 2 + Qt 6 小车控制台

[English](README.md) | [简体中文](README.zh-CN.md)

本仓库提供一个统一的 ROS 2 Humble 工作区，包含 Fishbot 平台、micro-ROS
Agent、车载端进程管理服务、路线巡检节点和 Qt 6 操作员控制台。原始资料仍保留在
被 Git 忽略的 `source/` 目录中，仅作为本地参考。

## 目录结构

- `ros2_qt6_ws/src/fishbot`：迁移的 Fishbot 功能包、驱动、地图和探索功能包。
- `ros2_qt6_ws/src/micro-ROS-Agent` 与 `micro_ros_msgs`：迁移的 micro-ROS Agent
  源码。
- `ros2_qt6_ws/src/car_*`：维护中的车辆控制功能包。
- `ros2_qt6_ws/config/vehicle.example.yaml`：安全且受版本控制的部署默认配置。
  修改设备路径前，请先复制为 `vehicle.local.yaml`。
- `deploy/`：systemd 和串口设备部署示例。

## 在 Ubuntu 22.04 上构建

安装 ROS 2 Humble、Qt 6 Widgets 与 Qt Linguist 开发工具，以及 `docs/operations.md` 中列出的
ROS 软件包，然后执行：

```bash
source /opt/ros/humble/setup.bash
cd ros2_qt6_ws
rosdep install --from-paths src --ignore-src -r -y
colcon build --symlink-install
source install/setup.bash
```

使用本地配置文件在车载端启动进程管理服务：

```bash
# 仅首次执行：保留已有的机器专用配置。
test -f config/vehicle.local.yaml || cp config/vehicle.example.yaml config/vehicle.local.yaml
ros2 run car_stack_manager car_stack_manager --ros-args \
  --params-file config/vehicle.local.yaml
```

## 在新终端中运行

每个新终端都必须先加载 ROS 2 Humble 底层环境和本工作区，然后才能运行任一
可执行程序。在车载计算机，或同一 ROS 2 DDS 网络中的另一台 Ubuntu 22.04
计算机上执行：

```bash
source /opt/ros/humble/setup.bash
cd /path/to/ros2_qt6_ws
source install/setup.bash
ros2 run car_operator_console car_operator_console
```

通过控制台菜单栏中的“语言”在 English 与简体中文之间切换。选择会立即生效，并在该
计算机后续启动时自动恢复。
