// main/calc_engine.h —— 纯 C 计算器核心计算引擎,独立于硬件与 UI,可进行主机单测。
#pragma once

#include <stdbool.h>
#include <stddef.h>

typedef enum {
    CALC_KEY_0 = 0,
    CALC_KEY_1,
    CALC_KEY_2,
    CALC_KEY_3,
    CALC_KEY_4,
    CALC_KEY_5,
    CALC_KEY_6,
    CALC_KEY_7,
    CALC_KEY_8,
    CALC_KEY_9,
    CALC_KEY_DOT,
    CALC_KEY_ADD,
    CALC_KEY_SUB,
    CALC_KEY_MUL,
    CALC_KEY_DIV,
    CALC_KEY_PERCENT,
    CALC_KEY_NEG,
    CALC_KEY_CLEAR,
    CALC_KEY_BACKSPACE,
    CALC_KEY_EQUALS,
    CALC_KEY_NONE,
} calc_key_t;

typedef struct {
    char display[32];      // 当前主显示数值/状态字符串 (如 "123.45", "0", "Error: Div by 0")
    char expr[64];         // 顶部历史表达式字符串 (如 "12 + 8 =", "50 * ")
    double operand_a;      // 上一个操作数
    calc_key_t pending_op; // 挂起的双目运算符 (+, -, *, / 或 CALC_KEY_NONE)
    bool is_new_entry;     // 是否处于新输入状态 (输入数字时覆盖当前显示)
    bool has_error;        // 是否处于错误状态
    bool evaluated;        // 是否刚完成等号结算
} calc_engine_t;

// 初始化计算引擎
void calc_engine_init(calc_engine_t *engine);

// 全清复位 (AC)
void calc_engine_reset(calc_engine_t *engine);

// 处理按键输入
void calc_engine_input(calc_engine_t *engine, calc_key_t key);

// 获取当前主显示屏文本
const char *calc_engine_get_display(const calc_engine_t *engine);

// 获取当前表达式行文本
const char *calc_engine_get_expr(const calc_engine_t *engine);

