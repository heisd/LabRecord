# my_robot_description

`my_robot_description` 是一个 ROS 2 机器人描述包，用于集中管理机器人模型与可视化所需资源。

## 功能说明

该包主要用于：

- 维护机器人 URDF/Xacro 描述文件；
- 管理可视化和碰撞检测所需的 Mesh 资源；
- 提供机器人模型加载相关的 launch 文件；
- 存放 RViz 或其他工具使用的配置文件。

## 目录结构（常见约定）

- `urdf/`：机器人结构描述文件（`.urdf` / `.xacro`）。
- `meshes/`：机器人网格模型（视觉/碰撞）。
- `launch/`：启动文件（发布 `robot_description`、启动可视化等）。
- `config/`：参数与工具配置。

> 注意：实际目录内容以仓库当前文件为准。

## 快速开始

请在工作空间根目录 `ros2_ws` 下执行：

```bash
colcon build --packages-select my_robot_description
source install/setup.bash
```

## 典型使用方式

构建并 source 后，可通过 launch 文件加载机器人描述。示例：

```bash
ros2 launch my_robot_description display.launch.py
```

如果你的包中使用了其他 launch 文件，请替换为对应文件名。

## 依赖环境

- ROS 2（建议与本仓库其余包保持同一发行版）；
- `colcon` 构建工具；
- `robot_state_publisher`、`joint_state_publisher`、`rviz2`（按 launch 配置需要安装）。

## 维护建议

- 修改 URDF/Xacro 后，建议在 RViz 中验证 TF、关节层级和模型姿态；
- 引入新 Mesh 时，统一单位与坐标系约定（米制、右手系）；
- 提交前执行最小构建验证，确保包可被正确索引与安装。
