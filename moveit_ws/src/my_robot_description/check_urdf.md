# 我们通过SolidWorks导入的urdf文件不知道是否正确
```bash
# 安装必要工具
sudo apt install liburdfdom-tools
# 检查urdf文件
dxf@ubuntu:~/Desktop/li/moveit_ws/src/my_robot_description/urdf$ check_urdf my_robot_description.urdf
robot name is: my_robot_description
---------- Successfully Parsed XML ---------------
root Link: base_link has 1 child(ren)
    child(1):  link_1
        child(1):  link_2
            child(1):  link_3
                child(1):  Link_4
                    child(1):  grab
```
证明我们的urdf文件没有问题

