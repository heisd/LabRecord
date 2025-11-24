#!/usr/bin/env python3
import rclpy
from rclpy.node import Node
from std_msgs.msg import Int32, String
import threading
import time

class HostBridge(Node):
    def __init__(self):
        super().__init__('host_bridge')
        
        # 订阅来自 ESP32 的消息
        self.esp32_sub = self.create_subscription(
            Int32,
            'esp32_to_host',
            self.esp32_callback,
            10)
        
        # 发布到 ESP32 的消息
        self.host_pub = self.create_publisher(
            String,
            'host_to_esp32',
            10)
        
        # 启动发送线程
        self.send_thread = threading.Thread(target=self.send_commands)
        self.send_thread.daemon = True
        self.send_thread.start()
        
        self.get_logger().info('主机桥接节点已启动')
    
    def esp32_callback(self, msg):
        # 处理来自 ESP32 的数据
        self.get_logger().info(f'收到 ESP32 传感器数据: {msg.data}')
        
        # 根据数据做出响应（示例）
        if msg.data > 2000:
            response_msg = String()
            response_msg.data = "数据过高!"
            self.host_pub.publish(response_msg)
    
    def send_commands(self):
        """定期发送命令到 ESP32"""
        command_count = 0
        while rclpy.ok():
            time.sleep(5)  # 每5秒发送一次
            
            msg = String()
            if command_count % 2 == 0:
                msg.data = "LED_ON"
            else:
                msg.data = "LED_OFF"
            
            self.host_pub.publish(msg)
            self.get_logger().info(f'发送命令到 ESP32: {msg.data}')
            command_count += 1

def main():
    rclpy.init()
    node = HostBridge()
    
    try:
        rclpy.spin(node)
    except KeyboardInterrupt:
        pass
    finally:
        node.destroy_node()
        rclpy.shutdown()

if __name__ == '__main__':
    main()
