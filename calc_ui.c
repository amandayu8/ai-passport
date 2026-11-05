// main/calc_ui.c —— 计算器独立 UI 与三键交互逻辑实现。
#include "calc_ui.h"
#include "calc_engine.h"
#include "bsp_battery.h"
#include "bsp_display.h"
#include "lvgl.h"
#include <stdio.h>

#define KEY_ROWS 5
#define KEY_COLS 4
#define KEY_COUNT 20

typedef struct {
    const char *label;
    calc_key_t key;
    uint32_t bg;
    uint32_t bg_focus;
    uint32_t text;
} key_meta_t;

static const key_meta_t s_keys[KEY_COUNT] = {
    // Row 0
    { "C",   CALC_KEY_CLEAR,     0x3B252C, 0x5C2B36, 0xF87171 },
    { "DEL", CALC_KEY_BACKSPACE, 0x1E293B, 0x334155, 0x38BDF8 },
    { "%",   CALC_KEY_PERCENT,   0x1E293B, 0x334155, 0x38BDF8 },
    { "/",   CALC_KEY_DIV,       0xB45309, 0xD97706, 0xFFFFFF },

    // Row 1
    { "7",   CALC_KEY_7,         0x1E293B, 0x334155, 0xF1F5F9 },
    { "8",   CALC_KEY_8,         0x1E293B, 0x334155, 0xF1F5F9 },
    { "9",   CALC_KEY_9,         0x1E293B, 0x334155, 0xF1F5F9 },
    { "*",   CALC_KEY_MUL,       0xB45309, 0xD97706, 0xFFFFFF },

    // Row 2
    { "4",   CALC_KEY_4,         0x1E293B, 0x334155, 0xF1F5F9 },
    { "5",   CALC_KEY_5,         0x1E293B, 0x334155, 0xF1F5F9 },
    { "6",   CALC_KEY_6,         0x1E293B, 0x334155, 0xF1F5F9 },
    { "-",   CALC_KEY_SUB,       0xB45309, 0xD97706, 0xFFFFFF },

    // Row 3
    { "1",   CALC_KEY_1,         0x1E293B, 0x334155, 0xF1F5F9 },
    { "2",   CALC_KEY_2,         0x1E293B, 0x334155, 0xF1F5F9 },
    { "3",   CALC_KEY_3,         0x1E293B, 0x334155, 0xF1F5F9 },
    { "+",   CALC_KEY_ADD,       0xB45309, 0xD97706, 0xFFFFFF },

    // Row 4
    { "+/-", CALC_KEY_NEG,       0x1E293B, 0x334155, 0x38BDF8 },
    { "0",   CALC_KEY_0,         0x1E293B, 0x334155, 0xF1F5F9 },
    { ".",   CALC_KEY_DOT,       0x1E293B, 0x334155, 0xF1F5F9 },
    { "=",   CALC_KEY_EQUALS,    0x047857, 0x059669, 0xFFFFFF },
};

static calc_engine_t s_engine;
static lv_obj_t *s_scr;
static lv_obj_t *s_title_label;
static lv_obj_t *s_bat_label;
static lv_obj_t *s_card;
static lv_obj_t *s_expr_label;
static lv_obj_t *s_disp_label;
static lv_obj_t *s_btn_objs[KEY_COUNT];
static lv_obj_t *s_btn_labels[KEY_COUNT];
static lv_timer_t *s_bat_timer;
static int s_focused_index = 17; // 默认聚焦在 "0" 按键上

static void update_focus_visual(int index, bool focused) {
    if (index < 0 || index >= KEY_COUNT || !s_btn_objs[index]) return;
    lv_obj_t *btn = s_btn_objs[index];
    if (focused) {
        lv_obj_set_style_border_width(btn, 3, 0);
        lv_obj_set_style_border_color(btn, lv_color_hex(0x00F0FF), 0);
        lv_obj_set_style_bg_color(btn, lv_color_hex(s_keys[index].bg_focus), 0);
    } else {
        lv_obj_set_style_border_width(btn, 1, 0);
        lv_obj_set_style_border_color(btn, lv_color_hex(0x334155), 0);
        lv_obj_set_style_bg_color(btn, lv_color_hex(s_keys[index].bg), 0);
    }
}

static void refresh_display(void) {
    if (!s_expr_label || !s_disp_label) return;
    lv_label_set_text(s_expr_label, calc_engine_get_expr(&s_engine));
    lv_label_set_text(s_disp_label, calc_engine_get_display(&s_engine));
}

static void update_battery(lv_timer_t *t) {
    (void)t;
    if (!s_bat_label) return;
    int soc = bsp_battery_soc();
    if (soc < 0) {
        lv_label_set_text(s_bat_label, "--%");
        lv_obj_set_style_text_color(s_bat_label, lv_color_hex(0x64748B), 0);
    } else {
        lv_label_set_text_fmt(s_bat_label, "%d%%", soc);
        lv_color_t color = (soc < 20) ? lv_color_hex(0xEF4444) : lv_color_hex(0x10B981);
        lv_obj_set_style_text_color(s_bat_label, color, 0);
    }
}

void calc_ui_enter(void) {
    calc_engine_init(&s_engine);

    s_scr = lv_obj_create(NULL);
    lv_obj_set_style_bg_color(s_scr, lv_color_hex(0x0B0F17), 0);
    lv_obj_set_style_bg_opa(s_scr, LV_OPA_COVER, 0);
    lv_obj_remove_flag(s_scr, LV_OBJ_FLAG_SCROLLABLE);

    // 顶部状态栏: 标题与电池
    s_title_label = lv_label_create(s_scr);
    lv_obj_set_style_text_font(s_title_label, &lv_font_montserrat_14, 0);
    lv_obj_set_style_text_color(s_title_label, lv_color_hex(0x64748B), 0);
    lv_label_set_text(s_title_label, "CALCULATOR");
    lv_obj_set_pos(s_title_label, 12, 6);

    s_bat_label = lv_label_create(s_scr);
    lv_obj_set_style_text_font(s_bat_label, &lv_font_montserrat_14, 0);
    lv_obj_set_style_text_color(s_bat_label, lv_color_hex(0x10B981), 0);
    lv_label_set_text(s_bat_label, "--%");
    lv_obj_set_pos(s_bat_label, 192, 6);

    // 显示卡片区域 (Display Card)
    s_card = lv_obj_create(s_scr);
    lv_obj_set_pos(s_card, 10, 26);
    lv_obj_set_size(s_card, 220, 72);
    lv_obj_set_style_bg_color(s_card, lv_color_hex(0x131B26), 0);
    lv_obj_set_style_bg_opa(s_card, LV_OPA_COVER, 0);
    lv_obj_set_style_radius(s_card, 8, 0);
    lv_obj_set_style_border_width(s_card, 1, 0);
    lv_obj_set_style_border_color(s_card, lv_color_hex(0x273549), 0);
    lv_obj_set_style_pad_all(s_card, 6, 0);
    lv_obj_remove_flag(s_card, LV_OBJ_FLAG_SCROLLABLE);

    s_expr_label = lv_label_create(s_card);
    lv_obj_set_style_text_font(s_expr_label, &lv_font_montserrat_14, 0);
    lv_obj_set_style_text_color(s_expr_label, lv_color_hex(0x94A3B8), 0);
    lv_obj_set_style_text_align(s_expr_label, LV_TEXT_ALIGN_RIGHT, 0);
    lv_obj_set_size(s_expr_label, 206, 20);
    lv_obj_set_pos(s_expr_label, 0, 4);
    lv_label_set_text(s_expr_label, "");

    s_disp_label = lv_label_create(s_card);
    lv_obj_set_style_text_font(s_disp_label, &lv_font_montserrat_20, 0);
    lv_obj_set_style_text_color(s_disp_label, lv_color_hex(0xFFFFFF), 0);
    lv_obj_set_style_text_align(s_disp_label, LV_TEXT_ALIGN_RIGHT, 0);
    lv_obj_set_size(s_disp_label, 206, 28);
    lv_obj_set_pos(s_disp_label, 0, 28);
    lv_label_set_text(s_disp_label, "0");

    // 键盘网格: 5 行 4 列
    const int start_x = 10;
    const int start_y = 105;
    const int btn_w = 49;
    const int btn_h = 35;
    const int spacing_x = 8;
    const int spacing_y = 7;

    for (int i = 0; i < KEY_COUNT; i++) {
        int r = i / KEY_COLS;
        int c = i % KEY_COLS;
        int bx = start_x + c * (btn_w + spacing_x);
        int by = start_y + r * (btn_h + spacing_y);

        lv_obj_t *btn = lv_obj_create(s_scr);
        lv_obj_set_pos(btn, bx, by);
        lv_obj_set_size(btn, btn_w, btn_h);
        lv_obj_set_style_radius(btn, 6, 0);
        lv_obj_set_style_bg_opa(btn, LV_OPA_COVER, 0);
        lv_obj_set_style_pad_all(btn, 0, 0);
        lv_obj_remove_flag(btn, LV_OBJ_FLAG_SCROLLABLE);

        lv_obj_t *label = lv_label_create(btn);
        lv_obj_set_style_text_font(label, &lv_font_montserrat_14, 0);
        lv_obj_set_style_text_color(label, lv_color_hex(s_keys[i].text), 0);
        lv_label_set_text(label, s_keys[i].label);
        lv_obj_center(label);

        s_btn_objs[i] = btn;
        s_btn_labels[i] = label;

        update_focus_visual(i, i == s_focused_index);
    }

    update_battery(NULL);
    s_bat_timer = lv_timer_create(update_battery, 3000, NULL);

    lv_screen_load(s_scr);
}

void calc_ui_exit(void) {
    if (s_bat_timer) {
        lv_timer_delete(s_bat_timer);
        s_bat_timer = NULL;
    }
    if (s_scr) {
        lv_obj_delete(s_scr);
        s_scr = NULL;
        s_title_label = s_bat_label = s_card = s_expr_label = s_disp_label = NULL;
        for (int i = 0; i < KEY_COUNT; i++) {
            s_btn_objs[i] = NULL;
            s_btn_labels[i] = NULL;
        }
    }
}

void calc_ui_key(bsp_btn_t btn, bsp_btn_ev_t ev) {
    if (!bsp_lvgl_lock(150)) return;

    if (btn == BSP_BTN_UP && ev == BSP_BTN_CLICK) {
        int prev = s_focused_index;
        s_focused_index = (s_focused_index + KEY_COUNT - 1) % KEY_COUNT;
        update_focus_visual(prev, false);
        update_focus_visual(s_focused_index, true);
    } else if (btn == BSP_BTN_DOWN && ev == BSP_BTN_CLICK) {
        int prev = s_focused_index;
        s_focused_index = (s_focused_index + 1) % KEY_COUNT;
        update_focus_visual(prev, false);
        update_focus_visual(s_focused_index, true);
    } else if (btn == BSP_BTN_OK) {
        if (ev == BSP_BTN_CLICK) {
            calc_engine_input(&s_engine, s_keys[s_focused_index].key);
            refresh_display();
        } else if (ev == BSP_BTN_LONG) {
            // 长按 OK 键: 极速全清复位 (AC)
            calc_engine_reset(&s_engine);
            refresh_display();
        }
    }

    bsp_lvgl_unlock();
}

