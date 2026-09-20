#pragma once

#include "lvgl.h"
#include "model/sync_payload.h"
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    UI_BT_STATE_OFF,
    UI_BT_STATE_ADVERTISING,
    UI_BT_STATE_CONNECTED,
    UI_BT_STATE_ERROR
} ui_bt_state_t;

/**
 * @brief Initialize all LVGL UI objects for AgentRing dashboard
 */
void ui_dashboard_init(void);

/**
 * @brief Update connection status banner
 * @param state Connection state
 * @param detail Descriptive message
 */
void ui_dashboard_set_bt_status(ui_bt_state_t state, const char *detail);

/**
 * @brief Update device Bluetooth local name display
 */
void ui_dashboard_set_device_name(const char *name);

/**
 * @brief Update dashboard with new sync payload
 */
void ui_dashboard_update_payload(const sync_payload_t *payload);

#ifdef __cplusplus
}
#endif
