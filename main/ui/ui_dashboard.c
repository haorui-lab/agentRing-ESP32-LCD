#include "ui_dashboard.h"
#include "ui_theme.h"
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <sys/time.h>
#include "nvs_flash.h"
#include "nvs.h"
#include "bsp/display.h"

LV_FONT_DECLARE(ui_font_chinese_16);
LV_FONT_DECLARE(ui_font_chinese_22);

static lv_obj_t *s_root = NULL;

// Top Bar widgets
static lv_obj_t *s_top_bar = NULL;
static lv_obj_t *s_status_dot = NULL;
static lv_obj_t *s_status_label = NULL;
static lv_obj_t *s_device_name_label = NULL;
static lv_obj_t *s_datetime_label = NULL;
static lv_obj_t *s_updated_label = NULL;
static lv_obj_t *s_brightness_btn = NULL;
static lv_obj_t *s_brightness_btn_label = NULL;
static lv_timer_t *s_clock_timer = NULL;


// Brightness Dropdown Panel widgets
static lv_obj_t *s_backdrop = NULL;
static lv_obj_t *s_brightness_panel = NULL;
static lv_obj_t *s_brightness_slider = NULL;
static lv_obj_t *s_brightness_val_label = NULL;
static lv_obj_t *s_preset_btns[4] = {NULL};
static int s_current_brightness = 100;

// Bottom Bar widgets
static lv_obj_t *s_bottom_bar = NULL;
static lv_obj_t *s_sync_info_label = NULL;

// Content area
static lv_obj_t *s_empty_state = NULL;
static lv_obj_t *s_empty_desc = NULL;
static lv_obj_t *s_columns_cont = NULL;

// Cached payload
static sync_payload_t s_cached_payload;
static bool s_has_cached_payload = false;

static const int PRESET_VALUES[4] = {25, 50, 75, 100};
static const char *PRESET_LABELS[4] = {"25% 低亮", "50% 中亮", "75% 高亮", "100% 极亮"};

static void save_brightness_to_nvs(int val) {
    nvs_handle_t handle;
    if (nvs_open("settings", NVS_READWRITE, &handle) == ESP_OK) {
        nvs_set_u8(handle, "brightness", (uint8_t)val);
        nvs_commit(handle);
        nvs_close(handle);
    }
}

int ui_dashboard_get_brightness(void) {
    nvs_handle_t handle;
    uint8_t val = 100;
    if (nvs_open("settings", NVS_READONLY, &handle) == ESP_OK) {
        nvs_get_u8(handle, "brightness", &val);
        nvs_close(handle);
    }
    if (val < 10) val = 10;
    if (val > 100) val = 100;
    s_current_brightness = val;
    return s_current_brightness;
}

static void update_preset_buttons_highlight(int current_val) {
    for (int i = 0; i < 4; i++) {
        if (!s_preset_btns[i]) continue;
        bool is_active = (current_val == PRESET_VALUES[i]);
        if (is_active) {
            lv_obj_set_style_bg_color(s_preset_btns[i], COLOR_STATUS_BLUE, LV_PART_MAIN);
            lv_obj_set_style_border_color(s_preset_btns[i], COLOR_STATUS_BLUE, LV_PART_MAIN);
            lv_obj_t *label = lv_obj_get_child(s_preset_btns[i], 0);
            if (label) lv_obj_set_style_text_color(label, lv_color_hex(0xFFFFFF), LV_PART_MAIN);
        } else {
            lv_obj_set_style_bg_color(s_preset_btns[i], COLOR_CAPSULE_BG, LV_PART_MAIN);
            lv_obj_set_style_border_color(s_preset_btns[i], COLOR_DIVIDER, LV_PART_MAIN);
            lv_obj_t *label = lv_obj_get_child(s_preset_btns[i], 0);
            if (label) lv_obj_set_style_text_color(label, COLOR_TEXT_SECONDARY, LV_PART_MAIN);
        }
    }
}

void ui_dashboard_set_brightness(int val, bool save_nvs) {
    if (val < 10) val = 10;
    if (val > 100) val = 100;
    s_current_brightness = val;

    bsp_display_brightness_set(val);

    if (save_nvs) {
        save_brightness_to_nvs(val);
    }

    if (s_brightness_btn_label) {
        char buf[32];
        snprintf(buf, sizeof(buf), "亮度 %d%%", val);
        lv_label_set_text(s_brightness_btn_label, buf);
    }
    if (s_brightness_val_label) {
        char buf[16];
        snprintf(buf, sizeof(buf), "%d%%", val);
        lv_label_set_text(s_brightness_val_label, buf);
    }
    if (s_brightness_slider && lv_slider_get_value(s_brightness_slider) != val) {
        lv_slider_set_value(s_brightness_slider, val, LV_ANIM_OFF);
    }
    update_preset_buttons_highlight(val);
}

static void ui_dashboard_show_brightness_panel(void) {
    if (!s_brightness_panel || !s_backdrop) return;
    lv_obj_move_foreground(s_backdrop);
    lv_obj_move_foreground(s_brightness_panel);
    lv_obj_clear_flag(s_backdrop, LV_OBJ_FLAG_HIDDEN);
    lv_obj_clear_flag(s_brightness_panel, LV_OBJ_FLAG_HIDDEN);
}

static void ui_dashboard_hide_brightness_panel(void) {
    if (!s_brightness_panel || !s_backdrop) return;
    lv_obj_add_flag(s_brightness_panel, LV_OBJ_FLAG_HIDDEN);
    lv_obj_add_flag(s_backdrop, LV_OBJ_FLAG_HIDDEN);
}

static void ui_dashboard_toggle_brightness_panel(void) {
    if (!s_brightness_panel) return;
    if (lv_obj_has_flag(s_brightness_panel, LV_OBJ_FLAG_HIDDEN)) {
        ui_dashboard_show_brightness_panel();
    } else {
        ui_dashboard_hide_brightness_panel();
    }
}

static void brightness_btn_click_cb(lv_event_t *e) {
    ui_dashboard_toggle_brightness_panel();
}

static void top_bar_click_cb(lv_event_t *e) {
    lv_obj_t *target = lv_event_get_target(e);
    if (target == s_top_bar || target == s_datetime_label) {
        ui_dashboard_toggle_brightness_panel();
    }
}

static void backdrop_click_cb(lv_event_t *e) {
    ui_dashboard_hide_brightness_panel();
}

static void slider_event_cb(lv_event_t *e) {
    lv_event_code_t code = lv_event_get_code(e);
    lv_obj_t *slider = lv_event_get_target(e);
    int val = lv_slider_get_value(slider);
    if (code == LV_EVENT_VALUE_CHANGED) {
        ui_dashboard_set_brightness(val, false);
    } else if (code == LV_EVENT_RELEASED) {
        ui_dashboard_set_brightness(val, true);
    }
}

static void preset_btn_click_cb(lv_event_t *e) {
    int val = (int)(intptr_t)lv_event_get_user_data(e);
    ui_dashboard_set_brightness(val, true);
}

static void close_btn_click_cb(lv_event_t *e) {
    ui_dashboard_hide_brightness_panel();
}

static const char *WEEKDAY_NAMES[] = {
    "星期日", "星期一", "星期二", "星期三", "星期四", "星期五", "星期六"
};

static void update_datetime_display(void) {
    if (!s_datetime_label) return;

    time_t now = time(NULL);
    if (now < 1704067200LL) {
        lv_label_set_text(s_datetime_label, "等待时钟同步…");
        return;
    }

    struct tm timeinfo;
    localtime_r(&now, &timeinfo);

    int wday = timeinfo.tm_wday;
    if (wday < 0 || wday > 6) wday = 0;

    char dt_buf[64];
    snprintf(dt_buf, sizeof(dt_buf), "%04d年%d月%d日 %s %02d:%02d:%02d",
             timeinfo.tm_year + 1900,
             timeinfo.tm_mon + 1,
             timeinfo.tm_mday,
             WEEKDAY_NAMES[wday],
             timeinfo.tm_hour,
             timeinfo.tm_min,
             timeinfo.tm_sec);

    lv_label_set_text(s_datetime_label, dt_buf);
}

static void clock_timer_cb(lv_timer_t *timer) {
    update_datetime_display();
}

static void create_top_bar(lv_obj_t *parent) {
    s_top_bar = lv_obj_create(parent);
    lv_obj_set_size(s_top_bar, 1024, 44);
    lv_obj_set_pos(s_top_bar, 0, 0);
    lv_obj_set_style_bg_color(s_top_bar, COLOR_CARD_BG, LV_PART_MAIN);
    lv_obj_set_style_border_side(s_top_bar, LV_BORDER_SIDE_BOTTOM, LV_PART_MAIN);
    lv_obj_set_style_border_color(s_top_bar, COLOR_DIVIDER, LV_PART_MAIN);
    lv_obj_set_style_border_width(s_top_bar, 1, LV_PART_MAIN);
    lv_obj_set_style_radius(s_top_bar, 0, LV_PART_MAIN);
    lv_obj_set_style_pad_hor(s_top_bar, 24, LV_PART_MAIN);
    lv_obj_set_style_pad_ver(s_top_bar, 6, LV_PART_MAIN);
    lv_obj_clear_flag(s_top_bar, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_event_cb(s_top_bar, top_bar_click_cb, LV_EVENT_CLICKED, NULL);

    // Left container: dot + status + device name
    lv_obj_t *left_cont = lv_obj_create(s_top_bar);
    lv_obj_set_size(left_cont, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
    lv_obj_set_style_bg_opa(left_cont, LV_OPA_TRANSP, LV_PART_MAIN);
    lv_obj_set_style_border_width(left_cont, 0, LV_PART_MAIN);
    lv_obj_set_style_pad_all(left_cont, 0, LV_PART_MAIN);
    lv_obj_set_flex_flow(left_cont, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(left_cont, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_gap(left_cont, 8, LV_PART_MAIN);
    lv_obj_clear_flag(left_cont, LV_OBJ_FLAG_SCROLLABLE);


    // Status Dot
    s_status_dot = lv_obj_create(left_cont);
    lv_obj_set_size(s_status_dot, 10, 10);
    lv_obj_set_style_radius(s_status_dot, LV_RADIUS_CIRCLE, LV_PART_MAIN);
    lv_obj_set_style_bg_color(s_status_dot, COLOR_STATUS_BLUE, LV_PART_MAIN);
    lv_obj_set_style_border_width(s_status_dot, 0, LV_PART_MAIN);
    lv_obj_clear_flag(s_status_dot, LV_OBJ_FLAG_SCROLLABLE);

    // Status Label
    s_status_label = lv_label_create(left_cont);
    lv_label_set_text(s_status_label, "等待连接...");
    lv_obj_set_style_text_color(s_status_label, COLOR_TEXT_MUTED, LV_PART_MAIN);
    lv_obj_set_style_text_font(s_status_label, &ui_font_chinese_16, LV_PART_MAIN);

    // Divider bar
    lv_obj_t *sep = lv_label_create(left_cont);
    lv_label_set_text(sep, "•");
    lv_obj_set_style_text_color(sep, COLOR_TEXT_LIGHT, LV_PART_MAIN);

    // Device Name
    s_device_name_label = lv_label_create(left_cont);
    lv_label_set_text(s_device_name_label, "AgentRing-ESP32-LCD");
    lv_obj_set_style_text_color(s_device_name_label, COLOR_TEXT_MAIN, LV_PART_MAIN);
    lv_obj_set_style_text_font(s_device_name_label, &lv_font_montserrat_16, LV_PART_MAIN);

    // Center label: Real-time Date and Time
    s_datetime_label = lv_label_create(s_top_bar);
    lv_obj_align(s_datetime_label, LV_ALIGN_CENTER, 0, 0);
    lv_obj_set_style_text_font(s_datetime_label, &ui_font_chinese_16, LV_PART_MAIN);
    lv_obj_set_style_text_color(s_datetime_label, COLOR_TEXT_PRIMARY, LV_PART_MAIN);
    update_datetime_display();

    // Right container: Last Updated + Brightness Pill Button
    lv_obj_t *right_cont = lv_obj_create(s_top_bar);
    lv_obj_set_size(right_cont, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
    lv_obj_set_style_bg_opa(right_cont, LV_OPA_TRANSP, LV_PART_MAIN);
    lv_obj_set_style_border_width(right_cont, 0, LV_PART_MAIN);
    lv_obj_set_style_pad_all(right_cont, 0, LV_PART_MAIN);
    lv_obj_set_flex_flow(right_cont, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(right_cont, LV_FLEX_ALIGN_END, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_gap(right_cont, 14, LV_PART_MAIN);
    lv_obj_align(right_cont, LV_ALIGN_RIGHT_MID, 0, 0);
    lv_obj_clear_flag(right_cont, LV_OBJ_FLAG_SCROLLABLE);

    // Right label: Last Updated
    s_updated_label = lv_label_create(right_cont);
    lv_label_set_text(s_updated_label, "尚未同步");
    lv_obj_set_style_text_color(s_updated_label, COLOR_TEXT_LIGHT, LV_PART_MAIN);
    lv_obj_set_style_text_font(s_updated_label, &ui_font_chinese_16, LV_PART_MAIN);

    // Brightness Pill Button
    s_brightness_btn = lv_btn_create(right_cont);
    lv_obj_set_size(s_brightness_btn, LV_SIZE_CONTENT, 28);
    lv_obj_set_style_radius(s_brightness_btn, 14, LV_PART_MAIN);
    lv_obj_set_style_bg_color(s_brightness_btn, COLOR_CAPSULE_BG, LV_PART_MAIN);
    lv_obj_set_style_border_color(s_brightness_btn, COLOR_DIVIDER, LV_PART_MAIN);
    lv_obj_set_style_border_width(s_brightness_btn, 1, LV_PART_MAIN);
    lv_obj_set_style_pad_hor(s_brightness_btn, 10, LV_PART_MAIN);
    lv_obj_set_style_pad_ver(s_brightness_btn, 2, LV_PART_MAIN);
    lv_obj_set_style_shadow_width(s_brightness_btn, 0, LV_PART_MAIN);
    lv_obj_add_event_cb(s_brightness_btn, brightness_btn_click_cb, LV_EVENT_CLICKED, NULL);

    s_brightness_btn_label = lv_label_create(s_brightness_btn);
    char init_b_str[32];
    snprintf(init_b_str, sizeof(init_b_str), "亮度 %d%%", s_current_brightness);
    lv_label_set_text(s_brightness_btn_label, init_b_str);
    lv_obj_set_style_text_font(s_brightness_btn_label, &ui_font_chinese_16, LV_PART_MAIN);
    lv_obj_set_style_text_color(s_brightness_btn_label, COLOR_TEXT_PRIMARY, LV_PART_MAIN);
    lv_obj_align(s_brightness_btn_label, LV_ALIGN_CENTER, 0, 0);
}

static void create_bottom_bar(lv_obj_t *parent) {
    s_bottom_bar = lv_obj_create(parent);
    lv_obj_set_size(s_bottom_bar, 1024, 38);
    lv_obj_set_pos(s_bottom_bar, 0, 562);
    lv_obj_set_style_bg_color(s_bottom_bar, COLOR_CARD_BG, LV_PART_MAIN);
    lv_obj_set_style_border_side(s_bottom_bar, LV_BORDER_SIDE_TOP, LV_PART_MAIN);
    lv_obj_set_style_border_color(s_bottom_bar, COLOR_DIVIDER, LV_PART_MAIN);
    lv_obj_set_style_border_width(s_bottom_bar, 1, LV_PART_MAIN);
    lv_obj_set_style_radius(s_bottom_bar, 0, LV_PART_MAIN);
    lv_obj_set_style_pad_hor(s_bottom_bar, 20, LV_PART_MAIN);
    lv_obj_set_style_pad_ver(s_bottom_bar, 6, LV_PART_MAIN);
    lv_obj_clear_flag(s_bottom_bar, LV_OBJ_FLAG_SCROLLABLE);

    s_sync_info_label = lv_label_create(s_bottom_bar);
    lv_label_set_text(s_sync_info_label, "数据源自 Mac 端 AgentRing 本地低功耗蓝牙 (BLE GATT) 实时推流 • 本地直连 • 15 秒保活同步");
    lv_obj_set_style_text_font(s_sync_info_label, &ui_font_chinese_16, LV_PART_MAIN);
    lv_obj_set_style_text_color(s_sync_info_label, COLOR_TEXT_TERTIARY, LV_PART_MAIN);
    lv_obj_align(s_sync_info_label, LV_ALIGN_CENTER, 0, 0);
}

static void create_empty_state(lv_obj_t *parent) {
    s_empty_state = lv_obj_create(parent);
    lv_obj_set_size(s_empty_state, 1024, 518);
    lv_obj_set_pos(s_empty_state, 0, 44);
    lv_obj_set_style_bg_color(s_empty_state, COLOR_BG, LV_PART_MAIN);
    lv_obj_set_style_border_width(s_empty_state, 0, LV_PART_MAIN);
    lv_obj_set_style_radius(s_empty_state, 0, LV_PART_MAIN);
    lv_obj_clear_flag(s_empty_state, LV_OBJ_FLAG_SCROLLABLE);

    // Centered card
    lv_obj_t *card = lv_obj_create(s_empty_state);
    lv_obj_set_size(card, 480, 260);
    lv_obj_align(card, LV_ALIGN_CENTER, 0, 0);
    lv_obj_set_style_bg_color(card, COLOR_CARD_BG, LV_PART_MAIN);
    lv_obj_set_style_border_color(card, COLOR_DIVIDER, LV_PART_MAIN);
    lv_obj_set_style_border_width(card, 1, LV_PART_MAIN);
    lv_obj_set_style_radius(card, 16, LV_PART_MAIN);
    lv_obj_set_style_pad_all(card, 32, LV_PART_MAIN);
    lv_obj_set_flex_flow(card, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(card, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_gap(card, 14, LV_PART_MAIN);
    lv_obj_clear_flag(card, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t *icon = lv_label_create(card);
    lv_label_set_text(icon, LV_SYMBOL_BLUETOOTH);
    lv_obj_set_style_text_font(icon, &lv_font_montserrat_36, LV_PART_MAIN);
    lv_obj_set_style_text_color(icon, COLOR_STATUS_BLUE, LV_PART_MAIN);

    lv_obj_t *title = lv_label_create(card);
    lv_label_set_text(title, "等待 AgentRing 同步");
    lv_obj_set_style_text_font(title, &ui_font_chinese_22, LV_PART_MAIN);
    lv_obj_set_style_text_color(title, COLOR_TEXT_MAIN, LV_PART_MAIN);

    s_empty_desc = lv_label_create(card);
    lv_label_set_text(s_empty_desc, "请在 Mac 状态栏打开 AgentRing 并启用蓝牙副屏同步\n设备广播名: AgentRing-LCD");
    lv_obj_set_style_text_font(s_empty_desc, &ui_font_chinese_16, LV_PART_MAIN);
    lv_obj_set_style_text_color(s_empty_desc, COLOR_TEXT_MUTED, LV_PART_MAIN);
    lv_obj_set_style_text_align(s_empty_desc, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN);
}

static void configure_arc_appearance(lv_obj_t *arc, int width, lv_color_t color, lv_color_t track_color) {
    lv_arc_set_rotation(arc, 270);
    lv_arc_set_bg_angles(arc, 0, 360);
    lv_arc_set_range(arc, 0, 100);

    // Transparent container
    lv_obj_set_style_bg_opa(arc, LV_OPA_TRANSP, LV_PART_MAIN);
    lv_obj_set_style_border_width(arc, 0, LV_PART_MAIN);
    lv_obj_set_style_pad_all(arc, 0, LV_PART_MAIN);

    // Track style (完整 360 度圆环底轨，浅灰色 #EDF0F5)
    lv_obj_set_style_arc_width(arc, width, LV_PART_MAIN);
    lv_obj_set_style_arc_color(arc, track_color, LV_PART_MAIN);
    lv_obj_set_style_arc_rounded(arc, true, LV_PART_MAIN);

    // Indicator style (彩色用量进度弧，圆角线帽)
    lv_obj_set_style_arc_width(arc, width, LV_PART_INDICATOR);
    lv_obj_set_style_arc_color(arc, color, LV_PART_INDICATOR);
    lv_obj_set_style_arc_rounded(arc, true, LV_PART_INDICATOR);

    // Remove knob completely
    lv_obj_remove_style(arc, NULL, LV_PART_KNOB);
    lv_obj_set_style_opa(arc, LV_OPA_TRANSP, LV_PART_KNOB);

    lv_obj_clear_flag(arc, LV_OBJ_FLAG_CLICKABLE);
}

static void build_provider_column(lv_obj_t *parent, const provider_data_t *provider, int index, int total_count) {
    provider_theme_t theme = ui_theme_get_provider_color(provider->id, provider->has_secondary);

    // Responsive geometry for 1024x600 IPS display
    int col_w = (total_count >= 4) ? 246 : (total_count == 3 ? 324 : (total_count == 2 ? 486 : 600));
    int ring_size = (total_count >= 4) ? 152 : (total_count == 3 ? 176 : 204);

    // 仿 macOS agentRing / Apple Watch 规范：内外环均采用饱满等宽 12%，呼吸间隔 4.5%
    int stroke_width = (int)(ring_size * 0.12f);
    int gap = (int)(ring_size * 0.045f);

    // Column container (518px height between top bar and bottom footer)
    lv_obj_t *col = lv_obj_create(parent);
    lv_obj_set_size(col, col_w, 518);
    lv_obj_set_style_bg_color(col, COLOR_BG, LV_PART_MAIN);
    lv_obj_set_style_bg_opa(col, LV_OPA_COVER, LV_PART_MAIN);
    lv_obj_set_style_border_width(col, 0, LV_PART_MAIN);
    if (index < total_count - 1) {
        lv_obj_set_style_border_side(col, LV_BORDER_SIDE_RIGHT, LV_PART_MAIN);
        lv_obj_set_style_border_color(col, COLOR_DIVIDER, LV_PART_MAIN);
        lv_obj_set_style_border_width(col, 1, LV_PART_MAIN);
    }
    lv_obj_set_style_pad_hor(col, 8, LV_PART_MAIN);
    // Vertical centering offset (pushes rings down to screen center)
    lv_obj_set_style_pad_top(col, 95, LV_PART_MAIN);
    lv_obj_set_style_pad_bottom(col, 10, LV_PART_MAIN);
    lv_obj_set_flex_flow(col, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(col, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_gap(col, 14, LV_PART_MAIN);
    lv_obj_clear_flag(col, LV_OBJ_FLAG_SCROLLABLE);

    // 1. Provider Title (仿 Android column_title: #4B5563, 居中粗体)
    lv_obj_t *name_lbl = lv_label_create(col);
    lv_label_set_text(name_lbl, provider->name);
    lv_obj_set_style_text_font(name_lbl, (total_count <= 2) ? &lv_font_montserrat_20 : &lv_font_montserrat_18, LV_PART_MAIN);
    lv_obj_set_style_text_color(name_lbl, COLOR_COLUMN_TITLE, LV_PART_MAIN);
    lv_obj_set_width(name_lbl, lv_pct(100));
    lv_obj_set_style_text_align(name_lbl, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN);
    lv_label_set_long_mode(name_lbl, LV_LABEL_LONG_DOT);

    // 2. Ring Container (Concentric Rings - 移除中央大字，纯净视觉留白)
    lv_obj_t *ring_box = lv_obj_create(col);
    lv_obj_set_size(ring_box, ring_size, ring_size);
    lv_obj_set_style_bg_opa(ring_box, LV_OPA_TRANSP, LV_PART_MAIN);
    lv_obj_set_style_border_width(ring_box, 0, LV_PART_MAIN);
    lv_obj_set_style_pad_all(ring_box, 0, LV_PART_MAIN);
    lv_obj_set_style_margin_ver(ring_box, 6, LV_PART_MAIN);
    lv_obj_clear_flag(ring_box, LV_OBJ_FLAG_SCROLLABLE);

    // Primary Arc (Outer)
    lv_obj_t *outer_arc = lv_arc_create(ring_box);
    lv_obj_set_size(outer_arc, ring_size, ring_size);
    lv_obj_align(outer_arc, LV_ALIGN_CENTER, 0, 0);
    configure_arc_appearance(outer_arc, stroke_width, theme.primary, COLOR_TRACK);

    int outer_val = provider->has_primary ? (int)provider->primary.remaining_percent : 100;
    lv_arc_set_value(outer_arc, outer_val);

    // Secondary Arc (Inner - 与外环等宽 stroke_width，严格同心)
    if (provider->has_secondary) {
        int inner_size = ring_size - 2 * (stroke_width + gap);
        lv_obj_t *inner_arc = lv_arc_create(ring_box);
        lv_obj_set_size(inner_arc, inner_size, inner_size);
        lv_obj_align(inner_arc, LV_ALIGN_CENTER, 0, 0);
        configure_arc_appearance(inner_arc, stroke_width, theme.secondary, COLOR_TRACK);

        int inner_val = (int)provider->secondary.remaining_percent;
        lv_arc_set_value(inner_arc, inner_val);
    }

    // 3. Flat Rows Container (仿 agentRing-Android 扁平列表，无臃肿卡片，行间 1px 浅灰细线)
    lv_obj_t *rows_box = lv_obj_create(col);
    lv_obj_set_width(rows_box, lv_pct(100));
    lv_obj_set_height(rows_box, LV_SIZE_CONTENT);
    lv_obj_set_style_bg_opa(rows_box, LV_OPA_TRANSP, LV_PART_MAIN);
    lv_obj_set_style_border_width(rows_box, 0, LV_PART_MAIN);
    lv_obj_set_style_pad_all(rows_box, 0, LV_PART_MAIN);
    lv_obj_set_flex_flow(rows_box, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_style_pad_gap(rows_box, 2, LV_PART_MAIN);
    lv_obj_clear_flag(rows_box, LV_OBJ_FLAG_SCROLLABLE);

    for (int r = 0; r < provider->row_count; r++) {
        const limit_row_t *row = &provider->rows[r];

        // 行间 1px 极细浅灰分割线 (仿 Android row_divider #ECEEF1)
        if (r > 0) {
            lv_obj_t *divider = lv_obj_create(rows_box);
            lv_obj_set_size(divider, lv_pct(100), 1);
            lv_obj_set_style_bg_color(divider, COLOR_ROW_DIVIDER, LV_PART_MAIN);
            lv_obj_set_style_bg_opa(divider, LV_OPA_COVER, LV_PART_MAIN);
            lv_obj_set_style_border_width(divider, 0, LV_PART_MAIN);
            lv_obj_set_style_pad_all(divider, 0, LV_PART_MAIN);
            lv_obj_set_style_margin_ver(divider, 2, LV_PART_MAIN);
            lv_obj_clear_flag(divider, LV_OBJ_FLAG_SCROLLABLE);
        }

        // 单行三通道横向对齐容器: [Label] ------------ [Percent] [Reset]
        lv_obj_t *row_cont = lv_obj_create(rows_box);
        lv_obj_set_width(row_cont, lv_pct(100));
        lv_obj_set_height(row_cont, 30);
        lv_obj_set_style_bg_opa(row_cont, LV_OPA_TRANSP, LV_PART_MAIN);
        lv_obj_set_style_border_width(row_cont, 0, LV_PART_MAIN);
        lv_obj_set_style_pad_hor(row_cont, 2, LV_PART_MAIN);
        lv_obj_set_style_pad_ver(row_cont, 0, LV_PART_MAIN);
        lv_obj_clear_flag(row_cont, LV_OBJ_FLAG_SCROLLABLE);
        lv_obj_set_flex_flow(row_cont, LV_FLEX_FLOW_ROW);
        lv_obj_set_flex_align(row_cont, LV_FLEX_ALIGN_SPACE_BETWEEN, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

        // 通道 1: 额度名称 (左对齐)
        lv_obj_t *r_lbl = lv_label_create(row_cont);
        int label_w = (total_count >= 4) ? 96 : 130;
        lv_obj_set_width(r_lbl, label_w);
        lv_label_set_long_mode(r_lbl, LV_LABEL_LONG_DOT);
        lv_label_set_text(r_lbl, row->label);
        lv_obj_set_style_text_font(r_lbl, &ui_font_chinese_16, LV_PART_MAIN);
        lv_obj_set_style_text_color(r_lbl, COLOR_TEXT_SECONDARY, LV_PART_MAIN);

        // 通道 2: 百分比 (加粗、右对齐、告急变色)
        lv_obj_t *p_lbl = lv_label_create(row_cont);
        lv_obj_set_width(p_lbl, 50);
        lv_obj_set_style_text_align(p_lbl, LV_TEXT_ALIGN_RIGHT, LV_PART_MAIN);

        char p_str[32];
        double p_val = 100.0;
        if (r == 0 && provider->has_primary) {
            p_val = provider->primary.remaining_percent;
            snprintf(p_str, sizeof(p_str), "%d%%", (int)p_val);
        } else if (r == 1 && provider->has_secondary) {
            p_val = provider->secondary.remaining_percent;
            snprintf(p_str, sizeof(p_str), "%d%%", (int)p_val);
        } else {
            p_val = atof(row->percent);
            snprintf(p_str, sizeof(p_str), "%s", row->percent);
        }
        lv_label_set_text(p_lbl, p_str);
        lv_obj_set_style_text_font(p_lbl, &lv_font_montserrat_16, LV_PART_MAIN);
        lv_obj_set_style_text_color(p_lbl, ui_theme_get_urgency_color(p_val), LV_PART_MAIN);

        // 通道 3: 重置时间 / 计费说明 (固定通道右对齐)
        lv_obj_t *rst_lbl = lv_label_create(row_cont);
        int rst_w = (total_count >= 4) ? 72 : 90;
        lv_obj_set_width(rst_lbl, rst_w);
        lv_obj_set_style_text_align(rst_lbl, LV_TEXT_ALIGN_RIGHT, LV_PART_MAIN);
        lv_label_set_long_mode(rst_lbl, LV_LABEL_LONG_DOT);
        lv_label_set_text(rst_lbl, row->reset);
        lv_obj_set_style_text_font(rst_lbl, &ui_font_chinese_16, LV_PART_MAIN);
        lv_obj_set_style_text_color(rst_lbl, COLOR_TEXT_SECONDARY, LV_PART_MAIN);
    }
}

static void create_brightness_dropdown(lv_obj_t *parent) {
    // 1. Semi-transparent backdrop to capture outside taps
    s_backdrop = lv_obj_create(parent);
    lv_obj_set_size(s_backdrop, 1024, 600);
    lv_obj_set_pos(s_backdrop, 0, 0);
    lv_obj_set_style_bg_color(s_backdrop, lv_color_black(), LV_PART_MAIN);
    lv_obj_set_style_bg_opa(s_backdrop, LV_OPA_20, LV_PART_MAIN);
    lv_obj_set_style_border_width(s_backdrop, 0, LV_PART_MAIN);
    lv_obj_set_style_radius(s_backdrop, 0, LV_PART_MAIN);
    lv_obj_clear_flag(s_backdrop, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_event_cb(s_backdrop, backdrop_click_cb, LV_EVENT_CLICKED, NULL);
    lv_obj_add_flag(s_backdrop, LV_OBJ_FLAG_HIDDEN);

    // 2. Control Panel Card (Apple Control Center style)
    s_brightness_panel = lv_obj_create(parent);
    lv_obj_set_size(s_brightness_panel, 460, 204);
    lv_obj_set_pos(s_brightness_panel, 282, 48); // Centered horizontally below top bar
    lv_obj_set_style_bg_color(s_brightness_panel, COLOR_CARD_BG, LV_PART_MAIN);
    lv_obj_set_style_border_color(s_brightness_panel, COLOR_DIVIDER, LV_PART_MAIN);
    lv_obj_set_style_border_width(s_brightness_panel, 1, LV_PART_MAIN);
    lv_obj_set_style_radius(s_brightness_panel, 16, LV_PART_MAIN);
    lv_obj_set_style_shadow_color(s_brightness_panel, lv_color_hex(0x000000), LV_PART_MAIN);
    lv_obj_set_style_shadow_width(s_brightness_panel, 30, LV_PART_MAIN);
    lv_obj_set_style_shadow_opa(s_brightness_panel, LV_OPA_10, LV_PART_MAIN);
    lv_obj_set_style_shadow_offset_y(s_brightness_panel, 6, LV_PART_MAIN);
    lv_obj_set_style_pad_hor(s_brightness_panel, 20, LV_PART_MAIN);
    lv_obj_set_style_pad_ver(s_brightness_panel, 16, LV_PART_MAIN);
    lv_obj_set_flex_flow(s_brightness_panel, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_style_pad_gap(s_brightness_panel, 14, LV_PART_MAIN);
    lv_obj_clear_flag(s_brightness_panel, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_flag(s_brightness_panel, LV_OBJ_FLAG_HIDDEN);

    // Header Row: Title + Percentage Badge + Close Button
    lv_obj_t *header_row = lv_obj_create(s_brightness_panel);
    lv_obj_set_size(header_row, lv_pct(100), 28);
    lv_obj_set_style_bg_opa(header_row, LV_OPA_TRANSP, LV_PART_MAIN);
    lv_obj_set_style_border_width(header_row, 0, LV_PART_MAIN);
    lv_obj_set_style_pad_all(header_row, 0, LV_PART_MAIN);
    lv_obj_set_flex_flow(header_row, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(header_row, LV_FLEX_ALIGN_SPACE_BETWEEN, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_clear_flag(header_row, LV_OBJ_FLAG_SCROLLABLE);

    // Header Left: "屏幕亮度调节"
    lv_obj_t *title = lv_label_create(header_row);
    lv_label_set_text(title, "屏幕亮度调节");
    lv_obj_set_style_text_font(title, &ui_font_chinese_16, LV_PART_MAIN);
    lv_obj_set_style_text_color(title, COLOR_TEXT_PRIMARY, LV_PART_MAIN);

    // Header Right Container: Badge + Close
    lv_obj_t *hdr_right = lv_obj_create(header_row);
    lv_obj_set_size(hdr_right, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
    lv_obj_set_style_bg_opa(hdr_right, LV_OPA_TRANSP, LV_PART_MAIN);
    lv_obj_set_style_border_width(hdr_right, 0, LV_PART_MAIN);
    lv_obj_set_style_pad_all(hdr_right, 0, LV_PART_MAIN);
    lv_obj_set_flex_flow(hdr_right, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(hdr_right, LV_FLEX_ALIGN_END, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_gap(hdr_right, 10, LV_PART_MAIN);
    lv_obj_clear_flag(hdr_right, LV_OBJ_FLAG_SCROLLABLE);

    s_brightness_val_label = lv_label_create(hdr_right);
    char buf[16];
    snprintf(buf, sizeof(buf), "%d%%", s_current_brightness);
    lv_label_set_text(s_brightness_val_label, buf);
    lv_obj_set_style_text_font(s_brightness_val_label, &lv_font_montserrat_16, LV_PART_MAIN);
    lv_obj_set_style_text_color(s_brightness_val_label, COLOR_STATUS_BLUE, LV_PART_MAIN);

    lv_obj_t *close_btn = lv_btn_create(hdr_right);
    lv_obj_set_size(close_btn, 24, 24);
    lv_obj_set_style_radius(close_btn, LV_RADIUS_CIRCLE, LV_PART_MAIN);
    lv_obj_set_style_bg_color(close_btn, COLOR_CAPSULE_BG, LV_PART_MAIN);
    lv_obj_set_style_border_width(close_btn, 0, LV_PART_MAIN);
    lv_obj_set_style_pad_all(close_btn, 0, LV_PART_MAIN);
    lv_obj_set_style_shadow_width(close_btn, 0, LV_PART_MAIN);
    lv_obj_add_event_cb(close_btn, close_btn_click_cb, LV_EVENT_CLICKED, NULL);

    lv_obj_t *close_lbl = lv_label_create(close_btn);
    lv_label_set_text(close_lbl, "✕");
    lv_obj_set_style_text_font(close_lbl, &ui_font_chinese_16, LV_PART_MAIN);
    lv_obj_set_style_text_color(close_lbl, COLOR_TEXT_SECONDARY, LV_PART_MAIN);
    lv_obj_align(close_lbl, LV_ALIGN_CENTER, 0, 0);

    // Slider Row
    s_brightness_slider = lv_slider_create(s_brightness_panel);
    lv_obj_set_size(s_brightness_slider, lv_pct(100), 22);
    lv_slider_set_range(s_brightness_slider, 10, 100);
    lv_slider_set_value(s_brightness_slider, s_current_brightness, LV_ANIM_OFF);
    lv_obj_set_style_radius(s_brightness_slider, 11, LV_PART_MAIN);
    lv_obj_set_style_bg_color(s_brightness_slider, COLOR_TRACK, LV_PART_MAIN);

    lv_obj_set_style_radius(s_brightness_slider, 11, LV_PART_INDICATOR);
    lv_obj_set_style_bg_color(s_brightness_slider, COLOR_STATUS_BLUE, LV_PART_INDICATOR);

    lv_obj_set_style_radius(s_brightness_slider, LV_RADIUS_CIRCLE, LV_PART_KNOB);
    lv_obj_set_style_bg_color(s_brightness_slider, COLOR_CARD_BG, LV_PART_KNOB);
    lv_obj_set_style_border_color(s_brightness_slider, COLOR_STATUS_BLUE, LV_PART_KNOB);
    lv_obj_set_style_border_width(s_brightness_slider, 3, LV_PART_KNOB);
    lv_obj_set_style_pad_all(s_brightness_slider, 4, LV_PART_KNOB);
    lv_obj_add_event_cb(s_brightness_slider, slider_event_cb, LV_EVENT_ALL, NULL);

    // Preset Buttons Row
    lv_obj_t *preset_row = lv_obj_create(s_brightness_panel);
    lv_obj_set_size(preset_row, lv_pct(100), 40);
    lv_obj_set_style_bg_opa(preset_row, LV_OPA_TRANSP, LV_PART_MAIN);
    lv_obj_set_style_border_width(preset_row, 0, LV_PART_MAIN);
    lv_obj_set_style_pad_all(preset_row, 0, LV_PART_MAIN);
    lv_obj_set_flex_flow(preset_row, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(preset_row, LV_FLEX_ALIGN_SPACE_BETWEEN, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_clear_flag(preset_row, LV_OBJ_FLAG_SCROLLABLE);

    for (int i = 0; i < 4; i++) {
        s_preset_btns[i] = lv_btn_create(preset_row);
        lv_obj_set_size(s_preset_btns[i], 98, 36);
        lv_obj_set_style_radius(s_preset_btns[i], 8, LV_PART_MAIN);
        lv_obj_set_style_border_width(s_preset_btns[i], 1, LV_PART_MAIN);
        lv_obj_set_style_pad_all(s_preset_btns[i], 0, LV_PART_MAIN);
        lv_obj_set_style_shadow_width(s_preset_btns[i], 0, LV_PART_MAIN);
        lv_obj_add_event_cb(s_preset_btns[i], preset_btn_click_cb, LV_EVENT_CLICKED, (void *)(intptr_t)PRESET_VALUES[i]);

        lv_obj_t *btn_lbl = lv_label_create(s_preset_btns[i]);
        lv_label_set_text(btn_lbl, PRESET_LABELS[i]);
        lv_obj_set_style_text_font(btn_lbl, &ui_font_chinese_16, LV_PART_MAIN);
        lv_obj_align(btn_lbl, LV_ALIGN_CENTER, 0, 0);
    }

    update_preset_buttons_highlight(s_current_brightness);
}

void ui_dashboard_init(void) {
    if (s_root) return;

    ui_dashboard_get_brightness();

    s_root = lv_scr_act();
    lv_obj_clean(s_root);
    lv_obj_set_size(s_root, 1024, 600);
    lv_obj_set_style_bg_color(s_root, COLOR_BG, LV_PART_MAIN);
    lv_obj_clear_flag(s_root, LV_OBJ_FLAG_SCROLLABLE);

    create_top_bar(s_root);
    create_bottom_bar(s_root);
    create_empty_state(s_root);

    // Columns Container (Between top bar 44px and bottom footer 562px)
    s_columns_cont = lv_obj_create(s_root);
    lv_obj_set_size(s_columns_cont, 1024, 518);
    lv_obj_set_pos(s_columns_cont, 0, 44);
    lv_obj_set_style_bg_color(s_columns_cont, COLOR_BG, LV_PART_MAIN);
    lv_obj_set_style_border_width(s_columns_cont, 0, LV_PART_MAIN);
    lv_obj_set_style_radius(s_columns_cont, 0, LV_PART_MAIN);
    lv_obj_set_style_pad_hor(s_columns_cont, 6, LV_PART_MAIN);
    lv_obj_set_style_pad_ver(s_columns_cont, 0, LV_PART_MAIN);
    lv_obj_set_flex_flow(s_columns_cont, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(s_columns_cont, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_gap(s_columns_cont, 8, LV_PART_MAIN);
    lv_obj_clear_flag(s_columns_cont, LV_OBJ_FLAG_SCROLLABLE);

    // Initially show empty state
    lv_obj_add_flag(s_columns_cont, LV_OBJ_FLAG_HIDDEN);
    lv_obj_clear_flag(s_empty_state, LV_OBJ_FLAG_HIDDEN);

    // Brightness Control Panel Dropdown (Top layer)
    create_brightness_dropdown(s_root);

    // 1-second Clock Timer for real-time Date & Time display
    if (!s_clock_timer) {
        s_clock_timer = lv_timer_create(clock_timer_cb, 1000, NULL);
    }
}

void ui_dashboard_set_bt_status(ui_bt_state_t state, const char *detail) {
    if (!s_status_dot || !s_status_label) return;

    switch (state) {
        case UI_BT_STATE_CONNECTED:
            lv_obj_set_style_bg_color(s_status_dot, COLOR_STATUS_GREEN, LV_PART_MAIN);
            break;
        case UI_BT_STATE_ADVERTISING:
            lv_obj_set_style_bg_color(s_status_dot, COLOR_STATUS_BLUE, LV_PART_MAIN);
            break;
        case UI_BT_STATE_OFF:
        case UI_BT_STATE_ERROR:
        default:
            lv_obj_set_style_bg_color(s_status_dot, COLOR_STATUS_RED, LV_PART_MAIN);
            break;
    }

    if (detail) {
        lv_label_set_text(s_status_label, detail);
    }
}

void ui_dashboard_set_device_name(const char *name) {
    if (name) {
        if (s_device_name_label) {
            lv_label_set_text(s_device_name_label, name);
        }
        if (s_empty_desc) {
            char desc_buf[160];
            snprintf(desc_buf, sizeof(desc_buf), "请在 Mac 状态栏打开 AgentRing 并启用蓝牙副屏同步\n设备广播名: %s", name);
            lv_label_set_text(s_empty_desc, desc_buf);
        }
    }
}

void ui_dashboard_update_payload(const sync_payload_t *payload) {
    if (!payload) return;

    // Check if identical to cached payload
    if (s_has_cached_payload && is_payload_equal(payload, &s_cached_payload)) {
        return;
    }

    s_cached_payload = *payload;
    s_has_cached_payload = true;

    // Synchronize system RTC from payload timestamp if valid (> 2024-01-01)
    if (payload->timestamp > 1704067200LL) {
        struct timeval tv = {
            .tv_sec = (time_t)payload->timestamp,
            .tv_usec = 0
        };
        settimeofday(&tv, NULL);
        update_datetime_display();
    }

    // Format updated timestamp
    time_t sync_time = (payload->timestamp > 1704067200LL) ? (time_t)payload->timestamp : time(NULL);
    struct tm timeinfo;
    localtime_r(&sync_time, &timeinfo);
    char updated_str[48];
    snprintf(updated_str, sizeof(updated_str), "最后更新 %02d:%02d:%02d",
             timeinfo.tm_hour, timeinfo.tm_min, timeinfo.tm_sec);
    lv_label_set_text(s_updated_label, updated_str);

    if (payload->provider_count == 0) {
        lv_obj_add_flag(s_columns_cont, LV_OBJ_FLAG_HIDDEN);
        lv_obj_clear_flag(s_empty_state, LV_OBJ_FLAG_HIDDEN);
        return;
    }

    lv_obj_clear_flag(s_columns_cont, LV_OBJ_FLAG_HIDDEN);
    lv_obj_add_flag(s_empty_state, LV_OBJ_FLAG_HIDDEN);

    // Rebuild columns
    lv_obj_clean(s_columns_cont);
    lv_obj_invalidate(s_columns_cont);

    for (int i = 0; i < payload->provider_count; i++) {
        build_provider_column(s_columns_cont, &payload->providers[i], i, payload->provider_count);
    }
}
