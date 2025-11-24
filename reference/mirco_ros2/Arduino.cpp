#include <micro_ros_arduino.h>
#include <rcl/rcl.h>
#include <rcl/error_handling.h>
#include <rclc/rclc.h>
#include <rclc/executor.h>
#include <std_msgs/msg/int32.h>
#include <std_msgs/msg/string.h>

// micro-ROS 对象
rcl_node_t node;
rcl_publisher_t publisher;
rcl_subscription_t subscriber;
std_msgs__msg__Int32 pub_msg;
std_msgs__msg__String sub_msg;
rclc_executor_t executor;
rcl_allocator_t allocator;
rclc_support_t support;
// 将timer移动到全局作用域
rclc_timer_t timer;

// 话题名称
const char *pub_topic = "esp32_to_host";    // ESP32 → 主机
const char *sub_topic = "host_to_esp32";    // 主机 → ESP32

// 订阅者回调函数（接收来自主机的消息）
void subscription_callback(const void *msgin) {
  const std_msgs__msg__String *msg = (const std_msgs__msg__String *)msgin;
  
  Serial.print("收到主机消息: ");
  Serial.println(msg->data.data);
  
  // 根据接收到的消息执行相应操作
  if (strstr(msg->data.data, "LED_ON") != NULL) {
    digitalWrite(LED_BUILTIN, HIGH);
    Serial.println("LED 已打开");
  } else if (strstr(msg->data.data, "LED_OFF") != NULL) {
    digitalWrite(LED_BUILTIN, LOW);
    Serial.println("LED 已关闭");
  }
}

// 定时器回调函数（定期向主机发送数据）
void timer_callback(rcl_timer_t *timer, int64_t last_call_time) {
  (void)last_call_time;
  if (timer != NULL) {
    // 发送传感器数据（示例：模拟读取）
    //
    pub_msg.data = analogRead(A0);
    
    // 发布消息
    rcl_ret_t ret = rcl_publish(&publisher, &pub_msg, NULL);
    if (ret == RCL_RET_OK) {
      Serial.print("发送数据到主机: ");
      Serial.println(pub_msg.data);
    }
  }
}

void setup() {
  // 初始化串口和 LED
  Serial.begin(115200);
  pinMode(LED_BUILTIN, OUTPUT);
  
  // 设置 micro-ROS 传输方式（WiFi）
  set_microros_wifi_transports("WHEELTEC_S300", "dongguan", "192.168.0.100", 8888);
  
  delay(2000);  // 等待连接建立

  // 初始化 micro-ROS
  allocator = rcl_get_default_allocator();
  rclc_support_init(&support, 0, NULL, &allocator);
  
  // 创建节点
  rclc_node_init_default(&node, "esp32_node", "", &support);
  
  // 创建发布者（ESP32 → 主机）
  rclc_publisher_init_default(
    &publisher,
    &node,
    ROSIDL_GET_MSG_TYPE_SUPPORT(std_msgs, msg, Int32),
    pub_topic);
  
  // 创建订阅者（主机 → ESP32）
  rclc_subscription_init_default(
    &subscriber,
    &node,
    ROSIDL_GET_MSG_TYPE_SUPPORT(std_msgs, msg, String),
    sub_topic);
  
  // 创建执行器
  rclc_executor_init(&executor, &support.context, 2, &allocator);
  
  // 添加订阅者到执行器
  rclc_executor_add_subscription(&executor, &subscriber, &sub_msg, &subscription_callback, ON_NEW_DATA);
  
  // 创建定时器（每2秒发送一次数据）
  rcl_timer_t timer;
  rclc_timer_init_default(&timer, &support, RCL_MS_TO_NS(2000), timer_callback);
  rclc_executor_add_timer(&executor, &timer);
  
  // 初始化消息
  pub_msg.data = 0;
  sub_msg.data.data = (char*)malloc(100);
  sub_msg.data.size = 0;
  sub_msg.data.capacity = 100;
  
  Serial.println("ESP32 micro-ROS 双向通信初始化完成!");
}

void loop() {
  // 处理 micro-ROS 通信
  rclc_executor_spin_some(&executor, RCL_MS_TO_NS(100));
  delay(10);
}
