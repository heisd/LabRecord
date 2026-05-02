# LabRecord（实验记录笔记）

> 这是一个“实验笔记型”仓库：按主题记录 ROS 2 / MoveIt / micro-ROS / 嵌入式联调过程、可复现步骤与参考资料。

---

## 1. 仓库定位

- 用途：保存实验过程、问题排查、代码样例与工作空间快照。
- 风格：以“可回看、可复现、可检索”为目标。
- 读者：未来的自己 + 实验室同学。

---

## 2. 仓库框架（Note Style）

### 2.1 工作空间（可运行）

- `ros2_ws/`：ROS 2 主工作空间（GUI/描述包等）。
- `moveit_ws/`：MoveIt 相关工作空间。
- `micro_ros_ws/`：micro-ROS 相关工作空间。
- `li_ros2/`：历史 ROS 2 试验工作空间。

### 2.2 实验记录（文档/素材）

- `reference/`：主题式参考记录（micro-ROS、相机、SLAM、串口问题等）。
- `md/`：补充 markdown 资料。
- `picture/`：截图与示意图。

### 2.3 其他

- `esp/`：ESP32 / ESP-IDF / Arduino 侧实验内容。
- `FixOpenProblem/`：特定问题修复脚本与说明。

---

## 3. 推荐笔记规范（建议后续统一）

每次实验可按以下模板新增一条记录：

1. **日期与目标**：今天要验证什么。
2. **环境信息**：ROS 发行版、系统、硬件、串口号。
3. **操作步骤**：按命令顺序记录。
4. **现象与日志**：成功/失败、关键报错。
5. **结论**：是否达成目标。
6. **下一步**：后续待办。

---

## 4. 常用命令备忘

在对应工作空间根目录执行：

```bash
colcon build --symlink-install
source install/setup.bash
```

按包构建（示例）：

```bash
colcon build --packages-select my_robot_description
```

---

## 5. 快速导航

- 包文档：`ros2_ws/src/my_robot_description/README.md`
- micro-ROS 记录：`reference/micro_ros_reference.md`
- OpenCV 问题修复：`FixOpenProblem/OPENCV_FIX_README.md`

---

## 6. 清理说明

本仓库已通过 `.gitignore` 忽略常见构建产物（如 `build/`、`install/`、`log/`）和本地缓存，减少无关文件干扰，便于把仓库当作长期实验笔记维护。
