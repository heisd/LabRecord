# LabRecord

机器人学习与实验资料集合，包含多个独立的 ROS 工作空间、ESP32 示例、机械臂实验与问题排查记录。

## 内容导航

- [md](./md)：学习文档
- [ros2_ws](./ros2_ws)：ROS 2 实验工作空间
- [ObjectGrabLi](./ObjectGrabLi)：抓取实验
- [esp](./esp)：ESP32 示例
- [moveit_ws](./moveit_ws)：MoveIt 实验工作空间
- [simple_urdf](./simple_urdf)：URDF 练习
- [FixOpenProblem/OPENCV_FIX_README.md](./FixOpenProblem/OPENCV_FIX_README.md)：OpenCV 问题排查
- [ros2_ws/src/robot_arm_gui/README.md](./ros2_ws/src/robot_arm_gui/README.md)：机械臂 GUI 说明

## 使用方式

本仓库含多个独立工程，先进入所需子项目阅读配置与说明。ROS 项目应在对应工作空间根目录构建；不建议在仓库顶层直接编译所有内容。

部分实验包含本机路径与历史构建产物，运行前需核对系统、ROS 版本、设备连接与路径。
