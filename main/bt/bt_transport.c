#include "bt_transport.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include "esp_log.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

static const char *TAG = "BtTransport";

#define RX_BUFFER_SIZE 4096
static char s_rx_buf[RX_BUFFER_SIZE];
static size_t s_rx_len = 0;

static bt_transport_config_t s_config;
static int64_t s_last_active_time_ms = 0;
static bool s_is_connected = false;

static const char *DEMO_JSON = 
"{\n"
"  \"timestamp\": 1726487626,\n"
"  \"providers\": [\n"
"    {\n"
"      \"id\": \"codex\",\n"
"      \"name\": \"Codex\",\n"
"      \"primary\": {\n"
"        \"label\": \"7天\",\n"
"        \"remainingPercent\": 16.0,\n"
"        \"resetsAt\": \"4d 14h\"\n"
"      },\n"
"      \"rows\": [\n"
"        {\n"
"          \"label\": \"7天\",\n"
"          \"percent\": \"16%\",\n"
"          \"reset\": \"4d 14h\"\n"
"        }\n"
"      ]\n"
"    },\n"
"    {\n"
"      \"id\": \"antigravity\",\n"
"      \"name\": \"Antigravity\",\n"
"      \"primary\": {\n"
"        \"label\": \"Gemini 5小时\",\n"
"        \"remainingPercent\": 51.0,\n"
"        \"resetsAt\": \"3h 33m\"\n"
"      },\n"
"      \"secondary\": {\n"
"        \"label\": \"Gemini 7天\",\n"
"        \"remainingPercent\": 86.0,\n"
"        \"resetsAt\": \"6d 17h\"\n"
"      },\n"
"      \"rows\": [\n"
"        {\n"
"          \"label\": \"Gemini 5小时\",\n"
"          \"percent\": \"51%\",\n"
"          \"reset\": \"3h 33m\"\n"
"        },\n"
"        {\n"
"          \"label\": \"Gemini 7天\",\n"
"          \"percent\": \"86%\",\n"
"          \"reset\": \"6d 17h\"\n"
"        }\n"
"      ]\n"
"    },\n"
"    {\n"
"      \"id\": \"antigravity_third\",\n"
"      \"name\": \"Antigravity Third\",\n"
"      \"primary\": {\n"
"        \"label\": \"Claude/GPT 5小时\",\n"
"        \"remainingPercent\": 100.0,\n"
"        \"resetsAt\": \"4h 59m\"\n"
"      },\n"
"      \"secondary\": {\n"
"        \"label\": \"Claude/GPT 7天\",\n"
"        \"remainingPercent\": 33.0,\n"
"        \"resetsAt\": \"5d 18h\"\n"
"      },\n"
"      \"rows\": [\n"
"        {\n"
"          \"label\": \"Claude/GPT 5小时\",\n"
"          \"percent\": \"100%\",\n"
"          \"reset\": \"4h 59m\"\n"
"        },\n"
"        {\n"
"          \"label\": \"Claude/GPT 7天\",\n"
"          \"percent\": \"33%\",\n"
"          \"reset\": \"5d 18h\"\n"
"        }\n"
"      ]\n"
"    }\n"
"  ]\n"
"}\n";

static void process_complete_line(char *line) {
    s_last_active_time_ms = esp_timer_get_time() / 1000;

    sync_payload_t payload;
    if (parse_sync_payload(line, &payload)) {
        if (strcmp(payload.type, "ping") == 0) {
            ESP_LOGD(TAG, "收到 AgentRing 保活心跳 ping (ts: %lld)", payload.timestamp);
        } else {
            ESP_LOGI(TAG, "成功解析用量数据帧 (包含 %d 个供应商)", payload.provider_count);
            if (s_config.on_payload) {
                s_config.on_payload(&payload);
            }
        }
    } else {
        ESP_LOGW(TAG, "未能解析数据行: %.64s...", line);
    }
}

void bt_transport_feed_bytes(const uint8_t *data, size_t len) {
    if (!data || len == 0) return;

    if (!s_is_connected) {
        s_is_connected = true;
        if (s_config.on_state) {
            s_config.on_state(UI_BT_STATE_CONNECTED, "已连接 (Mac)");
        }
    }

    for (size_t i = 0; i < len; i++) {
        char ch = (char)data[i];
        if (ch == '\r') continue; // Ignore carriage return

        if (ch == '\n') {
            s_rx_buf[s_rx_len] = '\0';
            if (s_rx_len > 0) {
                process_complete_line(s_rx_buf);
            }
            s_rx_len = 0;
        } else {
            if (s_rx_len < RX_BUFFER_SIZE - 1) {
                s_rx_buf[s_rx_len++] = ch;
            } else {
                ESP_LOGE(TAG, "RX 缓冲区溢出，重置缓冲区");
                s_rx_len = 0;
            }
        }
    }
}

void bt_transport_inject_demo_data(void) {
    ESP_LOGI(TAG, "注入演示样例数据");
    bt_transport_feed_bytes((const uint8_t *)DEMO_JSON, strlen(DEMO_JSON));
}

void bt_transport_tick(void) {
    int64_t now_ms = esp_timer_get_time() / 1000;
    if (s_is_connected && s_last_active_time_ms > 0) {
        if (now_ms - s_last_active_time_ms > 60000) { // 60s timeout
            ESP_LOGW(TAG, "连接超时 (60s 无数据/心跳)，断开会话");
            s_is_connected = false;
            s_rx_len = 0;
            if (s_config.on_state) {
                s_config.on_state(UI_BT_STATE_ADVERTISING, "连接超时，等待重新连接…");
            }
        }
    }
}

bool bt_transport_init(const bt_transport_config_t *config) {
    if (config) {
        s_config = *config;
    }

    s_rx_len = 0;
    s_is_connected = false;
    s_last_active_time_ms = 0;

    ESP_LOGI(TAG, "蓝牙传输层初始化完成，设备名: %s", s_config.device_name ? s_config.device_name : "AgentRing-ESP32");
    if (s_config.on_state) {
        s_config.on_state(UI_BT_STATE_ADVERTISING, "广播就绪，等待 Mac 连接…");
    }

    return true;
}
