# 在 Arduino使得ESP32的灯亮起来
## 1.在Arduino里面安装这个库Adafruit NeoPixel
## 2.负责下面的代码ArduinoRGB.cpp里面
## 3.下载进入ESP32的结果如下面的视频所示
<video src="./videos/esp32_rgb_led.mp4" autoplay="true" controls="controls" width="800" height="600">
</video>

# 下面通过micro_ros来控制我们的小灯
## 1.克隆我们的ArduinoRGBMicroROSWIFI.cpp
## 2.下载进入ESP32中使用Arduino IDE
## 3.在我们的ROS端口进入
```bash
cd ~/Desktop/li/micro_ros_ws
ros2 run micro_ros_agent micro_ros_agent udp4 --port 8888
```
在li目录下
## 4.在我们的ROS端口进入
```bash
cd ~/Desktop/li/micro_ros_ws
# 新建一个终端在这个上面运行发布数据的命令,下面是常见的颜色例子
# 红色
ros2 topic pub /led_color std_msgs/msg/ColorRGBA "{r: 1.0, g: 0.0, b: 0.0, a: 1.0}"

# 绿色
ros2 topic pub /led_color std_msgs/msg/ColorRGBA "{r: 0.0, g: 1.0, b: 0.0, a: 1.0}"

# 蓝色
ros2 topic pub /led_color std_msgs/msg/ColorRGBA "{r: 0.0, g: 0.0, b: 1.0, a: 1.0}"

# 紫色
ros2 topic pub /led_color std_msgs/msg/ColorRGBA "{r: 0.5, g: 0.0, b: 0.5, a: 1.0}"
```













