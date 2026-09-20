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

// Standard UI Colors (Matching agentRing-Android colors.xml & macOS Light Window Style)
#define COLOR_BG            lv_color_hex(0xF5F6F8)
#define COLOR_CARD_BG       lv_color_hex(0xFFFFFF)
#define COLOR_TRACK         lv_color_hex(0xEDF0F5)
#define COLOR_CAPSULE_BG    lv_color_hex(0xECEEF1)
#define COLOR_DIVIDER       lv_color_hex(0xE2E5E9)
#define COLOR_ROW_DIVIDER   lv_color_hex(0xECEEF1)

#define COLOR_COLUMN_TITLE  lv_color_hex(0x4B5563)
#define COLOR_TEXT_PRIMARY  lv_color_hex(0x111827)
#define COLOR_TEXT_SECONDARY lv_color_hex(0x6B7280)
#define COLOR_TEXT_TERTIARY lv_color_hex(0x9CA3AF)

// Compatibility aliases
#define COLOR_TEXT_MAIN     COLOR_TEXT_PRIMARY
#define COLOR_TEXT_MUTED    COLOR_TEXT_SECONDARY
#define COLOR_TEXT_LIGHT    COLOR_TEXT_TERTIARY

// Urgency state colors
#define COLOR_URGENCY_NORMAL   lv_color_hex(0x111827)
#define COLOR_URGENCY_WARNING  lv_color_hex(0xEA580C)
#define COLOR_URGENCY_CRITICAL lv_color_hex(0xDC2626)

// Status colors
#define COLOR_STATUS_GREEN  lv_color_hex(0x22C55E)
#define COLOR_STATUS_BLUE   lv_color_hex(0x3B82F6)
#define COLOR_STATUS_RED    lv_color_hex(0xEF4444)
#define COLOR_STATUS_YELLOW lv_color_hex(0xEAB308)
#define COLOR_STATUS_ORANGE lv_color_hex(0xEA580C)

/**
 * @brief Get colors for a given provider ID
 */
provider_theme_t ui_theme_get_provider_color(const char *provider_id, bool has_secondary);

/**
 * @brief Get text color based on remaining percent urgency
 */
lv_color_t ui_theme_get_urgency_color(double percent);

#ifdef __cplusplus
}
#endif
