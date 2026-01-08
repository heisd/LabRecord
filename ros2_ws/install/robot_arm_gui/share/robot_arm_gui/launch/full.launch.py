"""
完整启动：robot_state_publisher + GUI
"""

from launch import LaunchDescription
from launch_ros.actions import Node
from launch.substitutions import Command
from launch_ros.parameter_descriptions import ParameterValue
import os
from ament_index_python.packages import get_package_share_directory


def generate_launch_description():
    # 获取 URDF 路径
    pkg_path = get_package_share_directory('robot_arm_gui')
    urdf_file = os.path.join(pkg_path, 'urdf', 'simple_arm.urdf')
    
    # 读取 URDF
    with open(urdf_file, 'r') as f:
        robot_description = f.read()
    
    # robot_state_publisher 节点
    robot_state_publisher = Node(
        package='robot_state_publisher',
        executable='robot_state_publisher',
        name='robot_state_publisher',
        output='screen',
        parameters=[{
            'robot_description': robot_description,
            'publish_frequency': 30.0,
        }]
    )
    
    # GUI 节点
    gui_node = Node(
        package='robot_arm_gui',
        executable='robot_arm_gui',
        name='robot_arm_gui',
        output='screen',
    )
    
    return LaunchDescription([
        robot_state_publisher,
        gui_node,
    ])
