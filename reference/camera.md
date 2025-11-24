# 1.问题出现:我们运行color_grab发现传入的深度值是0
# 问题解决思路:
## 1.检查硬件连线问题
    检查之后发现没有问题
## 2.原因
    检查是因为距离太近了,导致深度值为0
# 2.问题出现：我们发现机械臂在运行颜色抓取的时候没有运动
    ros2 launch grab_demo color_grab.launch.py
    ros2 run grab_demo start_grab color_link
    发现并没有机械臂并没有运动
## 2.原因
    在hsv_range.cpp中的父坐标系不应该是                camera_color_optical_frame，而是camera_arm_color_optical_frame
# 3.问题出现：发现机械臂运行颜色抓取的情况下并没有到达我们预期的位置
## 原因：可能是位姿变化和 tf 坐标变换不正确



    





    
    



