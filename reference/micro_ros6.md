# 上次不知道为啥在上位机上发信号机械臂并没有和我们想的一样做出相应的变化
下面我准备从两个方向上对这个进行修改
## 1.检查 ESP32-PCA9865 模块之间的通信
测试每一个轴现在对应的是
0->2
1->5
2->0
3->4
4->3
5->1
为了使他与我们的编码符合我们就使用每一个舵机和编号都接回去了</br>
这个线接错真不能怪学长</br>
修改完成接线之后</br>
同时也证明了我们的 ESP32-PCA9865 通信模块没有问题
## 但是经过我检测我感觉 Jetson 到 esp32 的通信也没有问题呀
```bash
ros2 topic list
```
这里有这个 /servo_angles这个话题呀
并且使用
```bash
ros2 topic echo /servo_angles
```
<font color="blue">dxf@ubuntu:~/Desktop/li$ ros2 topic echo /servo_angles
</br>
layout:
  dim: []</br>
  data_offset: 0</br>
data:
- 90.0
- 90.0
- 90.0
- 90.0
- 120.0
- 90.0</br>
---</font>
## Claude告诉我是回调函数部分出现了问题
你在 `ros2 topic echo` 能看到数据，说明 ROS2 端正常，但 ESP32 的回调函数没有被触发。
- 上传测试版本[test](./TestCallBack/TestCallBack.ino)
发现串口并没有和我们想的一样有输出
- 当你用 ros2 topic pub 发布时，它使用的默认 QoS 可能与 ESP32 的不匹配,使用兼容的Qos设置[Qostest](./TestCallBack/QosTestCallBack.ino),或者选择在发布消息的是使用下面的命令

```bash
# 使用 best effort QoS 发布
ros2 topic pub /servo_angles std_msgs/msg/Float32MultiArray \
  "{data: [90,45,135,60,120,90]}" \
  --qos-reliability best_effort
```
# 原因出现在信息没有分配内存上
初始化信息的时候要使用静态缓冲区
```cpp
static float data_buffer[6];  // 静态缓冲区
msg.data.data = data_buffer;
msg.data.size = 0;
msg.data.capacity = 6;

```
修改之后发现就可以正常运行了
还是因为我们的信息类型初始化未给分配内存，导致了信息不能被实体化来创建
最后的代码存在了[OK](./micro_ros6/Arduino/ESP32OK.ino)
直接使用这个代码+我们的串口调试就可以了
并且在这个代码中我还加入了死区，为了防止过小的信号干扰，从而是舵机不那么稳定










