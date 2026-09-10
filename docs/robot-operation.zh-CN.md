# 小车建图、导航与巡检操作手册

本文面向已完成构建的实体小车，适用于 Ubuntu 24.04、ROS 2 Jazzy 和本仓库的
`ros2_qt6_ws`。它说明如何安全地启动底盘与雷达、用 Cartographer 建图、保存地图、
用 Nav2 定位导航，以及执行预设路线巡检。

> `ros2_qt6_ws` 是为兼容已有路径保留的目录名；其中的操作员控制台实际使用 Qt 5，
> 并内嵌 RViz。

> **安全第一。** 首次启动或修改串口、TF、底盘参数后，请将车轮架空测试。建图和导航
> 前确认急停可用、环境内无人处于行驶路径、并且随时准备点击“停止全部”或按 `Ctrl+C`。

## 1. 系统组成与运行模式

小车的数据链路如下：

```text
底盘控制器 ──串口──> micro-ROS Agent ──> /odom、/imu、/cmd_vel
C1 雷达     ──串口──> sllidar_ros2     ──> /scan
                                  │
fishbot_bringup: /odom ──> odom -> base_footprint TF
                                  │
      Cartographer（建图）或 Nav2 + AMCL（定位/导航）
                                  │
                    /map、/tf、navigate_to_pose
```

同一时刻只能运行一种高层模式：

| 模式 | 启动内容 | 用途 |
| --- | --- | --- |
| 基础（Base） | micro-ROS Agent、C1 雷达、Fishbot TF | 检查硬件和话题 |
| 建图（Mapping） | 基础模式 + Cartographer | 实时生成 `/map` |
| 导航（Navigation） | 基础模式 + Nav2/AMCL + 巡检节点 | 基于已保存地图定位、发目标点和巡检 |

`car_stack_manager` 会在进入建图时停止导航，在进入导航时停止建图。不要同时用手工
launch 与管理服务启动同一套节点，否则会产生重复的 `/tf`、`/scan` 或 `/map` 发布者。

## 2. 每次开机前的检查

### 2.1 加载环境

每一个终端都要单独执行：

```bash
source /opt/ros/jazzy/setup.bash
cd /home/flipped/code/ros2_car/ros2_qt6_ws
source install/setup.bash
```

如果源代码或依赖刚改动过，先重新构建：

```bash
colcon build --symlink-install
source install/setup.bash
```

### 2.2 核对串口和权限

默认硬件配置为控制器 `/dev/ttyACM0`（921600）与 C1 雷达 `/dev/ttyACM1`
（460800）。实际设备序号可能随插拔改变，优先使用稳定的
`/dev/serial/by-id/...` 名称。

```bash
ls -l /dev/serial/by-id/
ls -l /dev/ttyACM* /dev/ttyUSB*
groups
```

运行账户必须属于 `dialout` 组；加入组后需要重新登录：

```bash
sudo usermod -aG dialout "$USER"
```

复制并修改本机配置，不要修改示例文件：

```bash
cd /home/flipped/code/ros2_car/ros2_qt6_ws
test -f config/vehicle.local.yaml || cp config/vehicle.example.yaml config/vehicle.local.yaml
${EDITOR:-nano} config/vehicle.local.yaml
```

至少确认这些值与硬件一致：

```yaml
car_stack_manager:
  ros__parameters:
    micro_ros_device: /dev/serial/by-id/控制器对应设备
    micro_ros_baud: 921600
    lidar_device: /dev/serial/by-id/雷达对应设备
    lidar_baud: 460800
    lidar_frame_id: laser
    use_sim_time: false
```

### 2.3 网络与 ROS 域

车载端和操作员端必须使用同一个 `ROS_DOMAIN_ID`，并且在可进行 DDS 发现的网络中。
例如两个终端都使用域 0：

```bash
export ROS_DOMAIN_ID=0
```

若通过配置启动管理服务，`vehicle.local.yaml` 的 `ros_domain_id` 也应为同一数值。
排障时先在两端运行 `ros2 node list`；看不到对方节点时，先解决网络、域 ID 或防火墙，
不要开始建图或导航。

## 3. 启动基础硬件栈

推荐先启动车载管理服务。它负责子进程生命周期和状态话题：

```bash
source /opt/ros/jazzy/setup.bash
cd /home/flipped/code/ros2_car/ros2_qt6_ws
source install/setup.bash
ros2 run car_stack_manager car_stack_manager --ros-args \
  --params-file config/vehicle.local.yaml
```

在第二个终端观察状态：

```bash
ros2 topic echo /car_manager/component_status
```

可用 Qt 控制台点击“启动基础栈”，或用命令行调用服务：

```bash
ros2 service call /car_manager/manage_stack \
  car_control_interfaces/srv/ManageStack \
  "{command: 1, component: 'base', profile: ''}"
```

确认最基本的数据均在更新。以下命令各自持续输出，按 `Ctrl+C` 退出观察：

```bash
ros2 topic hz /scan
ros2 topic hz /odom
ros2 topic hz /imu
ros2 run tf2_ros tf2_echo odom base_footprint
```

如果 `/scan` 没有数据，先检查 `lidar_device`、供电、串口权限和波特率；如果 `/odom`
或 `/imu` 没有数据，先检查控制器固件、micro-ROS 串口与波特率。没有稳定的 `/scan`、
`/odom` 和 TF 时，不能可靠建图或导航。

### 不使用管理服务的手工启动

用于调试时，可以用统一 launch 入口替代管理服务。此方式会直接启动 Agent、雷达和
Fishbot bringup；不要同时运行 `car_stack_manager`。

```bash
ros2 launch car_platform_bringup vehicle_stack.launch.py \
  mode:=base \
  micro_ros_device:=/dev/ttyACM0 micro_ros_baud:=921600 \
  lidar_device:=/dev/ttyACM1 lidar_baud:=460800
```

## 4. 建图

### 4.1 启动 Cartographer

建图是操作端 Jazzy 工作区中的独立流程；车端 Humble 工作区只提供基础硬件话题，
不安装也不接受 `mapping` 管理命令。确认基础检查通过后，选择以下一种方式启动建图。

**方式 A：Qt 控制台（推荐）**

1. 手工准备车载端基础栈，并确认 `/scan`、`/odom`、`/imu` 和 `/tf` 都可见。
2. 在操作员端启动 `ros2 run car_operator_console car_operator_console`。
3. 打开“建图”标签页，确认其中的前置条件均为活跃。
4. 输入只含字母、数字、下划线或短横线的地图名称，点击“启动建图”。控制台会在
   操作端运行 Cartographer，并显示本地建图日志与 `/map` 状态。

**方式 B：手工 launch**

```bash
ros2 launch fishbot_cartographer cartographer.launch.py use_sim_time:=false
```

在另一终端验证 Cartographer 已发布栅格地图：

```bash
ros2 topic echo /map --once
ros2 topic hz /map
ros2 run tf2_ros tf2_echo map odom
```

### 4.2 在内嵌 RViz 中观察建图质量

启动控制台后打开“地图视图”标签页。它会按需启动内嵌 RViz，Fixed Frame 已设为
`map`，并默认显示：

- **Map**：`/map`；
- **LaserScan**：`/scan`；
- **TF**：检查 `map -> odom -> base_footprint` 以及雷达到车体的坐标关系；
- **Robot Model**：检查机器人模型位置是否与 TF 一致。

如需独立窗口排障，仍可在另一个终端运行 `rviz2`，并配置相同的话题与坐标系。

推动小车缓慢、平稳地覆盖环境。建议先沿墙走一圈，再穿过主要通道并回到起点以形成
闭环。避免快速旋转、颠簸、玻璃强反射区域和人员频繁穿行；这些都会降低扫描匹配质量。

### 4.3 使用独立遥控辅助移动

“遥控”标签页与建图流程独立。只有基础栈或 Cartographer 正在运行、`/scan`、`/odom`、
`/imu`、`/tf` 均活跃，且导航已停止时，才能勾选“启用遥控”。它向 `/cmd_vel` 发布速度，
仅用于缓慢辅助移动：

1. 先确认急停可用、车辆周围留有安全距离；首次使用请架空车轮验证方向。
2. 线速度建议从 `0.15 m/s` 开始，转向速度建议从 `0.60 rad/s` 开始。
3. 必须**按住**“前进 / 后退 / 左转 / 右转”按钮，或 `W/↑`、`S/↓`、`A/←`、`D/→`，才会
   持续行驶；松开按钮或按键立即发送零速度。Space 或 `0` 可立即制动。
4. 可在“键盘映射”中为每个动作设置一个额外按键，重复按键会被拒绝；“恢复默认按键”会移除
   自定义按键并保留默认映射。切离遥控页、窗口失焦、禁用遥控、停止前提条件或关闭控制台也会
   发送零速度。

该遥控不提供避障或自主安全决策，不能替代实体急停和现场监护。不要同时运行其他会向
`/cmd_vel` 发布命令的遥控器或导航程序。

### 4.4 保存地图

看到 `/map` 稳定、闭环合理后，在“建图”标签页点击“保存地图”。它会将
`<名称>.yaml` 和 `<名称>.pgm` 写入 `fishbot_navigation2/maps/`，并自动刷新控制台
导航地图列表。停止前若尚未保存，控制台会要求保存或确认放弃；停止建图只结束操作端的
Cartographer，不会停止车端基础硬件栈。

将地图加入版本控制前，请确认它不包含不应共享的场地信息。

## 5. 定位与导航

### 5.1 启动导航栈

导航前，确保地图的 `.yaml` 与 `.pgm` 同名且位于
`fishbot_navigation2/maps/`。示例地图包括 `bot202505_map.yaml`、
`bot20250623_map.yaml`、`fishbot_map.yaml` 和 `test_map.yaml`。

**Qt 控制台：** 在“导航地图”下拉框选择地图后点击“启动导航”。

**管理服务命令行：** `profile` 只能是该目录中的文件名，不可传绝对路径。

```bash
ros2 service call /car_manager/manage_stack \
  car_control_interfaces/srv/ManageStack \
  "{command: 1, component: 'navigation', profile: 'lab_01.yaml'}"
```

**手工 launch：**

```bash
ros2 launch car_platform_bringup vehicle_stack.launch.py \
  mode:=navigation \
  map_file:=/home/flipped/code/ros2_car/ros2_qt6_ws/src/fishbot/fishbot_navigation2/maps/lab_01.yaml \
  micro_ros_device:=/dev/ttyACM0 micro_ros_baud:=921600 \
  lidar_device:=/dev/ttyACM1 lidar_baud:=460800
```

确认 Nav2 已就绪：

```bash
ros2 action list | grep navigate_to_pose
ros2 topic echo /map --once
ros2 topic echo /amcl_pose --once
```

### 5.2 设置初始位姿

AMCL 必须知道机器人位于地图中的大致位置。将小车放在地图中对应的实际位置后：

- 在 Qt 控制台填写 `X`、`Y` 与偏航角（单位：度），点击“设置初始位姿”；或
- 在控制台“地图视图”的内嵌 RViz 中使用 **2D Pose Estimate**，在地图上拖出朝向。

设置后应看到 `/amcl_pose` 持续更新，且内嵌 RViz 中的激光点云能与墙体重合。若不重合，
重新设置初始位姿；始终无法收敛时，检查地图是否属于当前环境、激光 `frame_id`、
`odom -> base_footprint` TF 和雷达安装位置。

### 5.3 发送导航目标

确认初始位姿可靠后：

- **Qt 控制台：** 填写目标 `X`、`Y`、偏航角（度）并点击“发送目标”；
- **内嵌 RViz：** 使用 **Nav2 Goal** 在地图上指定目标和朝向；
- **命令行：**

```bash
ros2 action send_goal /navigate_to_pose nav2_msgs/action/NavigateToPose \
  "{pose: {header: {frame_id: map}, pose: {position: {x: 1.0, y: 0.5, z: 0.0}, orientation: {z: 0.0, w: 1.0}}}}"
```

发送前检查目标点在已知可通行区域内，预留足够的车体安全边界。需要紧急终止时，先在
控制台点击“取消目标”；若车辆行为异常，使用物理急停，再点击“停止全部”。

## 6. 预设路线巡检

导航模式会自动启动 `car_patrol`。路线文件为
`ros2_qt6_ws/src/car_patrol/config/patrol_routes.yaml`，每个路线包含一个 `start` 位姿和
多个 `points`；所有偏航角均为**度**。巡检会依次访问各点，最后返回 `start`。

修改路线前先备份，并确保坐标来自同一张地图：

```yaml
routes:
  warehouse_a:
    start: {x: 0.0, y: 0.0, yaw_deg: 0.0}
    points:
      - {x: 1.2, y: 0.3, yaw_deg: 90.0}
      - {x: 2.0, y: -0.8, yaw_deg: 180.0}
```

修改后重启导航模式，使巡检节点重新读取路线。当前 Qt 控制台仅显示 `default` 路线；
要运行其他路线，可从命令行发送 Action：

```bash
ros2 action send_goal /car_patrol/run car_control_interfaces/action/RunPatrol \
  "{route_name: 'warehouse_a'}" --feedback
```

巡检开始前会发布路线起点作为初始位姿，因此仍应先确认 AMCL 已经正常收敛。取消巡检可
在控制台点击“取消巡检”，或使用 `ros2 action cancel /car_patrol/run`。

## 7. 常用诊断与故障处理

| 现象 | 优先检查 | 建议处理 |
| --- | --- | --- |
| Agent 无法打开串口 | 设备路径、`dialout` 组、波特率 | 使用 `/dev/serial/by-id`，重新登录后再试 |
| `/scan` 为空 | 雷达电源、`lidar_device`、460800 波特率 | 先只启动基础栈，确认 `ros2 topic hz /scan` |
| `/odom` 或 `/imu` 为空 | 控制器固件、micro-ROS Agent 串口 | 检查 Agent 日志与控制器串口连接 |
| RViz 报 TF transform 不可用 | `/tf`、`/odom`、车体/雷达 frame 名 | 用 `tf2_echo odom base_footprint` 定位缺失链路 |
| 建图重影或墙体弯曲 | 轮滑、雷达松动、快速转动、里程计尺度 | 降速重建，固定传感器，校准底盘里程计 |
| AMCL 不收敛 | 地图不匹配、初始位姿偏差大、激光不对齐 | 用 RViz 重设初始位姿，核对地图与 TF |
| Nav2 不接收目标 | 导航未就绪或还在建图模式 | 确认 `/navigate_to_pose` 存在；停止建图并重新启动导航 |
| 控制台看不到车辆 | `ROS_DOMAIN_ID` 或 DDS 网络不一致 | 两端使用相同域 ID，检查防火墙与组播网络 |

以下命令适合快速收集现场信息：

```bash
ros2 node list
ros2 topic list
ros2 topic hz /scan
ros2 topic hz /odom
ros2 topic echo /car_manager/component_status
ros2 run tf2_tools view_frames
```

生成的 `frames.pdf` 可用于检查 TF 树是否闭合。排查时每次只启动一个模式，并记录
`/scan`、`/odom`、`/map` 与 Agent 的日志，能显著缩小问题范围。

## 8. 正常停止顺序

任务结束后先取消当前导航或巡检，再停止所有车载进程：

```bash
ros2 service call /car_manager/manage_stack \
  car_control_interfaces/srv/ManageStack \
  "{command: 2, component: 'all', profile: ''}"
```

确认底盘已经停稳后，退出控制台和管理服务。若使用手工 launch，在对应终端按 `Ctrl+C`
并等待节点完全退出后再关闭电源。
