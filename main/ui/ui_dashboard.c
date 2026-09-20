#include "ui_dashboard.h"
#include "ui_theme.h"
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <sys/time.h>

LV_FONT_DECLARE(ui_font_chinese_16);
LV_FONT_DECLARE(ui_font_chinese_22);

static lv_obj_t *s_root = NULL;

// Top Bar widgets
static lv_obj_t *s_top_bar = NULL;
static lv_obj_t *s_status_dot = NULL;
static lv_obj_t *s_status_label = NULL;
static lv_obj_t *s_device_name_label = NULL;
static lv_obj_t *s_updated_label = NULL;

// Bottom Bar widgets
static lv_obj_t *s_bottom_bar = NULL;
static lv_obj_t *s_sync_info_label = NULL;

// Content area
static lv_obj_t *s_empty_state = NULL;
static lv_obj_t *s_columns_cont = NULL;

// Cached payload
static sync_payload_t s_cached_payload;
static bool s_has_cached_payload = false;

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

    // Right label: Last Updated
    s_updated_label = lv_label_create(s_top_bar);
    lv_label_set_text(s_updated_label, "尚未同步");
    lv_obj_set_style_text_color(s_updated_label, COLOR_TEXT_LIGHT, LV_PART_MAIN);
    lv_obj_set_style_text_font(s_updated_label, &ui_font_chinese_16, LV_PART_MAIN);
    lv_obj_align(s_updated_label, LV_ALIGN_RIGHT_MID, 0, 0);
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

    lv_obj_t *desc = lv_label_create(card);
    lv_label_set_text(desc, "请在 Mac 状态栏打开 AgentRing 并启用蓝牙副屏同步\n设备广播名: AgentRing-ESP32-LCD");
    lv_obj_set_style_text_font(desc, &ui_font_chinese_16, LV_PART_MAIN);
    lv_obj_set_style_text_color(desc, COLOR_TEXT_MUTED, LV_PART_MAIN);
    lv_obj_set_style_text_align(desc, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN);
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

void ui_dashboard_init(void) {
    if (s_root) return;

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
    if (s_device_name_label && name) {
        lv_label_set_text(s_device_name_label, name);
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
