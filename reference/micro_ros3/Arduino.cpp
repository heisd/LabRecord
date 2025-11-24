#include <micro_ros_arduino.h>
#include <rcl/rcl.h>
#include <rcl/error_handling.h>
#include <rclc/rclc.h>
#include <rclc/executor.h>
#include <std_msgs/msg/int32.h>
#include <std_msgs/msg/string.h>
// 原理图上的GPIO2对应38号引脚
#define LED_PIN 38
// micro-ROS 对象
rcl_node_t node;
rcl_publisher_t publisher;
rcl_subscription_t subscriber;
std_msgs__msg__Int32 pub_msg;
std_msgs__msg__String sub_msg;
rclc_executor_t executor;
rcl_allocator_t allocator;
rclc_support_t support;
// 将timer移动到全局作用域 类型是rcl_timer_t
rcl_timer_t timer;

// 话题名称
const char *pub_topic = "esp32_to_host";    // ESP32 → 主机
const char *sub_topic = "host_to_esp32";    // 主机 → ESP

// 订阅者回调函数（接收来自主机的消息）
void subscription_callback(const void *msgin) {
    const std_msgs__msg__String *msg = (const std_msgs__msg__String *)msgin;
    Serial0.print("收到主机消息: ");
    //Serial.print("收到主机消息: ");
    Serial0.println(msg->data.data);
    //Serial.println(msg->data.data);
    // 根据接收到的消息执行相应操作
    if (strstr(msg->data.data, "LED_ON") != NULL) {
        // 因为设置的是下拉模式,所以要设置为LOW才会点亮LED
        digitalWrite(LED_PIN, LOW);
        Serial0.println("LED 已打开");
        //Serial.println("LED 已打开");
    } else if (strstr(msg->data.data, "LED_OFF") != NULL){
        digitalWrite(LED_PIN, HIGH);
        Serial0.println("LED 已关闭");
        //Serial.println("LED已关闭");
    }

}
void timer_callback(rcl_timer_t *timer, int64_t last_call_time) {
    (void)last_call_time;
    if (timer != NULL) {
        // 发送传感器数据（示例：模拟读取）
        // A0从原理图上读出
        pub_msg.data = analogRead(A0);

        // 发布消息
        rcl_ret_t ret = rcl_publish(&publisher, &pub_msg, NULL);
        if (ret == RCL_RET_OK) {
            // 使用另一个串口Serial0.print(),
            // 从这个网址上了解到https://docs.arduino.cc/tutorials/nano-esp32/cheat-sheet/#usb-serial--uart
            // 是发送和接收引脚
            Serial0.println("发送数据到主机: ");
            //Serial.print("发送数据到主机: ");
            Serial0.println(pub_msg.data);
            //Serial.println(pub_msg.data);

        } else {
            Serial0.print("发布失败，错误码: ");
            //Serial.print("发布失败，错误码: ");
            Serial0.println(ret);
            //Serial.println(ret);
        }
    }
}
void setup()
{
    // 设置小灯初始模式为输出模式,并设置为下拉模式,根据我们外围电路决定
    // 类似于STM32的GPIO初始化
    pinMode(LED_PIN, OUTPUT);
    // 设置ADC0引脚为输入模式   
    pinMode(A0, INPUT);
    // 初始化串口Serial和Serial0
    Serial.begin(115200);
    Serial0.begin(115200);

    digitalWrite(LED_PIN, HIGH); // 初始化时关闭LED
    set_microros_wifi_transports("WHEELTEC_S300", "dongguan", "192.168.0.100", 8888);
    delay(2000);  // 等待连接建立
    // 初始化 micro-ROS 支持结构
    allocator = rcl_get_default_allocator();
    rclc_support_init(&support, 0, NULL, &allocator);
    // 创建节点
    rclc_node_init_default(&node, "esp32_node", "", &support);
    // 创建发布者
    rclc_publisher_init_default(
        &publisher,
        &node,
        ROSIDL_GET_MSG_TYPE_SUPPORT(std_msgs, msg, Int32),
        pub_topic);
    // 创建订阅者
    rclc_subscription_init_default(
        &subscriber,
        &node,
        ROSIDL_GET_MSG_TYPE_SUPPORT(std_msgs, msg, String),
        sub_topic);
    // 初始化消息
    std_msgs__msg__Int32__init(&pub_msg);
    std_msgs__msg__String__init(&sub_msg);
    // 创建执行器
    rclc_executor_init(&executor, &support.context, 2, &allocator);
    // 添加订阅者和定时器到执行器
    rclc_executor_add_subscription(
        &executor,
        &subscriber,
        &sub_msg,
        &subscription_callback,
        ON_NEW_DATA);
    // 创建定时器
    rclc_timer_init_default(
        &timer,
        &support,
        RCL_MS_TO_NS(1000),
        timer_callback);
    rclc_executor_add_timer(&executor, &timer);
    Serial0.println("ESP32 micro-ROS 双向通信初始化完成!");
    // Serial.println("ESP32 micro-ROS 双向通信初始化完成!");
    
}
void loop(){
    // 处理micro-ros通信
    rclc_executor_spin_some(&executor, RCL_MS_TO_NS(100));
    delay(10);
}
