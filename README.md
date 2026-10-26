# LabRecord · 机器人实验与学习资料

机器人学习过程中的综合实验仓库，汇集 ROS 工作空间、ESP32 / micro-ROS 通信、URDF 与 MoveIt 配置、抓取实验和故障排查笔记。各目录来自不同阶段，包含独立工程与部分历史构建产物，应按子项目分别使用。

## 从哪里开始

| 目标 | 阅读入口 |
| --- | --- |
| ESP32 与主机通信 | [md/esp32-ros2-connection.md](./md/esp32-ros2-connection.md)、[md/ros2-esp32-connection.md](./md/ros2-esp32-connection.md) |
| 已有链路的快速检查 | [md/test_process.md](./md/test_process.md) |
| 抓取实验结构 | [ObjectGrabLi/](./ObjectGrabLi) |
| 机械臂 GUI | [ros2_ws/src/robot_arm_gui/README.md](./ros2_ws/src/robot_arm_gui/README.md) |
| MoveIt 配置 | [moveit_ws/](./moveit_ws) |
| OpenCV 链接问题 | [FixOpenProblem/OPENCV_FIX_README.md](./FixOpenProblem/OPENCV_FIX_README.md) |
| 串口与相机问题 | [reference/](./reference) |

## 仓库组成

| 目录 | 主要用途 |
| --- | --- |
| `ros2_ws/` | ROS 2 实验工作空间 |
| `ObjectGrabLi/` | 抓取实验，包含 bringup、vision、manipulation、calibration 等包 |
| `moveit_ws/` | MoveIt 配置与机器人描述 |
| `li_ros2/` | 机器人描述与模型相关实验 |
| `micro_ros_ws/` | micro-ROS 相关工作内容，含历史生成产物 |
| `esp/` | ESP32 语音/舵机与 micro-ROS 示例 |
| `simple_urdf/` | URDF 练习 |
| `src/` | 其他实验源码 |
| `md/`、`reference/` | 笔记、排障与参考资料 |
| `picture/` | 配图 |

仓库中机器人描述目录有保留历史拼写的名称，例如 `robot_desciption`；使用脚本和命令时应以实际路径为准。

## 获取与环境选择

```bash
git clone --branch liquanyan https://github.com/heisd/LabRecord.git
cd LabRecord
```

本仓库没有单一的 Python 版本、ROS 发行版和统一依赖清单。首先选择具体子项目，再检查其 `package.xml`、`CMakeLists.txt`、`setup.py` 与对应笔记。不能把所有目录都当成同一个可直接构建的工作空间。

## ROS 子工作空间构建方法

以选择 `ros2_ws/` 为例，在已经配置相应 ROS 环境与 rosdep 的终端：

```bash
cd ros2_ws
rosdep install --from-paths src --ignore-src -r -y
colcon build --symlink-install
source install/setup.bash
```

这是按目录结构给出的通用构建入口，不保证所有历史实验包在当前系统中一次编译成功。可以先执行 `colcon list` 核对被发现的包，再缩小到所需包及依赖。

切换到另一个工作空间时，注意终端中已加载的 overlay，避免同名包或旧生成产物影响结果。已提交的 `build/`、`install/` 或二进制文件可能来自不同机器，不能替代本机重建。

## micro-ROS 通信检查

[测试笔记](./md/test_process.md) 记录了两种 Agent 启动方式，选择与固件传输配置一致的一种：

```bash
# 串口传输
ros2 run micro_ros_agent micro_ros_agent serial --dev /dev/ttyUSB0 -b 115200

# UDP/Wi-Fi 传输
ros2 run micro_ros_agent micro_ros_agent udp4 --port 8888
```

这里假设 Agent 已单独安装。串口名、权限、波特率、IP 与端口应按硬件和固件核对；不要同时把两种传输当作同一设备的必选步骤。

笔记中的示例话题为：

```bash
ros2 topic list -t
ros2 topic echo /esp32_to_host
ros2 topic pub --once /host_to_esp32 std_msgs/msg/String "data: 'LED_ON'"
```

这些名称只适用于对应实验；先用 `ros2 topic list -t` 确认实际接口。主机桥接示例可从 [reference/micro_ros3/host_bridge_node.py](./reference/micro_ros3/host_bridge_node.py) 继续阅读。

## 抓取与标定实验

`ObjectGrabLi/src/` 包含机器人启动、标定、接口、视觉与操作相关包。其中部分文件仍属于框架或占位实现；目录存在不代表完整抓取流程已经实现。

使用前需要同时检查机器人描述、相机内参、手眼标定与实际控制接口，不能只运行一个 launch 就假定坐标系统已经正确。

## 常见问题

| 现象 | 优先排查 |
| --- | --- |
| 一次发现很多不相关包 | 是否在仓库顶层直接构建；切换到所选子工作空间 |
| 加载到旧节点 | 当前 overlay 和 install 路径 |
| Agent 在线但没有预期话题 | 固件传输配置、ROS 域与对应实验接口 |
| 图像与目标位置不一致 | 相机参数、深度、TF 和标定文件 |
| 本机绝对路径失效 | 搜索对应配置中的历史路径，替换成本机位置 |
| 某目录无法运行 | 检查是否只有框架/笔记，先阅读实际实现 |

本仓库适合作为实验过程与排障资料索引，各子项目的成熟程度与环境前提并不一致。
