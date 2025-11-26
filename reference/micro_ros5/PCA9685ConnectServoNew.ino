#include <micro_ros_arduino.h>
#include <Adafruit_NeoPixel.h>
#include <Adafruit_PWMServoDriver.h>
#include <WiFi.h>

#include <rcl/rcl.h>
#include <rcl/error_handling.h>
#include <rclc/rclc.h>
#include <rclc/executor.h>

#include <sensor_msgs/msg/joint_state.h>
#include <std_msgs/msg/float32_multi_array.h>

// WiFi 配置
#define WIFI_SSID "WHEELTEC_S300"
#define WIFI_PASS "dongguan"
#define AGENT_IP "192.168.0.100"
#define AGENT_PORT 8888

// LED 配置
#define LED_PIN 48
#define NUM_LEDS 1

// I2C 引脚配置 (ESP32-S3 默认引脚)
#define I2C_SDA 8   // GPIO8 - ESP32-S3 默认 SDA
#define I2C_SCL 9   // GPIO9 - ESP32-S3 默认 SCL

// 舵机配置
#define SERVO_COUNT 6
#define SERVO_FREQ 50  // 舵机频率 50Hz

// PCA9685 I2C 地址 (默认 0x40)
#define PCA9685_ADDRESS 0x40

// 舵机脉冲宽度范围 (根据你的舵机调整)
#define SERVO_MIN_PULSE 150   // 最小脉冲 (对应 0 度)
#define SERVO_MAX_PULSE 600   // 最大脉冲 (对应 180 度)

// 舵机角度限制
#define SERVO_MIN_ANGLE 0
#define SERVO_MAX_ANGLE 180

// micro-ROS 对象
rcl_subscription_t joint_sub;
rcl_subscription_t array_sub;
rcl_publisher_t joint_pub;

sensor_msgs__msg__JointState joint_msg;
sensor_msgs__msg__JointState joint_state_msg;
std_msgs__msg__Float32MultiArray array_msg;

rclc_executor_t executor;
rclc_support_t support;
rcl_allocator_t allocator;
rcl_node_t node;
rcl_timer_t timer;

// 对象
Adafruit_NeoPixel strip = Adafruit_NeoPixel(NUM_LEDS, LED_PIN, NEO_GRB + NEO_KHZ800);
Adafruit_PWMServoDriver pwm = Adafruit_PWMServoDriver(PCA9685_ADDRESS);

// 当前舵机角度
float current_angles[SERVO_COUNT] = {90, 90, 90, 90, 90, 90};

// 状态枚举
enum State {
  CONNECTING_WIFI,
  WAITING_AGENT,
  AGENT_AVAILABLE,
  AGENT_CONNECTED,
  AGENT_DISCONNECTED
} state;

// 显示状态
void showState() {
  switch(state) {
    case CONNECTING_WIFI:
      strip.setPixelColor(0, strip.Color(128, 0, 128));
      strip.show();
      break;
    case WAITING_AGENT:
      strip.setPixelColor(0, strip.Color(255, 255, 0));
      strip.show();
      break;
    case AGENT_AVAILABLE:
      strip.setPixelColor(0, strip.Color(0, 0, 255));
      strip.show();
      break;
    case AGENT_CONNECTED:
      strip.setPixelColor(0, strip.Color(0, 255, 0));
      strip.show();
      break;
    case AGENT_DISCONNECTED:
      strip.setPixelColor(0, strip.Color(255, 0, 0));
      strip.show();
      break;
  }
}

// 角度转换为脉冲宽度
uint16_t angleToPulse(float angle) {
  // 限制角度范围
  angle = constrain(angle, SERVO_MIN_ANGLE, SERVO_MAX_ANGLE);
  
  // 线性映射：0-180度 -> SERVO_MIN_PULSE - SERVO_MAX_PULSE
  uint16_t pulse = map(angle * 100, 0, 18000, SERVO_MIN_PULSE, SERVO_MAX_PULSE);
  return pulse;
}

// 设置单个舵机角度
void setServoAngle(uint8_t servo_num, float angle) {
  if (servo_num >= SERVO_COUNT) return;
  
  uint16_t pulse = angleToPulse(angle);
  pwm.setPWM(servo_num, 0, pulse);
  current_angles[servo_num] = angle;
  
  Serial.print("Servo ");
  Serial.print(servo_num);
  Serial.print(" -> ");
  Serial.print(angle);
  Serial.println(" degrees");
}

// 设置所有舵机角度
void setAllServos(float angles[SERVO_COUNT]) {
  for (int i = 0; i < SERVO_COUNT; i++) {
    setServoAngle(i, angles[i]);
  }
}

// 订阅回调 - JointState 消息 (标准 ROS 关节状态)
void joint_state_callback(const void * msgin) {
  const sensor_msgs__msg__JointState * msg = (const sensor_msgs__msg__JointState *)msgin;
  
  Serial.println("Received JointState message");
  
  // 使用 position 字段
  if (msg->position.size >= SERVO_COUNT) {
    for (int i = 0; i < SERVO_COUNT; i++) {
      float angle_rad = msg->position.data[i];
      float angle_deg = angle_rad * 180.0 / PI;  // 弧度转角度
      setServoAngle(i, angle_deg);
    }
  }
  
  // LED 闪烁表示接收到命令
  strip.setPixelColor(0, strip.Color(0, 255, 255));
  strip.show();
  delay(50);
  showState();
}

// 订阅回调 - Float32MultiArray 消息 (简单数组)
void array_callback(const void * msgin) {
  const std_msgs__msg__Float32MultiArray * msg = (const std_msgs__msg__Float32MultiArray *)msgin;
  
  Serial.println("========================================");
  Serial.println("Received Float32MultiArray message");
  Serial.print("Data size: ");
  Serial.println(msg->data.size);
  
  if (msg->data.size >= SERVO_COUNT) {
    for (int i = 0; i < SERVO_COUNT; i++) {
      Serial.print("Setting servo ");
      Serial.print(i);
      Serial.print(" to ");
      Serial.println(msg->data.data[i]);
      setServoAngle(i, msg->data.data[i]);
    }
  } else {
    Serial.print("WARNING: Expected ");
    Serial.print(SERVO_COUNT);
    Serial.print(" values, but received ");
    Serial.println(msg->data.size);
  }
  
  Serial.println("========================================");
  
  // LED 闪烁表示接收到命令
  strip.setPixelColor(0, strip.Color(0, 255, 255));
  strip.show();
  delay(50);
  showState();
}

// 定时器回调 - 发布当前关节状态
void timer_callback(rcl_timer_t * timer, int64_t last_call_time) {
  (void) last_call_time;
  if (timer != NULL) {
    // 更新关节状态消息
    for (int i = 0; i < SERVO_COUNT; i++) {
      joint_state_msg.position.data[i] = current_angles[i] * PI / 180.0;  // 角度转弧度
    }
    
    // 发布消息
    rcl_publish(&joint_pub, &joint_state_msg, NULL);
  }
}

// 连接WiFi
bool connectWiFi() {
  Serial.print("Connecting to WiFi ");
  Serial.println(WIFI_SSID);
  
  WiFi.begin(WIFI_SSID, WIFI_PASS);
  
  int attempts = 0;
  while (WiFi.status() != WL_CONNECTED && attempts < 30) {
    strip.setPixelColor(0, strip.Color(128, 0, 128));
    strip.show();
    delay(200);
    strip.setPixelColor(0, strip.Color(0, 0, 0));
    strip.show();
    delay(200);
    Serial.print(".");
    attempts++;
  }
  
  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("\nWiFi connected!");
    Serial.print("IP address: ");
    Serial.println(WiFi.localIP());
    return true;
  }
  return false;
}

// 初始化 JointState 消息
void init_joint_state_msg() {
  // 分配内存
  joint_state_msg.name.capacity = SERVO_COUNT;
  joint_state_msg.name.size = SERVO_COUNT;
  joint_state_msg.name.data = (rosidl_runtime_c__String*) malloc(SERVO_COUNT * sizeof(rosidl_runtime_c__String));
  
  joint_state_msg.position.capacity = SERVO_COUNT;
  joint_state_msg.position.size = SERVO_COUNT;
  joint_state_msg.position.data = (double*) malloc(SERVO_COUNT * sizeof(double));
  
  joint_state_msg.velocity.capacity = 0;
  joint_state_msg.velocity.size = 0;
  
  joint_state_msg.effort.capacity = 0;
  joint_state_msg.effort.size = 0;
  
  // 设置关节名称 - 使用静态字符串
  static char joint_name_0[] = "joint_0";
  static char joint_name_1[] = "joint_1";
  static char joint_name_2[] = "joint_2";
  static char joint_name_3[] = "joint_3";
  static char joint_name_4[] = "joint_4";
  static char joint_name_5[] = "joint_5";
  
  joint_state_msg.name.data[0].data = joint_name_0;
  joint_state_msg.name.data[0].size = strlen(joint_name_0);
  joint_state_msg.name.data[0].capacity = strlen(joint_name_0) + 1;
  
  joint_state_msg.name.data[1].data = joint_name_1;
  joint_state_msg.name.data[1].size = strlen(joint_name_1);
  joint_state_msg.name.data[1].capacity = strlen(joint_name_1) + 1;
  
  joint_state_msg.name.data[2].data = joint_name_2;
  joint_state_msg.name.data[2].size = strlen(joint_name_2);
  joint_state_msg.name.data[2].capacity = strlen(joint_name_2) + 1;
  
  joint_state_msg.name.data[3].data = joint_name_3;
  joint_state_msg.name.data[3].size = strlen(joint_name_3);
  joint_state_msg.name.data[3].capacity = strlen(joint_name_3) + 1;
  
  joint_state_msg.name.data[4].data = joint_name_4;
  joint_state_msg.name.data[4].size = strlen(joint_name_4);
  joint_state_msg.name.data[4].capacity = strlen(joint_name_4) + 1;
  
  joint_state_msg.name.data[5].data = joint_name_5;
  joint_state_msg.name.data[5].size = strlen(joint_name_5);
  joint_state_msg.name.data[5].capacity = strlen(joint_name_5) + 1;
  
  // 初始化位置为 0
  for (int i = 0; i < SERVO_COUNT; i++) {
    joint_state_msg.position.data[i] = 0.0;
  }
}

// 创建 micro-ROS 实体
bool createEntities() {
  allocator = rcl_get_default_allocator();
  
  if (rclc_support_init(&support, 0, NULL, &allocator) != RCL_RET_OK) {
    return false;
  }

  if (rclc_node_init_default(&node, "servo_controller_node", "", &support) != RCL_RET_OK) {
    return false;
  }

  // 创建订阅者 - JointState
  if (rclc_subscription_init_default(
    &joint_sub,
    &node,
    ROSIDL_GET_MSG_TYPE_SUPPORT(sensor_msgs, msg, JointState),
    "joint_commands") != RCL_RET_OK) {
    return false;
  }

  // 创建订阅者 - Float32MultiArray
  if (rclc_subscription_init_default(
    &array_sub,
    &node,
    ROSIDL_GET_MSG_TYPE_SUPPORT(std_msgs, msg, Float32MultiArray),
    "servo_angles") != RCL_RET_OK) {
    return false;
  }

  // 创建发布者 - 发布当前关节状态
  if (rclc_publisher_init_default(
    &joint_pub,
    &node,
    ROSIDL_GET_MSG_TYPE_SUPPORT(sensor_msgs, msg, JointState),
    "joint_states") != RCL_RET_OK) {
    return false;
  }

  // 创建定时器 - 每 100ms 发布一次状态
  if (rclc_timer_init_default(
    &timer,
    &support,
    RCL_MS_TO_NS(100),
    timer_callback) != RCL_RET_OK) {
    return false;
  }

  // 创建执行器
  if (rclc_executor_init(&executor, &support.context, 3, &allocator) != RCL_RET_OK) {
    return false;
  }
  
  if (rclc_executor_add_subscription(&executor, &joint_sub, &joint_msg, &joint_state_callback, ON_NEW_DATA) != RCL_RET_OK) {
    return false;
  }
  
  if (rclc_executor_add_subscription(&executor, &array_sub, &array_msg, &array_callback, ON_NEW_DATA) != RCL_RET_OK) {
    return false;
  }

  if (rclc_executor_add_timer(&executor, &timer) != RCL_RET_OK) {
    return false;
  }

  Serial.println("micro-ROS entities created successfully");
  return true;
}

void destroyEntities() {
  rmw_context_t * rmw_context = rcl_context_get_rmw_context(&support.context);
  (void) rmw_uros_set_context_entity_destroy_session_timeout(rmw_context, 0);

  rcl_subscription_fini(&joint_sub, &node);
  rcl_subscription_fini(&array_sub, &node);
  rcl_publisher_fini(&joint_pub, &node);
  rcl_timer_fini(&timer);
  rclc_executor_fini(&executor);
  rcl_node_fini(&node);
  rclc_support_fini(&support);
}

void setup() {
  Serial.begin(115200);  
  delay(2000);
  
  Serial.println("Starting 6-Axis Servo Controller");
  
  // 初始化 LED
  strip.begin();
  strip.show();
  
  // 初始化 I2C
  Serial.println("Initializing I2C...");
  Wire.begin(I2C_SDA, I2C_SCL);
  
  // 扫描 I2C 设备
  Serial.println("Scanning I2C bus...");
  byte error, address;
  int nDevices = 0;
  
  for(address = 1; address < 127; address++) {
    Wire.beginTransmission(address);
    error = Wire.endTransmission();
    
    if (error == 0) {
      Serial.print("I2C device found at address 0x");
      if (address < 16) Serial.print("0");
      Serial.print(address, HEX);
      Serial.println(" !");
      nDevices++;
    }
  }
  
  if (nDevices == 0) {
    Serial.println("ERROR: No I2C devices found!");
    Serial.println("Check your wiring:");
    Serial.print("  SDA -> GPIO");
    Serial.println(I2C_SDA);
    Serial.print("  SCL -> GPIO");
    Serial.println(I2C_SCL);
    // 红色快闪表示 I2C 错误
    while(1) {
      strip.setPixelColor(0, strip.Color(255, 0, 0));
      strip.show();
      delay(100);
      strip.setPixelColor(0, strip.Color(0, 0, 0));
      strip.show();
      delay(100);
    }
  } else {
    Serial.print("Found ");
    Serial.print(nDevices);
    Serial.println(" I2C device(s)");
  }
  
  // 初始化 PCA9685
  Serial.println("Initializing PCA9685...");
  pwm.begin();
  pwm.setPWMFreq(SERVO_FREQ);
  delay(100);
  
  // 设置所有舵机到中间位置
  Serial.println("Setting servos to home position (90 degrees)");
  for (int i = 0; i < SERVO_COUNT; i++) {
    setServoAngle(i, 90);
    delay(100);
  }
  
  // 连接WiFi
  state = CONNECTING_WIFI;
  if (!connectWiFi()) {
    while(1) {
      strip.setPixelColor(0, strip.Color(255, 0, 0));
      strip.show();
      delay(100);
      strip.setPixelColor(0, strip.Color(0, 0, 0));
      strip.show();
      delay(100);
    }
  }
  
  // 设置 micro-ROS
  set_microros_wifi_transports(WIFI_SSID, WIFI_PASS, AGENT_IP, AGENT_PORT);
  
  // 初始化消息
  init_joint_state_msg();
  
  state = WAITING_AGENT;
  showState();
}

void loop() {
  if (WiFi.status() != WL_CONNECTED) {
    state = CONNECTING_WIFI;
    connectWiFi();
    return;
  }
  
  switch(state) {
    case WAITING_AGENT:
      if (RMW_RET_OK == rmw_uros_ping_agent(1000, 1)) {
        state = AGENT_AVAILABLE;
        showState();
      }
      delay(500);
      break;

    case AGENT_AVAILABLE:
      if (createEntities()) {
        state = AGENT_CONNECTED;
        showState();
      } else {
        state = WAITING_AGENT;
        showState();
        delay(1000);
      }
      break;

    case AGENT_CONNECTED:
      if (RMW_RET_OK == rmw_uros_ping_agent(100, 1)) {
        rclc_executor_spin_some(&executor, RCL_MS_TO_NS(100));
      } else {
        state = AGENT_DISCONNECTED;
        showState();
      }
      break;

    case AGENT_DISCONNECTED:
      destroyEntities();
      state = WAITING_AGENT;
      showState();
      delay(1000);
      break;
  }
  
  delay(10);
}