cmake_minimum_required(VERSION 3.8)
project(robot_arm_gui)

# C++17 for ROS2
if(NOT CMAKE_CXX_STANDARD)
  set(CMAKE_CXX_STANDARD 17)
endif()

if(CMAKE_COMPILER_IS_GNUCXX OR CMAKE_CXX_COMPILER_ID MATCHES "Clang")
  add_compile_options(-Wall -Wextra -Wpedantic)
endif()

# Qt5 自动处理
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)

# 查找依赖
find_package(ament_cmake REQUIRED)
find_package(rclcpp REQUIRED)
find_package(rviz_common REQUIRED)
find_package(rviz_rendering REQUIRED)
find_package(rviz_default_plugins REQUIRED)
find_package(sensor_msgs REQUIRED)
find_package(geometry_msgs REQUIRED)
find_package(tf2_ros REQUIRED)
find_package(pluginlib REQUIRED)

find_package(Qt5 REQUIRED COMPONENTS 
  Core 
  Widgets 
  Gui
)

# 头文件
include_directories(
  include
  ${Qt5Core_INCLUDE_DIRS}
  ${Qt5Widgets_INCLUDE_DIRS}
  ${Qt5Gui_INCLUDE_DIRS}
)

# 主程序
add_executable(robot_arm_gui
  src/main.cpp
  src/main_window.cpp
  src/rviz_panel.cpp
  src/joint_control_panel.cpp
)

target_link_libraries(robot_arm_gui
  Qt5::Core
  Qt5::Widgets
  Qt5::Gui
)

ament_target_dependencies(robot_arm_gui
  rclcpp
  rviz_common
  rviz_rendering
  rviz_default_plugins
  sensor_msgs
  geometry_msgs
  tf2_ros
  pluginlib
)

# 安装
install(TARGETS robot_arm_gui
  DESTINATION lib/${PROJECT_NAME}
)

install(DIRECTORY launch config
  DESTINATION share/${PROJECT_NAME}
  OPTIONAL
)

ament_package()
