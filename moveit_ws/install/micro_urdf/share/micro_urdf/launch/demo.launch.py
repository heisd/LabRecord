from moveit_configs_utils import MoveItConfigsBuilder
from moveit_configs_utils.launches import generate_demo_launch
import os
from ament_index_python.packages import get_package_share_directory

def generate_launch_description():
    # 1. 指定 URDF 所在的包名和文件名
    description_package = "my_robot_description" 
    urdf_file_name = "my_robot_description.urdf" # 如果你的是 .urdf.xacro 请在这里改名

    # 2. 拼接出 URDF 的绝对路径
    urdf_file_path = os.path.join(
        get_package_share_directory(description_package),
        "urdf",
        urdf_file_name
    )
    
    # 3. 构建 MoveIt 配置
    moveit_config = MoveItConfigsBuilder("my_robot_description", package_name="micro_urdf") \
        .robot_description(file_path=urdf_file_path) \
        .to_moveit_configs()

    # 4. 生成启动描述
    return generate_demo_launch(moveit_config)