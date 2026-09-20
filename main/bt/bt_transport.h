#pragma once

#include <stdint.h>
#include <stdbool.h>
#include "model/sync_payload.h"
#include "ui/ui_dashboard.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef void (*bt_payload_cb_t)(const sync_payload_t *payload);
typedef void (*bt_state_cb_t)(ui_bt_state_t state, const char *detail);

typedef struct {
    const char *device_name;      // e.g. "AgentRing-ESP32"
    bt_payload_cb_t on_payload;
    bt_state_cb_t on_state;
} bt_transport_config_t;

/**
 * @brief Initialize Bluetooth transport service (BLE GATT / SPP / Stream Buffer)
 */
bool bt_transport_init(const bt_transport_config_t *config);

/**
 * @brief Feed incoming raw bytes into the stream framing buffer
 */
void bt_transport_feed_bytes(const uint8_t *data, size_t len);

/**
 * @brief Inject demo payload for quick display verification
 */
void bt_transport_inject_demo_data(void);

/**
 * @brief Periodic maintenance / watchdog tick
 */
void bt_transport_tick(void);

#ifdef __cplusplus
}
#endif
