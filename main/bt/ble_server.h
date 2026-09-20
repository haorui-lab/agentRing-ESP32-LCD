#pragma once

#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef void (*ble_data_rx_cb_t)(const uint8_t *data, size_t len);
typedef void (*ble_conn_state_cb_t)(bool connected, const char *detail);

/**
 * @brief Initialize BLE GATT Peripheral server (Nordic UART Service)
 * 
 * @param device_name BLE Advertising name (e.g. "AgentRing-ESP32-LCD")
 * @param rx_cb Callback when bytes are received from Mac via BLE
 * @param state_cb Callback when connection state changes
 * @return true on success, false on failure
 */
bool ble_server_init(const char *device_name, ble_data_rx_cb_t rx_cb, ble_conn_state_cb_t state_cb);

#ifdef __cplusplus
}
#endif
