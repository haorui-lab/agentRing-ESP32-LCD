#include "ble_server.h"
#include <string.h>
#include <stdio.h>
#include "esp_log.h"
#include "esp_err.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

/* NimBLE Includes */
#include "nimble/nimble_port.h"
#include "nimble/nimble_port_freertos.h"
#include "host/ble_hs.h"
#include "host/util/util.h"
#include "services/gap/ble_svc_gap.h"
#include "services/gatt/ble_svc_gatt.h"

static const char *TAG = "BleServer";

/* Nordic UART Service UUIDs (128-bit, Little Endian) */
/* 6E400001-B5A3-F393-E0A9-E50E24DCCA9E */
static const ble_uuid128_t s_svc_uart_uuid =
    BLE_UUID128_INIT(0x9e, 0xca, 0xdc, 0x24, 0x0e, 0xe5, 0xa9, 0xe0,
                     0x93, 0xf3, 0xa3, 0xb5, 0x01, 0x00, 0x40, 0x6e);

/* 6E400002-B5A3-F393-E0A9-E50E24DCCA9E (RX: Client writes to ESP32) */
static const ble_uuid128_t s_chr_uart_rx_uuid =
    BLE_UUID128_INIT(0x9e, 0xca, 0xdc, 0x24, 0x0e, 0xe5, 0xa9, 0xe0,
                     0x93, 0xf3, 0xa3, 0xb5, 0x02, 0x00, 0x40, 0x6e);

/* 6E400003-B5A3-F393-E0A9-E50E24DCCA9E (TX: ESP32 notifies Client) */
static const ble_uuid128_t s_chr_uart_tx_uuid =
    BLE_UUID128_INIT(0x9e, 0xca, 0xdc, 0x24, 0x0e, 0xe5, 0xa9, 0xe0,
                     0x93, 0xf3, 0xa3, 0xb5, 0x03, 0x00, 0x40, 0x6e);

static ble_data_rx_cb_t s_rx_callback = NULL;
static ble_conn_state_cb_t s_state_callback = NULL;
static char s_device_name[32] = "AgentRing-ESP32-LCD";
static uint8_t s_own_addr_type = 0;
static uint16_t s_conn_handle = BLE_HS_CONN_HANDLE_NONE;
static uint16_t s_tx_handle = 0;

static void ble_server_advertise(void);

static int uart_chr_access(uint16_t conn_handle, uint16_t attr_handle,
                           struct ble_gatt_access_ctxt *ctxt, void *arg) {
    if (ctxt->op == BLE_GATT_ACCESS_OP_WRITE_CHR) {
        uint16_t len = OS_MBUF_PKTLEN(ctxt->om);
        if (len > 0) {
            uint8_t buf[512];
            uint16_t copy_len = len < sizeof(buf) ? len : sizeof(buf);
            int rc = ble_hs_mbuf_to_flat(ctxt->om, buf, copy_len, NULL);
            if (rc == 0 && s_rx_callback) {
                s_rx_callback(buf, copy_len);
            }
        }
        return 0;
    }
    return BLE_ATT_ERR_UNLIKELY;
}

static const struct ble_gatt_svc_def s_gatt_svcs[] = {
    {
        .type = BLE_GATT_SVC_TYPE_PRIMARY,
        .uuid = &s_svc_uart_uuid.u,
        .characteristics = (struct ble_gatt_chr_def[]) {
            {
                /* RX characteristic: Write from Mac */
                .uuid = &s_chr_uart_rx_uuid.u,
                .access_cb = uart_chr_access,
                .flags = BLE_GATT_CHR_F_WRITE | BLE_GATT_CHR_F_WRITE_NO_RSP,
            },
            {
                /* TX characteristic: Notify to Mac */
                .uuid = &s_chr_uart_tx_uuid.u,
                .access_cb = uart_chr_access,
                .flags = BLE_GATT_CHR_F_NOTIFY,
                .val_handle = &s_tx_handle,
            },
            { 0 } /* No more characteristics */
        },
    },
    { 0 } /* No more services */
};

static int ble_gap_event(struct ble_gap_event *event, void *arg) {
    switch (event->type) {
    case BLE_GAP_EVENT_CONNECT:
        if (event->connect.status == 0) {
            s_conn_handle = event->connect.conn_handle;
            ESP_LOGI(TAG, "BLE 连接建立! conn_handle=%d", s_conn_handle);
            if (s_state_callback) {
                s_state_callback(true, "已连接 (Mac BLE)");
            }
        } else {
            ESP_LOGW(TAG, "BLE 连接失败, 重新启动广播... status=%d", event->connect.status);
            s_conn_handle = BLE_HS_CONN_HANDLE_NONE;
            ble_server_advertise();
        }
        return 0;

    case BLE_GAP_EVENT_DISCONNECT:
        ESP_LOGI(TAG, "BLE 已断开连接, 原因=%d", event->disconnect.reason);
        s_conn_handle = BLE_HS_CONN_HANDLE_NONE;
        if (s_state_callback) {
            s_state_callback(false, "广播就绪，等待 Mac 连接…");
        }
        ble_server_advertise();
        return 0;

    case BLE_GAP_EVENT_ADV_COMPLETE:
        ESP_LOGI(TAG, "BLE 广播结束，重新开启...");
        ble_server_advertise();
        return 0;

    case BLE_GAP_EVENT_MTU:
        ESP_LOGI(TAG, "BLE MTU 更新: conn_handle=%d, mtu=%d",
                 event->mtu.conn_handle, event->mtu.value);
        return 0;

    default:
        return 0;
    }
}

static void ble_server_advertise(void) {
    struct ble_gap_adv_params adv_params;
    struct ble_hs_adv_fields adv_fields;
    struct ble_hs_adv_fields rsp_fields;
    int rc;

    /* 1. Advertising Payload (<= 31 bytes): Flags + Complete Device Name */
    memset(&adv_fields, 0, sizeof(adv_fields));
    adv_fields.flags = BLE_HS_ADV_F_DISC_GEN | BLE_HS_ADV_F_BREDR_UNSUP;
    adv_fields.name = (uint8_t *)s_device_name;
    adv_fields.name_len = strlen(s_device_name);
    adv_fields.name_is_complete = 1;

    rc = ble_gap_adv_set_fields(&adv_fields);
    if (rc != 0) {
        ESP_LOGE(TAG, "设置 BLE 广播字段失败: rc=%d", rc);
        return;
    }

    /* 2. Scan Response Payload (<= 31 bytes): 128-bit Nordic UART Service UUID */
    memset(&rsp_fields, 0, sizeof(rsp_fields));
    rsp_fields.uuids128 = (ble_uuid128_t[]) { s_svc_uart_uuid };
    rsp_fields.num_uuids128 = 1;
    rsp_fields.uuids128_is_complete = 1;

    rc = ble_gap_adv_rsp_set_fields(&rsp_fields);
    if (rc != 0) {
        ESP_LOGE(TAG, "设置 BLE 扫描响应字段失败: rc=%d", rc);
        return;
    }

    memset(&adv_params, 0, sizeof(adv_params));
    adv_params.conn_mode = BLE_GAP_CONN_MODE_UND;
    adv_params.disc_mode = BLE_GAP_DISC_MODE_GEN;
    adv_params.itvl_min = BLE_GAP_ADV_FAST_INTERVAL1_MIN;
    adv_params.itvl_max = BLE_GAP_ADV_FAST_INTERVAL1_MAX;

    rc = ble_gap_adv_start(s_own_addr_type, NULL, BLE_HS_FOREVER,
                           &adv_params, ble_gap_event, NULL);
    if (rc != 0) {
        ESP_LOGE(TAG, "开启 BLE 广播失败: rc=%d", rc);
    } else {
        ESP_LOGI(TAG, "BLE 广播已开启，设备名: [%s]", s_device_name);
    }
}

static void ble_on_sync(void) {
    int rc = ble_hs_util_ensure_addr(0);
    if (rc != 0) {
        ESP_LOGE(TAG, "确保蓝牙地址失败: rc=%d", rc);
        return;
    }

    rc = ble_hs_id_infer_auto(0, &s_own_addr_type);
    if (rc != 0) {
        ESP_LOGE(TAG, "推断蓝牙地址类型失败: rc=%d", rc);
        return;
    }

    uint8_t addr_val[6] = {0};
    rc = ble_hs_id_copy_addr(s_own_addr_type, addr_val, NULL);
    if (rc == 0) {
        ESP_LOGI(TAG, "BLE 本地 MAC 地址: %02x:%02x:%02x:%02x:%02x:%02x (type=%d)",
                 addr_val[5], addr_val[4], addr_val[3],
                 addr_val[2], addr_val[1], addr_val[0], s_own_addr_type);
    }

    ble_server_advertise();
}

static void ble_on_reset(int reason) {
    ESP_LOGW(TAG, "NimBLE 协议栈复位，reason=%d", reason);
}

static void ble_host_task(void *param) {
    ESP_LOGI(TAG, "NimBLE Host Task 启动");
    nimble_port_run();
    nimble_port_freertos_deinit();
}

bool ble_server_init(const char *device_name, ble_data_rx_cb_t rx_cb, ble_conn_state_cb_t state_cb) {
    if (device_name && strlen(device_name) > 0) {
        strncpy(s_device_name, device_name, sizeof(s_device_name) - 1);
        s_device_name[sizeof(s_device_name) - 1] = '\0';
    }

    s_rx_callback = rx_cb;
    s_state_callback = state_cb;

    esp_err_t ret = nimble_port_init();
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "nimble_port_init 失败: %d", ret);
        return false;
    }

    ble_hs_cfg.reset_cb = ble_on_reset;
    ble_hs_cfg.sync_cb = ble_on_sync;

    ble_svc_gap_init();
    ble_svc_gatt_init();

    int rc = ble_gatts_count_cfg(s_gatt_svcs);
    if (rc != 0) {
        ESP_LOGE(TAG, "ble_gatts_count_cfg 失败: %d", rc);
        return false;
    }

    rc = ble_gatts_add_svcs(s_gatt_svcs);
    if (rc != 0) {
        ESP_LOGE(TAG, "ble_gatts_add_svcs 失败: %d", rc);
        return false;
    }

    rc = ble_svc_gap_device_name_set(s_device_name);
    if (rc != 0) {
        ESP_LOGE(TAG, "设置 GAP 设备名失败: %d", rc);
        return false;
    }

    nimble_port_freertos_init(ble_host_task);
    ESP_LOGI(TAG, "BLE 服务端初始化就绪");
    return true;
}
