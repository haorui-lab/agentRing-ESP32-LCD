#include "ui_theme.h"
#include <string.h>

provider_theme_t ui_theme_get_provider_color(const char *provider_id, bool has_secondary) {
    provider_theme_t theme;

    if (!provider_id) {
        theme.primary = lv_color_hex(0x617FA8);
        theme.secondary = lv_color_hex(0x4E6B93);
        return theme;
    }

    if (strcmp(provider_id, "codex") == 0) {
        if (has_secondary) {
            theme.primary = lv_color_hex(0x718F66);   // 嫩绿
            theme.secondary = lv_color_hex(0x58744F); // 暗青绿
        } else {
            theme.primary = lv_color_hex(0x58744F);   // 暗青绿
            theme.secondary = lv_color_hex(0x58744F);
        }
    } else if (strcmp(provider_id, "antigravity") == 0) {
        theme.primary = lv_color_hex(0x617FA8);       // 科技蓝灰
        theme.secondary = lv_color_hex(0x4E6B93);     // 深蓝灰
    } else if (strcmp(provider_id, "antigravity_third") == 0) {
        theme.primary = lv_color_hex(0xA8785D);       // 暖陶红
        theme.secondary = lv_color_hex(0x8C5F44);     // 暗陶褐
    } else if (strcmp(provider_id, "cursor") == 0) {
        theme.primary = lv_color_hex(0xB76776);       // 玫瑰红
        theme.secondary = lv_color_hex(0x96515E);     // 暗玫瑰
    } else {
        // Fallback default
        theme.primary = lv_color_hex(0x5B6E8C);
        theme.secondary = lv_color_hex(0x44536A);
    }

    return theme;
}
