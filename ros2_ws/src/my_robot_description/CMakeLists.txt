cmake_minimum_required(VERSION 3.5)
project(my_robot_description)

# 寻找 ROS 2 编译工具
find_package(ament_cmake REQUIRED)
find_package(urdf REQUIRED) 

# 安装文件夹到 ROS 2 的共享目录
# SolidWorks 导出的通常包含: launch, meshes, urdf, textures
install(
  DIRECTORY launch meshes urdf textures
  DESTINATION share/${PROJECT_NAME}
)

ament_package()