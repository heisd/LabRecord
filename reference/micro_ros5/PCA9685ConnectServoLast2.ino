#include <micro_ros_arduino.h>
#include <Adafruit_NeoPixel.h>
#include <Adafruit_PWMServoDriver.h>
#include <WiFi.h>

#include <rcl/rcl.h>
#include <rcl/error_handling.h>
#include <rclc/rclc.h>
#include <rclc/executor.h>
#include <std_msgs/msg/float32_multi_array.h>

// ==================== 配置参数 ====================
// WiFi 配置
#define WIFI_SSID "WHEELTEC_S300"
#define WIFI_PASS "dongguan"
#define AGENT_IP "192.168.0.100"
#define AGENT_PORT 8888

// LED 配置
#define LED_PIN 38
#define NUM_LEDS 1

// I2C 引脚配置（ESP32-S3）
#define I2C_SDA 8
#define I2C_SCL 9

// 舵机配置
#define SERVO_COUNT 6
#define SERVO_FREQ 50
#define PCA9685_ADDRESS 0x40
#define SERVO_MIN_PULSE 150   // 0度对应的脉冲值
#define SERVO_MAX_PULSE 600   // 180度对应的脉冲值

// ==================== 全局对象 ====================
Adafruit_NeoPixel strip = Adafruit_NeoPixel(NUM_LEDS, LED_PIN, NEO_GRB + NEO_KHZ800);
Adafruit_PWMServoDriver pwm = Adafruit_PWMServoDriver(PCA9685_ADDRESS);

// micro-ROS 对象
rcl_subscription_t subscriber;
std_msgs__msg__Float32MultiArray msg;
rclc_executor_t executor;
rclc_support_t support;
rcl_allocator_t allocator;
rcl_node_t node;

// 当前舵机角度
float current_angles[SERVO_COUNT] = {90, 90, 90, 90, 90, 90};

// 系统状态
enum State {
  INIT,
  CONNECTING_WIFI,
  WIFI_CONNECTED,
  WAITING_AGENT,
  AGENT_CONNECTED,
  RUNNING,
  ERROR_STATE
} state;

unsigned long last_print = 0;
unsigned long message_count = 0;

// ==================== LED 状态显示 ====================
void setLED(uint8_t r, uint8_t g, uint8_t b) {
  strip.setPixelColor(0, strip.Color(r, g, b));
  strip.show();
}

void showState() {
  switch(state) {
    case INIT:
      setLED(50, 50, 50);  // 白色
      break;
    case CONNECTING_WIFI:
      setLED(128, 0, 128); // 紫色
      break;
    case WIFI_CONNECTED:
      setLED(0, 0, 255);   // 蓝色
      break;
    case WAITING_AGENT:
      setLED(255, 255, 0); // 黄色
      break;
    case AGENT_CONNECTED:
    case RUNNING:
      setLED(0, 255, 0);   // 绿色
      break;
    case ERROR_STATE:
      setLED(255, 0, 0);   // 红色
      break;
  }
}

// ==================== 舵机控制函数 ====================
uint16_t angleToPulse(float angle) {
  angle = constrain(angle, 0, 180);
  return map(angle * 100, 0, 18000, SERVO_MIN_PULSE, SERVO_MAX_PULSE);
}

void setServo(uint8_t num, float angle) {
  if (num >= SERVO_COUNT) {
    Serial.print("ERROR: Invalid servo number ");
    Serial.println(num);
    return;
  }
  
  uint16_t pulse = angleToPulse(angle);
  pwm.setPWM(num, 0, pulse);
  current_angles[num] = angle;
  
  Serial.print("  Servo ");
  Serial.print(num);
  Serial.print(" -> ");
  Serial.print(angle, 1);
  Serial.print(" deg (pulse: ");
  Serial.print(pulse);
  Serial.println(")");
}

// ==================== ROS 回调函数 ====================
void subscription_callback(const void * msgin) {
  const std_msgs__msg__Float32MultiArray * msg_in = 
    (const std_msgs__msg__Float32MultiArray *)msgin;
  
  message_count++;
  
  Serial.println("\n****************************************");
  Serial.print("*** MESSAGE RECEIVED #");
  Serial.print(message_count);
  Serial.println(" ***");
  Serial.println("****************************************");
  Serial.print("Data size: ");
  Serial.println(msg_in->data.size);
  
  // 青色闪烁表示收到消息
  setLED(0, 255, 255);
  
  if (msg_in->data.size >= SERVO_COUNT) {
    Serial.println("Setting servos:");
    for (int i = 0; i < SERVO_COUNT; i++) {
      setServo(i, msg_in->data.data[i]);
      delay(50);  // 小延迟，避免同时驱动太多舵机
    }
    Serial.println("All servos updated!");
  } else {
    Serial.print("ERROR: Expected ");
    Serial.print(SERVO_COUNT);
    Serial.print(" values, got ");
    Serial.println(msg_in->data.size);
  }
  
  Serial.println("****************************************\n");
  
  delay(200);
  showState();
}

// ==================== WiFi 连接 ====================
bool connectWiFi() {
  Serial.println("\n--- Connecting to WiFi ---");
  Serial.print("SSID: ");
  Serial.println(WIFI_SSID);
  
  WiFi.begin(WIFI_SSID, WIFI_PASS);
  
  int attempts = 0;
  while (WiFi.status() != WL_CONNECTED && attempts < 30) {
    setLED(128, 0, 128);
    delay(200);
    setLED(0, 0, 0);
    delay(200);
    Serial.print(".");
    attempts++;
  }
  Serial.println();
  
  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("WiFi connected!");
    Serial.print("IP address: ");
    Serial.println(WiFi.localIP());
    Serial.print("Signal strength: ");
    Serial.print(WiFi.RSSI());
    Serial.println(" dBm");
    return true;
  }
  
  Serial.println("WiFi connection FAILED!");
  return false;
}

// ==================== micro-ROS 初始化 ====================
bool createEntities() {
  Serial.println("\n--- Creating micro-ROS entities ---");
  
  allocator = rcl_get_default_allocator();
  
  Serial.print("1. Init support... ");
  if (rclc_support_init(&support, 0, NULL, &allocator) != RCL_RET_OK) {
    Serial.println("FAILED!");
    return false;
  }
  Serial.println("OK");

  Serial.print("2. Init node... ");
  if (rclc_node_init_default(&node, "servo_controller", "", &support) != RCL_RET_OK) {
    Serial.println("FAILED!");
    return false;
  }
  Serial.println("OK");

  Serial.print("3. Init subscription... ");
  if (rclc_subscription_init_default(
    &subscriber,
    &node,
    ROSIDL_GET_MSG_TYPE_SUPPORT(std_msgs, msg, Float32MultiArray),
    "servo_angles") != RCL_RET_OK) {
    Serial.println("FAILED!");
    return false;
  }
  Serial.println("OK");

  Serial.print("4. Init executor... ");
  if (rclc_executor_init(&executor, &support.context, 1, &allocator) != RCL_RET_OK) {
    Serial.println("FAILED!");
    return false;
  }
  Serial.println("OK");
  
  Serial.print("5. Add subscription... ");
  if (rclc_executor_add_subscription(
    &executor, &subscriber, &msg, 
    &subscription_callback, ON_NEW_DATA) != RCL_RET_OK) {
    Serial.println("FAILED!");
    return false;
  }
  Serial.println("OK");

  Serial.println("--- All entities created successfully! ---");
  Serial.println("Subscribed to topic: /servo_angles");
  Serial.println("Ready to receive commands!");
  return true;
}

void destroyEntities() {
  Serial.println("Destroying entities...");
  rcl_subscription_fini(&subscriber, &node);
  rclc_executor_fini(&executor);
  rcl_node_fini(&node);
  rclc_support_fini(&support);
}

// ==================== 硬件初始化 ====================
bool initHardware() {
  Serial.println("\n========================================");
  Serial.println("Hardware Initialization");
  Serial.println("========================================");
  
  // 初始化 LED
  Serial.print("1. Initializing LED... ");
  strip.begin();
  strip.show();
  Serial.println("OK");
  
  // 初始化 I2C
  Serial.print("2. Initializing I2C (SDA=");
  Serial.print(I2C_SDA);
  Serial.print(", SCL=");
  Serial.print(I2C_SCL);
  Serial.print(")... ");
  Wire.begin(I2C_SDA, I2C_SCL);
  Wire.setClock(100000);  // 100kHz
  delay(100);
  Serial.println("OK");
  
  // 扫描 I2C 设备
  Serial.println("3. Scanning I2C bus...");
  bool pca9685_found = false;
  int devices = 0;
  
  for(byte addr = 1; addr < 127; addr++) {
    Wire.beginTransmission(addr);
    if (Wire.endTransmission() == 0) {
      Serial.print("   Found device at 0x");
      if (addr < 16) Serial.print("0");
      Serial.println(addr, HEX);
      devices++;
      if (addr == 0x40) pca9685_found = true;
    }
  }
  
  Serial.print("   Total devices: ");
  Serial.println(devices);
  
  if (!pca9685_found) {
    Serial.println("\n*** ERROR: PCA9685 not found at 0x40! ***");
    Serial.println("Check wiring:");
    Serial.println("  PCA9685 VCC -> 3.3V or 5V");
    Serial.println("  PCA9685 GND -> GND");
    Serial.println("  PCA9685 SDA -> GPIO8");
    Serial.println("  PCA9685 SCL -> GPIO9");
    Serial.println("  PCA9685 V+  -> External 5V");
    return false;
  }
  
  // 初始化 PCA9685
  Serial.print("4. Initializing PCA9685... ");
  pwm.begin();
  pwm.setPWMFreq(SERVO_FREQ);
  delay(100);
  Serial.println("OK");
  
  // 设置舵机到初始位置
  Serial.println("5. Setting servos to home position (90°)...");
  for (int i = 0; i < SERVO_COUNT; i++) {
    setServo(i, 90);
    delay(100);
  }
  
  Serial.println("========================================");
  Serial.println("Hardware initialization complete!");
  Serial.println("========================================\n");
  
  return true;
}

// ==================== Setup ====================
void setup() {
  // 初始化串口
  Serial.begin(115200);
  delay(3000);
  
  Serial.println("\n\n\n");
  Serial.println("╔════════════════════════════════════════╗");
  Serial.println("║  ESP32-S3 Servo Controller v1.0       ║");
  Serial.println("║  6-Axis Servo with micro-ROS          ║");
  Serial.println("╚════════════════════════════════════════╝");
  Serial.println();
  
  // 硬件初始化
  state = INIT;
  showState();
  
  if (!initHardware()) {
    Serial.println("\n!!! Hardware initialization FAILED !!!");
    state = ERROR_STATE;
    showState();
    while(1) {
      delay(1000);
      Serial.println("Fix hardware and press RESET");
    }
  }
  
  // 连接 WiFi
  state = CONNECTING_WIFI;
  showState();
  
  if (!connectWiFi()) {
    state = ERROR_STATE;
    showState();
    while(1) {
      delay(1000);
      Serial.println("Fix WiFi and press RESET");
    }
  }
  
  state = WIFI_CONNECTED;
  showState();
  delay(500);
  
  // 配置 micro-ROS
  Serial.println("\n--- Configuring micro-ROS ---");
  Serial.print("Agent IP: ");
  Serial.println(AGENT_IP);
  Serial.print("Agent Port: ");
  Serial.println(AGENT_PORT);
  Serial.println("Calling set_microros_wifi_transports...");
  Serial.flush();
  
  set_microros_wifi_transports(WIFI_SSID, WIFI_PASS, AGENT_IP, AGENT_PORT);
  
  Serial.println("Transport configured!");
  Serial.flush();
  
  state = WAITING_AGENT;
  showState();
  
  Serial.println("\n========================================");
  Serial.println("Setup complete! Waiting for agent...");
  Serial.println("Start agent with:");
  Serial.println("  ros2 run micro_ros_agent micro_ros_agent udp4 --port 8888");
  Serial.println("========================================\n");
  Serial.flush();
  
  last_print = millis();
}

// ==================== Loop ====================
void loop() {
  // 检查 WiFi
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("\n!!! WiFi disconnected !!!");
    state = CONNECTING_WIFI;
    showState();
    connectWiFi();
    return;
  }
  
  switch(state) {
    case WAITING_AGENT:
      // 等待 micro-ROS Agent
      if (millis() - last_print > 2000) {
        Serial.println("Pinging agent...");
        last_print = millis();
      }
      
      if (RMW_RET_OK == rmw_uros_ping_agent(1000, 1)) {
        Serial.println("\n>>> Agent detected! <<<");
        if (createEntities()) {
          state = RUNNING;
          showState();
          Serial.println("\n*** SYSTEM READY ***");
          Serial.println("Waiting for commands on /servo_angles");
          Serial.println("Example:");
          Serial.println("  ros2 topic pub /servo_angles std_msgs/msg/Float32MultiArray");
          Serial.println("    \"{data: [90.0, 45.0, 135.0, 60.0, 120.0, 90.0]}\" --once\n");
        } else {
          Serial.println("Failed to create entities!");
          delay(2000);
        }
      }
      delay(100);
      break;

    case RUNNING:
      // 正常运行，处理消息
      if (RMW_RET_OK == rmw_uros_ping_agent(100, 1)) {
        rclc_executor_spin_some(&executor, RCL_MS_TO_NS(100));
      } else {
        Serial.println("\n!!! Agent lost !!!");
        destroyEntities();
        state = WAITING_AGENT;
        showState();
      }
      break;
  }
  
  delay(10);
}