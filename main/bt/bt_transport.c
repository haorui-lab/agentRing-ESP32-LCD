#include "bt_transport.h"
#include "ble_server.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include "esp_log.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/ringbuf.h"

static const char *TAG = "BtTransport";

#define RX_BUFFER_SIZE 4096
static char s_rx_buf[RX_BUFFER_SIZE];
static size_t s_rx_len = 0;

static RingbufHandle_t s_rx_ringbuf = NULL;
static bt_transport_config_t s_config;
static int64_t s_last_active_time_ms = 0;
static bool s_is_connected = false;
static bool s_usb_active = false;

static const char *DEMO_JSON_FORMAT = 
"{\"timestamp\":%lld,\"providers\":["
"{\"id\":\"codex\",\"name\":\"Codex\",\"primary\":{\"label\":\"7天\",\"remainingPercent\":16.0,\"resetsAt\":\"4d 14h\"},\"rows\":[{\"label\":\"7天\",\"percent\":\"16%\",\"reset\":\"4d 14h\"}]},"
"{\"id\":\"antigravity\",\"name\":\"Antigravity\",\"primary\":{\"label\":\"Gemini 5小时\",\"remainingPercent\":51.0,\"resetsAt\":\"3h 33m\"},\"secondary\":{\"label\":\"Gemini 7天\",\"remainingPercent\":86.0,\"resetsAt\":\"6d 17h\"},\"rows\":[{\"label\":\"Gemini 5小时\",\"percent\":\"51%\",\"reset\":\"3h 33m\"},{\"label\":\"Gemini 7天\",\"percent\":\"86%\",\"reset\":\"6d 17h\"}]},"
"{\"id\":\"antigravity_third\",\"name\":\"Antigravity Third\",\"primary\":{\"label\":\"Third 5小时\",\"remainingPercent\":100.0,\"resetsAt\":\"4h 59m\"},\"secondary\":{\"label\":\"Third 7天\",\"remainingPercent\":33.0,\"resetsAt\":\"5d 18h\"},\"rows\":[{\"label\":\"Third 5小时\",\"percent\":\"100%\",\"reset\":\"4h 59m\"},{\"label\":\"Third 7天\",\"percent\":\"33%\",\"reset\":\"5d 18h\"}]},"
"{\"id\":\"cursor\",\"name\":\"Cursor\",\"primary\":{\"label\":\"Cursor 模型\",\"remainingPercent\":76.0,\"resetsAt\":\"11d 8h\",\"remainingDetails\":\"380 / 500\"},\"secondary\":{\"label\":\"其他模型\",\"remainingPercent\":100.0},\"rows\":[{\"label\":\"Cursor 模型\",\"percent\":\"76%\",\"reset\":\"11d 8h\"},{\"label\":\"其他模型\",\"percent\":\"$25.00\",\"reset\":\"\"}]}"
"]}\n";

static void process_complete_line(char *line) {
    s_last_active_time_ms = esp_timer_get_time() / 1000;

    char *json_start = strchr(line, '{');
    if (!json_start) {
        return;
    }

    sync_payload_t payload;
    if (parse_sync_payload(json_start, &payload)) {
        if (strcmp(payload.type, "ping") == 0) {
            ESP_LOGD(TAG, "收到 AgentRing 保活心跳 ping (ts: %lld)", payload.timestamp);
        } else {
            ESP_LOGI(TAG, "成功解析用量数据帧 (包含 %d 个供应商)", payload.provider_count);
            if (s_config.on_payload) {
                s_config.on_payload(&payload);
            }
        }
    } else {
        ESP_LOGW(TAG, "未能解析数据行: %.64s...", json_start);
    }
}

void bt_transport_feed_bytes(const uint8_t *data, size_t len) {
    if (!data || len == 0 || !s_rx_ringbuf) return;

    if (!s_is_connected) {
        s_is_connected = true;
        if (s_config.on_state) {
            s_config.on_state(UI_BT_STATE_CONNECTED, s_usb_active ? "已连接 (USB-C)" : "已连接 (Mac)");
        }
    }

    /* Fast, non-blocking copy into ring buffer; zero stack load on calling task */
    BaseType_t res = xRingbufferSend(s_rx_ringbuf, data, len, pdMS_TO_TICKS(20));
    if (res != pdTRUE) {
        ESP_LOGW(TAG, "RingBuffer 满，临时丢弃 %d 字节", (int)len);
    }
}

static void on_ble_rx(const uint8_t *data, size_t len) {
    s_usb_active = false;
    bt_transport_feed_bytes(data, len);
}

static void on_ble_conn_state(bool connected, const char *detail) {
    s_is_connected = connected;
    if (s_config.on_state) {
        s_config.on_state(connected ? UI_BT_STATE_CONNECTED : UI_BT_STATE_ADVERTISING, detail);
    }
}

static void bt_rx_worker_task(void *pvParameters) {
    ESP_LOGI(TAG, "数据流解析处理任务就绪 (Stack: 16KB)");
    size_t item_size = 0;
    while (1) {
        uint8_t *item = (uint8_t *)xRingbufferReceiveUpTo(s_rx_ringbuf, &item_size, pdMS_TO_TICKS(50), 256);
        if (item && item_size > 0) {
            for (size_t i = 0; i < item_size; i++) {
                char ch = (char)item[i];
                if (ch == '\r') continue;

                if (ch == '\n') {
                    s_rx_buf[s_rx_len] = '\0';
                    if (s_rx_len > 0) {
                        process_complete_line(s_rx_buf);
                    }
                    s_rx_len = 0;
                } else {
                    // Ignore leading whitespace/noise before the opening brace '{'
                    if (s_rx_len == 0 && ch != '{') {
                        continue;
                    }
                    if (s_rx_len < RX_BUFFER_SIZE - 1) {
                        s_rx_buf[s_rx_len++] = ch;
                    } else {
                        ESP_LOGE(TAG, "RX 缓冲区溢出，重置缓冲区");
                        s_rx_len = 0;
                    }
                }
            }
            vRingbufferReturnItem(s_rx_ringbuf, item);
        }
    }
}

static void usb_serial_rx_task(void *pvParameters) {
    ESP_LOGI(TAG, "USB-C 串口实时接收任务已启动");
    uint8_t buf[256];
    while (1) {
        int n = read(fileno(stdin), buf, sizeof(buf));
        if (n > 0) {
            s_usb_active = true;
            bt_transport_feed_bytes(buf, (size_t)n);
        } else {
            vTaskDelay(pdMS_TO_TICKS(20));
        }
    }
}

void bt_transport_inject_demo_data(void) {
    ESP_LOGI(TAG, "注入演示样例数据 (4 个供应商: Codex, Antigravity, Antigravity Third, Cursor)");
    int64_t now_ts = 1789896924; // 2026-09-20 17:35:24 CST
    char buf[2048];
    snprintf(buf, sizeof(buf), DEMO_JSON_FORMAT, (long long)now_ts);
    bt_transport_feed_bytes((const uint8_t *)buf, strlen(buf));
}

void bt_transport_retry(void) {
    ESP_LOGI(TAG, "用户手动触发重试，重启 BLE 广播会话");
    s_is_connected = false;
    s_usb_active = false;
    s_rx_len = 0;
    s_last_active_time_ms = 0;
    if (s_config.on_state) {
        s_config.on_state(UI_BT_STATE_ADVERTISING, "重新广播中，等待连接…");
    }
    ble_server_restart_advertising();
}

void bt_transport_tick(void) {
    int64_t now_ms = esp_timer_get_time() / 1000;
    if (s_is_connected && s_last_active_time_ms > 0) {
        if (now_ms - s_last_active_time_ms > 60000) { // 60s timeout
            ESP_LOGW(TAG, "连接超时 (60s 无数据/心跳)，主动重置会话并重启广播");
            s_is_connected = false;
            s_usb_active = false;
            s_rx_len = 0;
            s_last_active_time_ms = 0;
            if (s_config.on_state) {
                s_config.on_state(UI_BT_STATE_ADVERTISING, "连接超时，重新广播中…");
            }
            ble_server_restart_advertising();
        }
    } else if (!s_is_connected && !s_usb_active) {
        // 广播看门狗：如果当前未连接且 BLE 广播意外停止，则自动恢复广播
        static int64_t s_last_adv_check_ms = 0;
        if (now_ms - s_last_adv_check_ms > 5000) {
            s_last_adv_check_ms = now_ms;
            if (!ble_server_is_advertising()) {
                ESP_LOGI(TAG, "看门狗检测到广播已停止，自动恢复广播");
                ble_server_restart_advertising();
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
    s_usb_active = false;
    s_last_active_time_ms = 0;

    /* 1. Create 16KB ByteBuf RingBuffer for zero-copy async data passing */
    s_rx_ringbuf = xRingbufferCreate(16384, RINGBUF_TYPE_BYTEBUF);
    if (!s_rx_ringbuf) {
        ESP_LOGE(TAG, "创建 RX RingBuffer 失败!");
        return false;
    }

    /* 2. Start dedicated high-stack worker task for JSON parsing & LVGL update */
    BaseType_t worker_ret = xTaskCreate(bt_rx_worker_task, "bt_worker", 16384, NULL, 5, NULL);
    if (worker_ret != pdPASS) {
        ESP_LOGE(TAG, "创建数据流解析任务失败");
    }

    /* 3. Start background USB Serial RX Task */
    BaseType_t usb_ret = xTaskCreate(usb_serial_rx_task, "usb_rx", 4096, NULL, 4, NULL);
    if (usb_ret != pdPASS) {
        ESP_LOGE(TAG, "创建 USB 串口接收任务失败");
    }

    const char *name = s_config.device_name ? s_config.device_name : "AgentRing-ESP32-LCD";

    /* 4. Initialize BLE Peripheral (Nordic UART Service via NimBLE over ESP-Hosted) */
    bool ble_ok = ble_server_init(name, on_ble_rx, on_ble_conn_state);
    if (!ble_ok) {
        ESP_LOGW(TAG, "BLE 外设初始化未就绪，继续启动 USB 串口通道");
    }

    ESP_LOGI(TAG, "传输层就绪 (BLE: %s, USB: 就绪), 设备名: %s", ble_ok ? "广播中" : "未启动", name);
    if (s_config.on_state) {
        s_config.on_state(UI_BT_STATE_ADVERTISING, "广播就绪，等待 Mac 连接同步…");
    }

    return true;
}
