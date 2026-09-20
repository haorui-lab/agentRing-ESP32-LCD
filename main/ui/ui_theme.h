#pragma once

#include "lvgl.h"
#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    lv_color_t primary;
    lv_color_t secondary;
} provider_theme_t;

// Standard UI Colors
#define COLOR_BG            lv_color_hex(0xFFFFFF)
#define COLOR_CARD_BG       lv_color_hex(0xF8F9FA)
#define COLOR_TRACK         lv_color_hex(0xEDF0F5)
#define COLOR_CAPSULE_BG    lv_color_hex(0xECEEF1)
#define COLOR_DIVIDER       lv_color_hex(0xE5E5EA)

#define COLOR_TEXT_MAIN     lv_color_hex(0x1D1D1F)
#define COLOR_TEXT_MUTED    lv_color_hex(0x6E6E73)
#define COLOR_TEXT_LIGHT    lv_color_hex(0x8E8E93)

// Status colors
#define COLOR_STATUS_GREEN  lv_color_hex(0x34C759)
#define COLOR_STATUS_BLUE   lv_color_hex(0x007AFF)
#define COLOR_STATUS_RED    lv_color_hex(0xFF3B30)
#define COLOR_STATUS_ORANGE lv_color_hex(0xFF9500)

/**
 * @brief Get colors for a given provider ID
 */
provider_theme_t ui_theme_get_provider_color(const char *provider_id, bool has_secondary);

#ifdef __cplusplus
}
#endif
