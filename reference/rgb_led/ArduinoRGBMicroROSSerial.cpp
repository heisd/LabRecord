#include <micro_ros_arduino.h>
#include <Adafruit_NeoPixel.h>

#include <rcl/rcl.h>
#include <rcl/error_handling.h>
#include <rclc/rclc.h>
#include <rclc/executor.h>

#include <std_msgs/msg/color_rgba.h>

// LED 配置
#define LED_PIN 48
#define NUM_LEDS 1

// micro-ROS 对象
rcl_subscription_t subscriber;
std_msgs__msg__ColorRGBA msg;
rclc_executor_t executor;
rclc_support_t support;
rcl_allocator_t allocator;
rcl_node_t node;

// NeoPixel 对象
Adafruit_NeoPixel strip = Adafruit_NeoPixel(NUM_LEDS, LED_PIN, NEO_GRB + NEO_KHZ800);

// 错误处理宏
#define RCCHECK(fn) { rcl_ret_t temp_rc = fn; if((temp_rc != RCL_RET_OK)){error_loop();}}
#define RCSOFTCHECK(fn) { rcl_ret_t temp_rc = fn; if((temp_rc != RCL_RET_OK)){}}

// 错误处理函数
void error_loop(){
  while(1){
    strip.setPixelColor(0, strip.Color(255, 0, 0)); // 红色表示错误
    strip.show();
    delay(100);
    strip.setPixelColor(0, strip.Color(0, 0, 0)); // 关闭
    strip.show();
    delay(100);
  }
}

// 订阅回调函数 - 接收颜色消息并控制LED
void subscription_callback(const void * msgin) {  
  const std_msgs__msg__ColorRGBA * msg = (const std_msgs__msg__ColorRGBA *)msgin;
  
  // 将 0-1 的浮点值转换为 0-255 的整数值
  uint8_t r = (uint8_t)(msg->r * 255.0);
  uint8_t g = (uint8_t)(msg->g * 255.0);
  uint8_t b = (uint8_t)(msg->b * 255.0);
  
  // 设置LED颜色
  strip.setPixelColor(0, strip.Color(r, g, b));
  strip.show();
}

void setup() {
  // 初始化串口（用于 micro-ROS 通信）
  Serial.begin(115200);
  set_microros_transports();
  
  // 初始化 NeoPixel
  strip.begin();
  strip.setPixelColor(0, strip.Color(0, 0, 0));
  strip.show();
  
  delay(2000);

  allocator = rcl_get_default_allocator();

  // 创建初始化选项
  RCCHECK(rclc_support_init(&support, 0, NULL, &allocator));

  // 创建节点
  RCCHECK(rclc_node_init_default(&node, "rgb_led_node", "", &support));

  // 创建订阅者
  RCCHECK(rclc_subscription_init_default(
    &subscriber,
    &node,
    ROSIDL_GET_MSG_TYPE_SUPPORT(std_msgs, msg, ColorRGBA),
    "led_color"));

  // 创建执行器
  RCCHECK(rclc_executor_init(&executor, &support.context, 1, &allocator));
  RCCHECK(rclc_executor_add_subscription(&executor, &subscriber, &msg, &subscription_callback, ON_NEW_DATA));
  
  // 启动指示 - 白色闪烁
  strip.setPixelColor(0, strip.Color(50, 50, 50));
  strip.show();
  delay(500);
  strip.setPixelColor(0, strip.Color(0, 0, 0));
  strip.show();
}

void loop() {
  delay(100);
  RCSOFTCHECK(rclc_executor_spin_some(&executor, RCL_MS_TO_NS(100)));
}