from moveit_configs_utils import MoveItConfigsBuilder
from moveit_configs_utils.launches import generate_demo_launch
import os
from ament_index_python.packages import get_package_share_directory

def generate_launch_description():
    description_package = "my_robot_description" 
    urdf_file_name = "my_robot_description.urdf"
    
    urdf_file_path = os.path.join(
        get_package_share_directory(description_package),
        "urdf",
        urdf_file_name
    )
    
    moveit_config = (
        MoveItConfigsBuilder(robot_name="my_robot_description", package_name="micro_urdf")
        .robot_description(file_path=urdf_file_path)
        .robot_description_semantic(file_path="config/my_robot_description.srdf")
        .robot_description_kinematics(file_path="config/kinematics.yaml")
        .joint_limits(file_path="config/joint_limits.yaml")
        .trajectory_execution(file_path="config/moveit_controllers.yaml")
        .planning_pipelines(pipelines=["ompl"])
        .pilz_cartesian_limits(file_path="config/pilz_cartesian_limits.yaml")
        .to_moveit_configs()
    )

    return generate_demo_launch(moveit_config)