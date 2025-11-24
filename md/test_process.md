# 启动agent
# 串口方式
ros2 run micro_ros_agent micro_ros_agent serial --dev /dev/ttyUSB0 -b 115200

# 或 WiFi 方式
ros2 run micro_ros_agent micro_ros_agent udp4 --port 8888

python3 host_bridge_node.py

# 查看所有话题
ros2 topic list

# 手动发送命令到 ESP32
ros2 topic pub /host_to_esp32 std_msgs/msg/String "data: 'LED_ON'"

# 查看 ESP32 发送的数据
ros2 topic echo /esp32_to_host
