// main/calc_ui.h —— 全新设计的计算器独立 UI 界面与交互控制。
#pragma once

#include "bsp_button.h"
#include "esp_err.h"

// 创建并载入计算器主界面 (调用前需持有 LVGL 锁)
void calc_ui_enter(void);

// 退出并销毁计算器 UI,释放定时器与内存 (调用前需持有 LVGL 锁)
void calc_ui_exit(void);

// 响应物理三键事件 (内部自动管理 LVGL 锁)
void calc_ui_key(bsp_btn_t btn, bsp_btn_ev_t ev);

