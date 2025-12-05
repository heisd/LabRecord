#include <micro_ros_arduino.h>
#include <Adafruit_PWMServoDriver.h>
#include <WiFi.h>

#include <rcl/rcl.h>
#include <rclc/rclc.h>
#include <rclc/executor.h>
#include <std_msgs/msg/float32_multi_array.h>

// 配置 - 修改这里！
#define WIFI_SSID "WHEELTEC_S300"
#define WIFI_PASS "dongguan"
#define AGENT_IP "192.168.0.100"  // ← 改成你电脑的实际 IP！
#define AGENT_PORT 8888

#define I2C_SDA 8
#define I2C_SCL 9

Adafruit_PWMServoDriver pwm = Adafruit_PWMServoDriver(0x40);

rcl_subscription_t subscriber;
std_msgs__msg__Float32MultiArray msg;
rclc_executor_t executor;
rclc_support_t support;
rcl_allocator_t allocator;
rcl_node_t node;

unsigned long msg_count = 0;

// 舵机控制
void setServo(uint8_t num, float angle) {
  // 增加死区保护，如果角度变化很小不发送指令
  static float last_angles[16] = {0};
 if (abs(last_angles[num] - angle) < 1.0) {
    return; // 变化太小，忽略
  }
  last_angles[num]=angle;

  angle = constrain(angle, 0, 180);
  // 根据数据手册调整对应脉宽
  uint16_t pulse = map(angle * 100, 0, 18000, 150, 600);
  pwm.setPWM(num, 0, pulse);
  
  Serial.print("  Servo ");
  Serial.print(num);
  Serial.print(" -> ");
  Serial.print(angle, 1);
  Serial.println("°");
}

// ROS 回调
void callback(const void * msgin) {
    // 观测是否进行ROS回调
    Serial.println("Callback called");
  const std_msgs__msg__Float32MultiArray * m = (const std_msgs__msg__Float32MultiArray *)msgin;
  
  msg_count++;
  
  Serial.println("\n★★★★★★★★★★★★★★★★★★★★★★★★★★");
  Serial.print("★★★ MESSAGE #");
  Serial.print(msg_count);
  Serial.println(" RECEIVED ★★★");
  Serial.println("★★★★★★★★★★★★★★★★★★★★★★★★★★");
  
  if (m->data.size >= 6) {
    Serial.println("Setting servos:");
    for (int i = 0; i < 6; i++) {
      setServo(i, m->data.data[i]);
      Serial.print(m->data.data[i] );
    }
    Serial.println("Done!");
  }
  
  Serial.println("★★★★★★★★★★★★★★★★★★★★★★★★★★\n");
}

void setup() {
  Serial.begin(115200);
  delay(3000);
  
  Serial.println("\n\n\n");
  Serial.println("========================================");
  Serial.println("  micro-ROS Servo Controller DEBUG");
  Serial.println("========================================\n");
  
  // 硬件初始化
  Serial.println(">>> Initializing Hardware <<<");
  Wire.begin(I2C_SDA, I2C_SCL);
  pwm.begin();
  pwm.setPWMFreq(50);
  delay(100);
  
  // 简单测试：所有舵机归中
  Serial.println("Setting all servos to 90°...");
  for(int i = 0; i < 6; i++) {
    setServo(i, 90);
    delay(100);
  }
  Serial.println("Hardware OK!\n");
  
  // WiFi
  Serial.println(">>> Connecting to WiFi <<<");
  Serial.print("SSID: ");
  Serial.println(WIFI_SSID);
  WiFi.begin(WIFI_SSID, WIFI_PASS);
  
  int attempts = 0;
  while (WiFi.status() != WL_CONNECTED && attempts < 30) {
    Serial.print(".");
    delay(500);
    attempts++;
  }
  
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("\nWiFi FAILED!");
    while(1) delay(1000);
  }
  
  Serial.println("\nWiFi OK!");
  Serial.print("ESP32 IP: ");
  Serial.println(WiFi.localIP());
  Serial.print("Agent IP: ");
  Serial.println(AGENT_IP);
  Serial.print("Agent Port: ");
  Serial.println(AGENT_PORT);
  Serial.println();
  
  // micro-ROS
  Serial.println(">>> Configuring micro-ROS <<<");
  Serial.flush();
  
  set_microros_wifi_transports(WIFI_SSID, WIFI_PASS, AGENT_IP, AGENT_PORT);
  
  Serial.println("Transport configured!");
  Serial.flush();
  
  delay(1000);
  
  Serial.println("\n>>> Waiting for Agent <<<");
  Serial.println("Make sure agent is running:");
  Serial.println("  ros2 run micro_ros_agent micro_ros_agent udp4 --port 8888\n");
  
  // 等待 agent
  int ping_attempts = 0;
  while(true) {
    Serial.print("Ping attempt ");
    Serial.print(++ping_attempts);
    Serial.print("... ");
    Serial.flush();
    
    if (RMW_RET_OK == rmw_uros_ping_agent(1000, 1)) {
      Serial.println("SUCCESS!");
      break;
    } else {
      Serial.println("no response");
    }
    
    delay(1000);
    
    if (ping_attempts >= 10) {
      Serial.println("\n!!! Agent not found after 10 attempts !!!");
      Serial.println("Check:");
      Serial.println("  1. Is agent running?");
      Serial.println("  2. Is AGENT_IP correct?");
      Serial.println("  3. Is firewall blocking port 8888?");
      while(1) delay(1000);
    }
  }
  
  Serial.println("\n>>> Creating Entities <<<");
  
  allocator = rcl_get_default_allocator();
  

  // 初始化消息 - 必须分配内存！
  static float data_buffer[6];  // 静态缓冲区
  msg.data.data = data_buffer;
  msg.data.size = 0;
  msg.data.capacity = 6;

  msg.layout.dim.data = NULL;
  msg.layout.dim.size = 0;
  msg.layout.dim.capacity = 0;
  msg.layout.data_offset = 0;
  
  if (rclc_support_init(&support, 0, NULL, &allocator) != RCL_RET_OK) {
    Serial.println("FAILED: support_init");
    while(1) delay(1000);
  }
  Serial.println("✓ Support init");

  if (rclc_node_init_default(&node, "servo_node", "", &support) != RCL_RET_OK) {
    Serial.println("FAILED: node_init");
    while(1) delay(1000);
  }
  Serial.println("✓ Node init");

  if (rclc_subscription_init_default(&subscriber, &node,
      ROSIDL_GET_MSG_TYPE_SUPPORT(std_msgs, msg, Float32MultiArray),
      "servo_angles") != RCL_RET_OK) {
    Serial.println("FAILED: subscription_init");
    while(1) delay(1000);
  }
  Serial.println("✓ Subscription init");

  if (rclc_executor_init(&executor, &support.context, 1, &allocator) != RCL_RET_OK) {
    Serial.println("FAILED: executor_init");
    while(1) delay(1000);
  }
  Serial.println("✓ Executor init");
  
  if (rclc_executor_add_subscription(&executor, &subscriber, &msg, &callback, ON_NEW_DATA) != RCL_RET_OK) {
    Serial.println("FAILED: add_subscription");
    while(1) delay(1000);
  }
  Serial.println("✓ Subscription added");

  Serial.println("\n========================================");
  Serial.println("  ★★★ SYSTEM READY ★★★");
  Serial.println("========================================");
  Serial.println("Subscribed to: /servo_angles");
  Serial.println("\nSend commands:");
  Serial.println("  ros2 topic pub /servo_angles std_msgs/msg/Float32MultiArray \\");
  Serial.println("    \"{data: [90.0, 45.0, 135.0, 60.0, 120.0, 90.0]}\" --once");
  Serial.println("\nWaiting for messages...\n");
}

void loop() {
  static unsigned long last_check = 0;
  
  // 每秒检查一次 Agent 连接
  if (millis() - last_check > 1000) {
    if (RMW_RET_OK != rmw_uros_ping_agent(100, 1)) {
      Serial.println("\n!!! Agent connection lost !!!");
      Serial.println("Reconnecting...");
      
      // 销毁旧实体
      rcl_subscription_fini(&subscriber, &node);
      rclc_executor_fini(&executor);
      rcl_node_fini(&node);
      rclc_support_fini(&support);
      
      delay(1000);
      
      // 重新连接
      setup();
      return;
    }
    last_check = millis();
  }
  
  // 处理消息
  rclc_executor_spin_some(&executor, RCL_MS_TO_NS(100));
  delay(10);
}