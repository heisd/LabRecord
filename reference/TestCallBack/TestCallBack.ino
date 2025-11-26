#include <micro_ros_arduino.h>
#include <Adafruit_PWMServoDriver.h>
#include <WiFi.h>
#include <rcl/rcl.h>
#include <rclc/rclc.h>
#include <rclc/executor.h>
#include <std_msgs/msg/float32_multi_array.h>

#define WIFI_SSID "WHEELTEC_S300"
#define WIFI_PASS "dongguan"
#define AGENT_IP "192.168.0.100"
#define AGENT_PORT 8888
#define I2C_SDA 8
#define I2C_SCL 9

Adafruit_PWMServoDriver pwm = Adafruit_PWMServoDriver(0x40);
rcl_subscription_t sub;
std_msgs__msg__Float32MultiArray msg;
rclc_executor_t executor;
rclc_support_t support;
rcl_allocator_t allocator;
rcl_node_t node;

unsigned long count = 0;

void setServo(uint8_t n, float angle) {
  angle = constrain(angle, 0, 180);
  uint16_t pulse = map(angle * 100, 0, 18000, 150, 600);
  pwm.setPWM(n, 0, pulse);
  Serial.print("S");
  Serial.print(n);
  Serial.print("=");
  Serial.print(angle, 0);
  Serial.print(" ");
}

void callback(const void * msgin) {
  const std_msgs__msg__Float32MultiArray * m = (const std_msgs__msg__Float32MultiArray *)msgin;
  
  count++;
  Serial.print("\n>>> MSG #");
  Serial.print(count);
  Serial.println(" <<<");
  
  if (m->data.size >= 6) {
    for (int i = 0; i < 6; i++) {
      setServo(i, m->data.data[i]);
    }
    Serial.println("\nOK!");
  }
}

void setup() {
  Serial.begin(115200);
  delay(2000);
  Serial.println("\n=== START ===");
  
  // 硬件
  Wire.begin(I2C_SDA, I2C_SCL);
  pwm.begin();
  pwm.setPWMFreq(50);
  Serial.println("HW OK");
  
  // WiFi
  WiFi.begin(WIFI_SSID, WIFI_PASS);
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print(".");
    delay(500);
  }
  Serial.print("\nWiFi: ");
  Serial.println(WiFi.localIP());
  
  // micro-ROS
  set_microros_wifi_transports(WIFI_SSID, WIFI_PASS, AGENT_IP, AGENT_PORT);
  
  Serial.print("Ping");
  while (RMW_RET_OK != rmw_uros_ping_agent(1000, 1)) {
    Serial.print(".");
    delay(500);
  }
  Serial.println(" OK");
  
  allocator = rcl_get_default_allocator();
  
  msg.data.data = NULL;
  msg.data.size = 0;
  msg.data.capacity = 0;
  
  rclc_support_init(&support, 0, NULL, &allocator);
  rclc_node_init_default(&node, "servo", "", &support);
  rclc_subscription_init_default(&sub, &node,
    ROSIDL_GET_MSG_TYPE_SUPPORT(std_msgs, msg, Float32MultiArray), "servo_angles");
  rclc_executor_init(&executor, &support.context, 1, &allocator);
  rclc_executor_add_subscription(&executor, &sub, &msg, &callback, ON_NEW_DATA);
  
  Serial.println("*** READY ***");
  Serial.println("ros2 topic pub /servo_angles std_msgs/msg/Float32MultiArray \"{data: [90,90,90,90,90,90]}\" --once");
}

void loop() {
  rclc_executor_spin_some(&executor, RCL_MS_TO_NS(100));
  delay(10);
}