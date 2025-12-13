#include <stdio.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "nvs_flash.h"
#include "driver/i2c.h"

#include <rcl/rcl.h>
#include <rclc/rclc.h>
#include <rclc/executor.h>
#include <std_msgs/msg/float32_multi_array.h>
#include <rmw_microros/rmw_microros.h>
#include <uros_network_interfaces.h>

static const char *TAG = "SERVO_CTRL";

// ========== 配置 ==========
#define WIFI_SSID      "WHEELTEC_S300_S300"
#define WIFI_PASS      "dongguan"
#define AGENT_IP       "192.168.0.100"
#define AGENT_PORT     8888

#define I2C_MASTER_NUM     I2C_NUM_0
#define I2C_SDA_PIN        8
#define I2C_SCL_PIN        9
#define I2C_FREQ_HZ        400000
#define PCA9685_ADDR       0x40

// ========== PCA9685 驱动 ==========
#define PCA9685_MODE1      0x00
#define PCA9685_PRESCALE   0xFE
#define PCA9685_LED0_ON_L  0x06

static esp_err_t pca9685_write_byte(uint8_t reg, uint8_t value) {
    uint8_t buf[2] = {reg, value};
    return i2c_master_write_to_device(I2C_MASTER_NUM, PCA9685_ADDR, buf, 2, pdMS_TO_TICKS(100));
}

static esp_err_t pca9685_init(void) {
    // I2C 初始化
    i2c_config_t conf = {
        .mode = I2C_MODE_MASTER,
        .sda_io_num = I2C_SDA_PIN,
        .scl_io_num = I2C_SCL_PIN,
        .sda_pullup_en = GPIO_PULLUP_ENABLE,
        .scl_pullup_en = GPIO_PULLUP_ENABLE,
        .master.clk_speed = I2C_FREQ_HZ,
    };
    ESP_ERROR_CHECK(i2c_param_config(I2C_MASTER_NUM, &conf));
    ESP_ERROR_CHECK(i2c_driver_install(I2C_MASTER_NUM, conf.mode, 0, 0, 0));
    
    // PCA9685 初始化：设置 50Hz PWM
    pca9685_write_byte(PCA9685_MODE1, 0x10);  // Sleep
    vTaskDelay(pdMS_TO_TICKS(5));
    
    // prescale = 25MHz / (4096 * 50Hz) - 1 ≈ 121
    pca9685_write_byte(PCA9685_PRESCALE, 121);
    
    pca9685_write_byte(PCA9685_MODE1, 0x00);  // Wake up
    vTaskDelay(pdMS_TO_TICKS(5));
    pca9685_write_byte(PCA9685_MODE1, 0x20);  // Auto-increment
    
    ESP_LOGI(TAG, "PCA9685 initialized");
    return ESP_OK;
}

static float last_angles[16] = {0};

static void set_servo(uint8_t channel, float angle) {
    // 死区保护
    if (fabsf(last_angles[channel] - angle) < 1.0f) {
        return;
    }
    last_angles[channel] = angle;
    
    // 限制角度范围
    if (angle < 0) angle = 0;
    if (angle > 180) angle = 180;
    
    // 角度转 PWM (150-600 对应 0-180°)
    uint16_t pulse = (uint16_t)(150 + (angle / 180.0f) * 450);
    
    uint8_t reg = PCA9685_LED0_ON_L + 4 * channel;
    uint8_t buf[5] = {reg, 0, 0, pulse & 0xFF, pulse >> 8};
    i2c_master_write_to_device(I2C_MASTER_NUM, PCA9685_ADDR, buf, 5, pdMS_TO_TICKS(100));
    
    ESP_LOGI(TAG, "Servo %d -> %.1f°", channel, angle);
}

// ========== WiFi ==========
static EventGroupHandle_t wifi_event_group;
#define WIFI_CONNECTED_BIT BIT0

static void wifi_event_handler(void *arg, esp_event_base_t event_base,
                               int32_t event_id, void *event_data) {
    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START) {
        esp_wifi_connect();
    } else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED) {
        ESP_LOGW(TAG, "WiFi disconnected, reconnecting...");
        esp_wifi_connect();
    } else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
        ip_event_got_ip_t *event = (ip_event_got_ip_t *)event_data;
        ESP_LOGI(TAG, "Got IP: " IPSTR, IP2STR(&event->ip_info.ip));
        xEventGroupSetBits(wifi_event_group, WIFI_CONNECTED_BIT);
    }
}

static void wifi_init(void) {
    wifi_event_group = xEventGroupCreate();
    
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    esp_netif_create_default_wifi_sta();
    
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));
    
    ESP_ERROR_CHECK(esp_event_handler_instance_register(WIFI_EVENT, ESP_EVENT_ANY_ID,
                    &wifi_event_handler, NULL, NULL));
    ESP_ERROR_CHECK(esp_event_handler_instance_register(IP_EVENT, IP_EVENT_STA_GOT_IP,
                    &wifi_event_handler, NULL, NULL));
    
    wifi_config_t wifi_config = {
        .sta = {
            .ssid = WIFI_SSID,
            .password = WIFI_PASS,
        },
    };
    
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_config));
    ESP_ERROR_CHECK(esp_wifi_start());
    
    ESP_LOGI(TAG, "Waiting for WiFi...");
    xEventGroupWaitBits(wifi_event_group, WIFI_CONNECTED_BIT, pdFALSE, pdTRUE, portMAX_DELAY);
    ESP_LOGI(TAG, "WiFi connected!");
}

// ========== micro-ROS ==========
static rcl_subscription_t subscriber;
static std_msgs__msg__Float32MultiArray msg;
static float data_buffer[6];
static unsigned long msg_count = 0;

static void subscription_callback(const void *msgin) {
    const std_msgs__msg__Float32MultiArray *m = 
        (const std_msgs__msg__Float32MultiArray *)msgin;
    
    msg_count++;
    ESP_LOGI(TAG, "★★★ MESSAGE #%lu RECEIVED ★★★", msg_count);
    
    if (m->data.size >= 6) {
        for (int i = 0; i < 6; i++) {
            set_servo(i, m->data.data[i]);
        }
    }
}

static void micro_ros_task(void *arg) {
    rcl_allocator_t allocator = rcl_get_default_allocator();
    rclc_support_t support;
    rcl_node_t node;
    rclc_executor_t executor;
    
    // 配置 Agent 地址
    rmw_uros_set_custom_transport(
        false,
        NULL,
        uros_transport_open,
        uros_transport_close,
        uros_transport_write,
        uros_transport_read
    );
    
    // 设置 Agent IP 和端口
    char agent_ip[] = AGENT_IP;
    char agent_port[10];
    snprintf(agent_port, sizeof(agent_port), "%d", AGENT_PORT);
    
    #ifdef CONFIG_MICRO_ROS_ESP_NETIF_WLAN
    ucdr_init_udp_transport(agent_ip, agent_port);
    #endif
    
    // 等待 Agent
    ESP_LOGI(TAG, "Waiting for micro-ROS agent at %s:%d...", AGENT_IP, AGENT_PORT);
    while (rmw_uros_ping_agent(1000, 1) != RMW_RET_OK) {
        ESP_LOGW(TAG, "Agent not found, retrying...");
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
    ESP_LOGI(TAG, "Agent connected!");
    
    // 初始化消息缓冲区
    msg.data.data = data_buffer;
    msg.data.size = 0;
    msg.data.capacity = 6;
    msg.layout.dim.data = NULL;
    msg.layout.dim.size = 0;
    msg.layout.dim.capacity = 0;
    
    // 创建 ROS 实体
    rclc_support_init(&support, 0, NULL, &allocator);
    rclc_node_init_default(&node, "servo_node", "", &support);
    
    rclc_subscription_init_default(
        &subscriber, &node,
        ROSIDL_GET_MSG_TYPE_SUPPORT(std_msgs, msg, Float32MultiArray),
        "servo_angles"
    );
    
    rclc_executor_init(&executor, &support.context, 1, &allocator);
    rclc_executor_add_subscription(&executor, &subscriber, &msg, 
                                   &subscription_callback, ON_NEW_DATA);
    
    ESP_LOGI(TAG, "========================================");
    ESP_LOGI(TAG, "  ★★★ SYSTEM READY ★★★");
    ESP_LOGI(TAG, "========================================");
    ESP_LOGI(TAG, "Subscribed to: /servo_angles");
    
    // 主循环
    while (1) {
        rclc_executor_spin_some(&executor, RCL_MS_TO_NS(100));
        vTaskDelay(pdMS_TO_TICKS(10));
    }
}

// ========== 主函数 ==========
void app_main(void) {
    ESP_LOGI(TAG, "========================================");
    ESP_LOGI(TAG, "  micro-ROS Servo Controller (ESP-IDF)");
    ESP_LOGI(TAG, "========================================");
    
    // NVS 初始化（WiFi 需要）
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);
    
    // 硬件初始化
    pca9685_init();
    
    // 所有舵机归中
    ESP_LOGI(TAG, "Setting all servos to 90°...");
    for (int i = 0; i < 6; i++) {
        last_angles[i] = -999;  // 强制更新
        set_servo(i, 90);
        vTaskDelay(pdMS_TO_TICKS(100));
    }
    
    // WiFi 连接
    wifi_init();
    
    // 启动 micro-ROS 任务
    xTaskCreate(micro_ros_task, "micro_ros_task", 16384, NULL, 5, NULL);
}