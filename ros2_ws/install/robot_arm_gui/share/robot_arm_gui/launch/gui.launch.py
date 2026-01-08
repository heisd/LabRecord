"""
启动机器臂 GUI 和相关节点
"""

from launch import LaunchDescription
from launch_ros.actions import Node
from launch.actions import DeclareLaunchArgument
from launch.substitutions import LaunchConfiguration, Command
from launch_ros.parameter_descriptions import ParameterValue
import os
from ament_index_python.packages import get_package_share_directory


def generate_launch_description():
    # 参数
    urdf_file_arg = DeclareLaunchArgument(
        'urdf_file',
        default_value='',
        description='Path to robot URDF file'
    )
    
    # 如果有 URDF，启动 robot_state_publisher
    # 注意：你需要根据实际情况修改 URDF 路径
    
    # GUI 节点
    gui_node = Node(
        package='robot_arm_gui',
        executable='robot_arm_gui',
        name='robot_arm_gui',
        output='screen',
    )
    
    return LaunchDescription([
        urdf_file_arg,
        gui_node,
    ])
