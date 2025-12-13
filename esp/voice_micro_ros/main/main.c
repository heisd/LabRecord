#include <stdio.h>
#include <string.h>
#include <math.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "esp_log.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "nvs_flash.h"
#include "driver/i2c.h"
#include "driver/i2s_std.h"

// micro-ROS
#include <rcl/rcl.h>
#include <rclc/rclc.h>
#include <rclc/executor.h>
#include <std_msgs/msg/float32_multi_array.h>
#include <std_msgs/msg/int32.h>
#include <rmw_microros/rmw_microros.h>

// ESP-SR 语音识别
#include "esp_afe_sr_iface.h"
#include "esp_afe_sr_models.h"
#include "esp_mn_iface.h"
#include "esp_mn_models.h"
#include "model_path.h"

static const char *TAG = "SERVO_VOICE";

// ========== 配置 ==========
#define WIFI_SSID      "WHEELTEC_S300"
#define WIFI_PASS      "dongguan"
#define AGENT_IP       "192.168.0.100"
#define AGENT_PORT     8888

// I2C (PCA9685)
#define I2C_MASTER_NUM     I2C_NUM_0
#define I2C_SDA_PIN        8
#define I2C_SCL_PIN        9
#define I2C_FREQ_HZ        400000
#define PCA9685_ADDR       0x40

// I2S 麦克风 (INMP441)
#define I2S_NUM            I2S_NUM_1
#define I2S_BCK_PIN        16
#define I2S_WS_PIN         17
#define I2S_DATA_PIN       15
#define SAMPLE_RATE        16000

// ========== 命令定义 ==========
typedef enum {
    CMD_NONE = 0,
    CMD_FORWARD,    // 前进
    CMD_BACKWARD,   // 后退
    CMD_GRAB,       // 抓取
    CMD_RELEASE,    // 释放
    CMD_STOP,       // 停止
    CMD_RESET,      // 复位
} voice_cmd_t;

// 命令词（拼音格式）
static const char *commands[] = {
    "qian jin",     // 前进
    "hou tui",      // 后退
    "zhua qu",      // 抓取
    "shi fang",     // 释放
    "ting zhi",     // 停止
    "fu wei",       // 复位
};
#define COMMAND_NUM (sizeof(commands) / sizeof(commands[0]))

// ========== 全局变量 ==========
static i2s_chan_handle_t i2s_rx_handle = NULL;
static esp_afe_sr_iface_t *afe_handle = NULL;
static esp_afe_sr_data_t *afe_data = NULL;
static QueueHandle_t cmd_queue = NULL;

// micro-ROS
static rcl_subscription_t subscriber;
static rcl_publisher_t cmd_publisher;
static std_msgs__msg__Float32MultiArray angle_msg;
static std_msgs__msg__Int32 cmd_msg;
static float angle_buffer[6];

// ========== PCA9685 驱动（保持不变）==========
#define PCA9685_MODE1      0x00
#define PCA9685_PRESCALE   0xFE
#define PCA9685_LED0_ON_L  0x06

static esp_err_t pca9685_write_byte(uint8_t reg, uint8_t value) {
    uint8_t buf[2] = {reg, value};
    return i2c_master_write_to_device(I2C_MASTER_NUM, PCA9685_ADDR, buf, 2, pdMS_TO_TICKS(100));
}

static esp_err_t pca9685_init(void) {
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
    
    pca9685_write_byte(PCA9685_MODE1, 0x10);
    vTaskDelay(pdMS_TO_TICKS(5));
    pca9685_write_byte(PCA9685_PRESCALE, 121);
    pca9685_write_byte(PCA9685_MODE1, 0x00);
    vTaskDelay(pdMS_TO_TICKS(5));
    pca9685_write_byte(PCA9685_MODE1, 0x20);
    
    ESP_LOGI(TAG, "PCA9685 initialized");
    return ESP_OK;
}

static float last_angles[16] = {0};

static void set_servo(uint8_t channel, float angle) {
    if (fabsf(last_angles[channel] - angle) < 1.0f) return;
    last_angles[channel] = angle;
    
    if (angle < 0) angle = 0;
    if (angle > 180) angle = 180;
    
    uint16_t pulse = (uint16_t)(150 + (angle / 180.0f) * 450);
    uint8_t reg = PCA9685_LED0_ON_L + 4 * channel;
    uint8_t buf[5] = {reg, 0, 0, pulse & 0xFF, pulse >> 8};
    i2c_master_write_to_device(I2C_MASTER_NUM, PCA9685_ADDR, buf, 5, pdMS_TO_TICKS(100));
    
    ESP_LOGI(TAG, "Servo %d -> %.1f°", channel, angle);
}

// ========== I2S 麦克风初始化 ==========
static esp_err_t i2s_mic_init(void) {
    i2s_chan_config_t chan_cfg = I2S_CHANNEL_DEFAULT_CONFIG(I2S_NUM, I2S_ROLE_MASTER);
    ESP_ERROR_CHECK(i2s_new_channel(&chan_cfg, NULL, &i2s_rx_handle));
    
    i2s_std_config_t std_cfg = {
        .clk_cfg = I2S_STD_CLK_DEFAULT_CONFIG(SAMPLE_RATE),
        .slot_cfg = I2S_STD_PHILIPS_SLOT_DEFAULT_CONFIG(I2S_DATA_BIT_WIDTH_32BIT, I2S_SLOT_MODE_MONO),
        .gpio_cfg = {
            .mclk = I2S_GPIO_UNUSED,
            .bclk = I2S_BCK_PIN,
            .ws = I2S_WS_PIN,
            .dout = I2S_GPIO_UNUSED,
            .din = I2S_DATA_PIN,
            .invert_flags = {
                .mclk_inv = false,
                .bclk_inv = false,
                .ws_inv = false,
            },
        },
    };
    
    ESP_ERROR_CHECK(i2s_channel_init_std_mode(i2s_rx_handle, &std_cfg));
    ESP_ERROR_CHECK(i2s_channel_enable(i2s_rx_handle));
    
    ESP_LOGI(TAG, "I2S microphone initialized");
    return ESP_OK;
}

// ========== ESP-SR 语音识别初始化 ==========
static esp_err_t speech_recognition_init(void) {
    // AFE 配置
    afe_config_t afe_config = AFE_CONFIG_DEFAULT();
    afe_config.wakenet_model_name = "wn9_hilexin";  // 唤醒词 "Hi 乐鑫"
    afe_config.aec_init = false;  // 单麦克风不需要回声消除
    
    afe_handle = (esp_afe_sr_iface_t *)&ESP_AFE_SR_HANDLE;
    afe_data = afe_handle->create_from_config(&afe_config);
    
    if (afe_data == NULL) {
        ESP_LOGE(TAG, "Failed to create AFE");
        return ESP_FAIL;
    }
    
    // MultiNet 命令词识别
    esp_mn_iface_t *mn_handle = esp_mn_handle_from_name("mn6_cn");
    model_iface_data_t *mn_data = mn_handle->create(commands, COMMAND_NUM, 5);
    
    if (mn_data == NULL) {
        ESP_LOGE(TAG, "Failed to create MultiNet");
        return ESP_FAIL;
    }
    
    ESP_LOGI(TAG, "Speech recognition initialized");
    ESP_LOGI(TAG, "Wake word: 'Hi 乐鑫'");
    ESP_LOGI(TAG, "Commands: 前进, 后退, 抓取, 释放, 停止, 复位");
    
    return ESP_OK;
}

// ========== 语音识别任务 ==========
static void speech_recognition_task(void *arg) {
    int audio_chunksize = afe_handle->get_feed_chunksize(afe_data);
    int16_t *audio_buffer = malloc(audio_chunksize * sizeof(int16_t));
    size_t bytes_read;
    
    ESP_LOGI(TAG, "Speech recognition task started");
    
    while (1) {
        // 从麦克风读取音频
        i2s_channel_read(i2s_rx_handle, audio_buffer, 
                         audio_chunksize * sizeof(int16_t), &bytes_read, portMAX_DELAY);
        
        // 送入 AFE 处理
        afe_handle->feed(afe_data, audio_buffer);
        
        // 获取识别结果
        afe_fetch_result_t *result = afe_handle->fetch(afe_data);
        
        if (result == NULL) continue;
        
        // 检测到唤醒词
        if (result->wakeup_state == WAKENET_DETECTED) {
            ESP_LOGI(TAG, "★★★ 唤醒词检测到！请说命令... ★★★");
        }
        
        // 命令词识别结果
        if (result->ret_value == ESP_MN_STATE_DETECTED) {
            int cmd_id = result->phrase_id;
            ESP_LOGI(TAG, "★★★ 识别到命令: %s (ID: %d) ★★★", 
                     commands[cmd_id], cmd_id);
            
            // 发送到命令队列
            voice_cmd_t cmd = cmd_id + 1;  // 命令从 1 开始
            xQueueSend(cmd_queue, &cmd, 0);
        }
    }
    
    free(audio_buffer);
}

// ========== 命令执行任务 ==========
static void command_execute_task(void *arg) {
    voice_cmd_t cmd;
    
    // 预定义动作角度
    const float poses[][6] = {
        [CMD_FORWARD]  = {90, 60, 90, 90, 90, 90},   // 前进姿态
        [CMD_BACKWARD] = {90, 120, 90, 90, 90, 90},  // 后退姿态
        [CMD_GRAB]     = {90, 90, 45, 90, 90, 30},   // 抓取（夹爪闭合）
        [CMD_RELEASE]  = {90, 90, 45, 90, 90, 90},   // 释放（夹爪张开）
        [CMD_STOP]     = {90, 90, 90, 90, 90, 90},   // 停止
        [CMD_RESET]    = {90, 90, 90, 90, 90, 90},   // 复位
    };
    
    while (1) {
        if (xQueueReceive(cmd_queue, &cmd, portMAX_DELAY)) {
            ESP_LOGI(TAG, "Executing command: %d", cmd);
            
            if (cmd > CMD_NONE && cmd <= CMD_RESET) {
                // 执行对应动作
                for (int i = 0; i < 6; i++) {
                    set_servo(i, poses[cmd][i]);
                }
                
                // 通过 micro-ROS 发布命令（可选）
                cmd_msg.data = cmd;
                // rcl_publish(&cmd_publisher, &cmd_msg, NULL);
            }
        }
    }
}

// ========== WiFi（保持不变）==========
static EventGroupHandle_t wifi_event_group;
#define WIFI_CONNECTED_BIT BIT0

static void wifi_event_handler(void *arg, esp_event_base_t event_base,
                               int32_t event_id, void *event_data) {
    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START) {
        esp_wifi_connect();
    } else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED) {
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
    
    xEventGroupWaitBits(wifi_event_group, WIFI_CONNECTED_BIT, pdFALSE, pdTRUE, portMAX_DELAY);
    ESP_LOGI(TAG, "WiFi connected!");
}

// ========== micro-ROS 回调（保持不变）==========
static unsigned long msg_count = 0;

static void subscription_callback(const void *msgin) {
    const std_msgs__msg__Float32MultiArray *m = 
        (const std_msgs__msg__Float32MultiArray *)msgin;
    
    msg_count++;
    ESP_LOGI(TAG, "★★★ ROS MESSAGE #%lu ★★★", msg_count);
    
    if (m->data.size >= 6) {
        for (int i = 0; i < 6; i++) {
            set_servo(i, m->data.data[i]);
        }
    }
}

// ========== micro-ROS 任务 ==========
static void micro_ros_task(void *arg) {
    rcl_allocator_t allocator = rcl_get_default_allocator();
    rclc_support_t support;
    rcl_node_t node;
    rclc_executor_t executor;
    
    // 等待 Agent
    ESP_LOGI(TAG, "Waiting for micro-ROS agent...");
    while (rmw_uros_ping_agent(1000, 1) != RMW_RET_OK) {
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
    ESP_LOGI(TAG, "Agent connected!");
    
    // 初始化消息
    angle_msg.data.data = angle_buffer;
    angle_msg.data.size = 0;
    angle_msg.data.capacity = 6;
    
    // 创建实体
    rclc_support_init(&support, 0, NULL, &allocator);
    rclc_node_init_default(&node, "servo_voice_node", "", &support);
    
    rclc_subscription_init_default(&subscriber, &node,
        ROSIDL_GET_MSG_TYPE_SUPPORT(std_msgs, msg, Float32MultiArray),
        "servo_angles");
    
    rclc_publisher_init_default(&cmd_publisher, &node,
        ROSIDL_GET_MSG_TYPE_SUPPORT(std_msgs, msg, Int32),
        "voice_command");
    
    rclc_executor_init(&executor, &support.context, 1, &allocator);
    rclc_executor_add_subscription(&executor, &subscriber, &angle_msg,
                                   &subscription_callback, ON_NEW_DATA);
    
    ESP_LOGI(TAG, "micro-ROS ready!");
    
    while (1) {
        rclc_executor_spin_some(&executor, RCL_MS_TO_NS(100));
        vTaskDelay(pdMS_TO_TICKS(10));
    }
}

// ========== 主函数 ==========
void app_main(void) {
    ESP_LOGI(TAG, "========================================");
    ESP_LOGI(TAG, "  Voice-Controlled Servo (ESP-IDF)");
    ESP_LOGI(TAG, "========================================");
    
    // NVS
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);
    
    // 创建命令队列
    cmd_queue = xQueueCreate(10, sizeof(voice_cmd_t));
    
    // 硬件初始化
    pca9685_init();
    i2s_mic_init();
    
    // 舵机归中
    ESP_LOGI(TAG, "Resetting servos...");
    for (int i = 0; i < 6; i++) {
        last_angles[i] = -999;
        set_servo(i, 90);
        vTaskDelay(pdMS_TO_TICKS(100));
    }
    
    // 语音识别初始化
    speech_recognition_init();
    
    // WiFi
    wifi_init();
    
    // 启动任务
    xTaskCreatePinnedToCore(speech_recognition_task, "sr_task", 8192, NULL, 5, NULL, 0);
    xTaskCreatePinnedToCore(command_execute_task, "cmd_task", 4096, NULL, 4, NULL, 1);
    xTaskCreatePinnedToCore(micro_ros_task, "uros_task", 16384, NULL, 5, NULL, 1);
    
    ESP_LOGI(TAG, "========================================");
    ESP_LOGI(TAG, "  System Ready!");
    ESP_LOGI(TAG, "  Say 'Hi 乐鑫' to wake up");
    ESP_LOGI(TAG, "  Then say: 前进/后退/抓取/释放/停止/复位");
    ESP_LOGI(TAG, "========================================");
}
