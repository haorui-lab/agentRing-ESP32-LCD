#include <stdio.h>
#include <time.h>
#include <stdlib.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_err.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include "lvgl.h"
#include "bsp/esp-bsp.h"
#include "bsp/display.h"
#include "ui/ui_dashboard.h"
#include "bt/bt_transport.h"
#include "esp_mac.h"

static const char *TAG = "AgentRingMain";

static void on_bt_payload_received(const sync_payload_t *payload) {
    if (bsp_display_lock(-1)) {
        ui_dashboard_update_payload(payload);
        bsp_display_unlock();
    }
}

static void on_bt_state_changed(ui_bt_state_t state, const char *detail) {
    if (bsp_display_lock(-1)) {
        ui_dashboard_set_bt_status(state, detail);
        bsp_display_unlock();
    }
}

void app_main(void) {
    ESP_LOGI(TAG, "================================================");
    ESP_LOGI(TAG, "  AgentRing ESP32 LCD Companion Display");
    ESP_LOGI(TAG, "  Waveshare ESP32-P4-WIFI6-Touch-LCD-7B");
    ESP_LOGI(TAG, "================================================");

    // 1. Initialize NVS and Timezone
    setenv("TZ", "CST-8", 1);
    tzset();

    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    // 2. Initialize BSP Display (1024x600 IPS Touch LCD with LVGL 9)
    bsp_display_cfg_t cfg = {
        .lv_adapter_cfg = ESP_LV_ADAPTER_DEFAULT_CONFIG(),
        .rotation = ESP_LV_ADAPTER_ROTATE_0,
        .tear_avoid_mode = ESP_LV_ADAPTER_TEAR_AVOID_MODE_DOUBLE_DIRECT,
        .touch_flags = {
            .swap_xy = 0,
            .mirror_x = 1,
            .mirror_y = 1,
        },
    };

    lv_display_t *display = bsp_display_start_with_config(&cfg);
    ESP_ERROR_CHECK(display != NULL ? ESP_OK : ESP_FAIL);
    bsp_display_backlight_on();
    ESP_LOGI(TAG, "屏幕初始化完成，分辨率: %dx%d", BSP_LCD_H_RES, BSP_LCD_V_RES);

    // 3. Generate dynamic unique device name based on Base MAC address (AgentRing-LCD-XXXX)
    uint8_t mac[6] = {0};
    char device_name[32] = "AgentRing-LCD";
    if (esp_read_mac(mac, ESP_MAC_BASE) == ESP_OK) {
        snprintf(device_name, sizeof(device_name), "AgentRing-LCD-%02X%02X", mac[4], mac[5]);
    } else {
        snprintf(device_name, sizeof(device_name), "AgentRing-LCD-0000");
    }
    ESP_LOGI(TAG, "设备专属唯一名称: %s (MAC: %02X:%02X:%02X:%02X:%02X:%02X)",
             device_name, mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);

    // 4. Initialize UI Dashboard
    if (bsp_display_lock(-1)) {
        ui_dashboard_init();
        ui_dashboard_set_retry_callback(bt_transport_retry);
        int initial_brightness = ui_dashboard_get_brightness();
        bsp_display_brightness_set(initial_brightness);
        ESP_LOGI(TAG, "恢复屏幕背光亮度: %d%%", initial_brightness);
        ui_dashboard_set_device_name(device_name);
        ui_dashboard_set_bt_status(UI_BT_STATE_ADVERTISING, "广播就绪，等待 Mac 连接…");
        bsp_display_unlock();
    }

    // 5. Initialize Bluetooth Transport Layer
    bt_transport_config_t bt_cfg = {
        .device_name = device_name,
        .on_payload = on_bt_payload_received,
        .on_state = on_bt_state_changed,
    };
    bt_transport_init(&bt_cfg);

    // 5. Dual Transport (BLE + USB Serial) is active. Screen displays waiting state until first live frame.
    ESP_LOGI(TAG, "已就绪，正在等待 Mac 端实时推流数据 (支持 BLE 无线 / USB-C 串口)...");

    // 6. Main Background Watchdog Loop
    while (1) {
        vTaskDelay(pdMS_TO_TICKS(1000));
        bt_transport_tick();
    }
}
