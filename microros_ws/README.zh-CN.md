# ROS 2 Humble 车载基础工作区

[English](README.md) | [简体中文](README.zh-CN.md)

本工作区是面向 Ubuntu 22.04 / ROS 2 Humble 实体车载端的独立源码树，仅提供基础
硬件能力。它包含 micro-ROS Agent、C1 雷达驱动、Fishbot URDF/TF 启动、车端进程管理
服务，以及与 Jazzy 操作员控制台共用的 DDS 接口。

本工作区刻意不包含 Qt、RViz、Nav2、Cartographer、建图、导航、巡检或探索功能包。

## 安装依赖

在车载计算机安装 ROS 2 Humble 与基础栈依赖：

```bash
sudo apt update
sudo apt install ros-humble-ros-base ros-humble-robot-state-publisher \
  ros-humble-joint-state-publisher ros-humble-rmw-fastrtps-cpp \
  python3-colcon-common-extensions python3-rosdep build-essential cmake git
sudo rosdep init  # 仅当此计算机尚未初始化 rosdep 时执行
rosdep update
```

首次构建需要联网：`micro_ros_agent` 会下载已固定版本的
Micro-XRCE-DDS-Agent 依赖（`v2.4.2`）。

## 构建

```bash
source /opt/ros/humble/setup.bash
cd /opt/ros2-car/microros_ws
rosdep install --from-paths src --ignore-src -r -y --skip-keys ament_python
colcon build --symlink-install
source install/setup.bash
```

如需手工启动基础栈，设备参数应与进程管理服务保持一致：

```bash
ros2 launch car_vehicle_bringup vehicle_base.launch.py \
  micro_ros_device:=/dev/ttyACM0 micro_ros_baud:=921600 \
  lidar_device:=/dev/ttyACM1 lidar_baud:=460800 lidar_frame_id:=laser
```

正常运行时应只启动进程管理服务，由它管理全部子进程：

```bash
cp config/vehicle.example.yaml /tmp/vehicle.yaml
ros2 run car_stack_manager car_stack_manager --ros-args --params-file /tmp/vehicle.yaml
```

随后可通过 `/car_manager/manage_stack` 操作 `base`、`agent`、`lidar`、`bringup`
或 `all`。`navigation`、`mapping` 和 `patrol` 会按设计返回错误，因为它们不属于该
Humble 基础工作区。

## 安装系统服务

将 `config/vehicle.example.yaml` 复制到 `/etc/ros2-car/vehicle.yaml` 后，按实际
设备修改串口路径。将 `deploy/systemd/car-stack-manager-humble.service` 安装为
`/etc/systemd/system/car-stack-manager.service`；若工作区不在
`/opt/ros2-car/microros_ws`，需同步修改服务文件中的路径。

再将 `deploy/systemd/vehicle.env.example` 复制为 `/etc/ros2-car/vehicle.env`，并使
其中的 `ROS_DOMAIN_ID` 与 `vehicle.yaml` 的 `ros_domain_id` 相同，最后执行：

```bash
sudo systemctl daemon-reload
sudo systemctl enable --now car-stack-manager
```

运行服务的账户必须属于 `dialout` 组。优先使用稳定的
`/dev/serial/by-id/...` 路径，不建议依赖可能变化的 `/dev/ttyACM*` 编号。

## 与 Jazzy 操作员控制台联调

车载端与操作端必须使用相同的 `ROS_DOMAIN_ID` 和 `RMW_IMPLEMENTATION`，建议两端
都使用 `rmw_fastrtps_cpp`。两端还必须构建内容完全一致的
`car_control_interfaces` IDL。

在 Jazzy 操作员计算机执行以下命令确认连通性：

```bash
ros2 service call /car_manager/manage_stack \
  car_control_interfaces/srv/ManageStack "{command: 3, component: '', profile: ''}"
ros2 topic echo /car_manager/component_status
ros2 topic echo /scan --once
ros2 topic echo /odom --once
ros2 topic echo /imu --once
```

服务查询成功且可收到状态、雷达、里程计和 IMU 数据后，才可由 Jazzy 控制台请求启动
车端基础栈。
