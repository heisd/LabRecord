# Robot Arm GUI - RViz2 嵌入版

Qt5 界面嵌入 RViz2 3D 视图，用于可视化控制机械臂。

## 功能

- ✅ 嵌入 RViz2 3D 渲染窗口
- ✅ 6 轴关节滑块控制
- ✅ 实时发布 `/joint_states`
- ✅ 显示 RobotModel、Grid、TF
- ✅ 快捷动作按钮（归零、准备位）

## 界面预览

```
┌────────────────────────────────────────────────────────┐
│  机器臂控制 - RViz2 嵌入版                              │
├────────────────────────────────────┬───────────────────┤
│                                    │  🦾 关节控制      │
│                                    │  ─────────────    │
│         RViz2 3D 视图              │  J1: [====] 0°   │
│                                    │  J2: [====] 0°   │
│      (显示机械臂模型)               │  J3: [====] 0°   │
│                                    │  J4: [====] 0°   │
│                                    │  J5: [====] 0°   │
│                                    │  J6: [====] 0°   │
│                                    │                  │
│                                    │  [归零] [准备位]  │
├────────────────────────────────────┴───────────────────┤
│  状态: 关节角度: J1=0° J2=0° J3=0° J4=0° J5=0° J6=0°   │
└────────────────────────────────────────────────────────┘
```

## 依赖

```bash
# ROS2 依赖
sudo apt install ros-${ROS_DISTRO}-rviz2 \
                 ros-${ROS_DISTRO}-rviz-common \
                 ros-${ROS_DISTRO}-rviz-rendering \
                 ros-${ROS_DISTRO}-rviz-default-plugins \
                 ros-${ROS_DISTRO}-robot-state-publisher

# Qt5 依赖
sudo apt install qtbase5-dev qt5-qmake
```

## 编译

```bash
# 1. 将此包复制到你的 ROS2 工作空间
cp -r robot_arm_gui ~/ros2_ws/src/

# 2. 编译
cd ~/ros2_ws
colcon build --packages-select robot_arm_gui

# 3. source
source install/setup.bash
```

## 运行

### 方式一：使用自带的简单 URDF 测试

```bash
# 需要先安装 urdf 文件到 share 目录
mkdir -p ~/ros2_ws/install/robot_arm_gui/share/robot_arm_gui/urdf
cp src/robot_arm_gui/urdf/simple_arm.urdf \
   ~/ros2_ws/install/robot_arm_gui/share/robot_arm_gui/urdf/

# 启动
ros2 launch robot_arm_gui full.launch.py
```

### 方式二：使用你自己的机械臂 URDF

```bash
# 终端 1: 启动 robot_state_publisher
ros2 launch your_robot_description robot.launch.py

# 终端 2: 启动 GUI
ros2 run robot_arm_gui robot_arm_gui
```

## 连接 ESP32-S3

确保你的 ESP32-S3 上运行的 micro-ROS 节点订阅 `/joint_states` 话题：

```cpp
// ESP32 micro-ROS 代码示例
#include <sensor_msgs/msg/joint_state.h>

void joint_state_callback(const sensor_msgs__msg__JointState * msg) {
    for (size_t i = 0; i < msg->position.size && i < 6; i++) {
        float angle_rad = msg->position.data[i];
        // 转换为舵机 PWM 并控制
        set_servo_angle(i, angle_rad);
    }
}
```

## 自定义

### 修改关节数量

在 `src/main_window.cpp` 中修改：

```cpp
joint_names_ = {"joint_1", "joint_2", ...};  // 关节名称
current_joint_angles_.resize(N, 0.0);        // N 个关节
```

### 修改关节限位

```cpp
std::vector<std::pair<double, double>> limits = {
    {-180, 180},  // Joint 1
    {-90, 90},    // Joint 2
    // ...
};
```

### 添加更多 RViz 显示

在 `src/rviz_panel.cpp` 的 `initialize()` 后添加：

```cpp
// 添加点云
auto cloud = manager_->createDisplay("rviz_default_plugins/PointCloud2", "PointCloud", true);
cloud->subProp("Topic")->setValue("/camera/points");

// 添加图像
auto image = manager_->createDisplay("rviz_default_plugins/Image", "Camera", true);
image->subProp("Topic")->setValue("/camera/image_raw");
```

## 故障排除
[ERROR] [robot_arm_gui-2]: process has died [pid 21305, exit code -11, cmd '/home/dxf/Desktop/li/ros2_ws/install/robot_arm_gui/lib/robot_arm_gui/robot_arm_gui --ros-args -r __node:=robot_arm_gui'].

### 黑屏问题

如果 RViz 区域显示黑屏：

1. 确保显卡驱动正确安装
2. 尝试设置环境变量：`export LIBGL_ALWAYS_SOFTWARE=1`
3. 检查 Ogre 渲染后端

### robot_description 话题为空

确保 `robot_state_publisher` 正在运行并发布 `/robot_description` 话题：

```bash
ros2 topic echo /robot_description --once
```

### 模型不显示

检查 TF 树是否正确：

```bash
ros2 run tf2_tools view_frames
```

## 项目结构

```
robot_arm_gui/
├── CMakeLists.txt
├── package.xml
├── README.md
├── include/robot_arm_gui/
│   ├── main_window.hpp      # 主窗口
│   ├── rviz_panel.hpp       # RViz 嵌入面板
│   └── joint_control_panel.hpp  # 关节控制面板
├── src/
│   ├── main.cpp
│   ├── main_window.cpp
│   ├── rviz_panel.cpp
│   └── joint_control_panel.cpp
├── launch/
│   ├── gui.launch.py        # 仅 GUI
│   └── full.launch.py       # GUI + robot_state_publisher
└── urdf/
    └── simple_arm.urdf      # 测试用 URDF
```

## License

MIT
