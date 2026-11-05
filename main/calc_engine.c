// main/calc_engine.c —— 计算器核心计算引擎实现。
#include "calc_engine.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#define MAX_INPUT_DIGITS 12

static char op_to_char(calc_key_t op) {
    switch (op) {
        case CALC_KEY_ADD: return '+';
        case CALC_KEY_SUB: return '-';
        case CALC_KEY_MUL: return '*';
        case CALC_KEY_DIV: return '/';
        default: return '?';
    }
}

static void format_double(double val, char *buf, size_t max_len) {
    if (isnan(val) || isinf(val)) {
        snprintf(buf, max_len, "Error: Overflow");
        return;
    }
    if (fabs(val) < 1e-12) {
        snprintf(buf, max_len, "0");
        return;
    }
    if (fabs(val) >= 1e12 || fabs(val) < 1e-6) {
        snprintf(buf, max_len, "%.6g", val);
        return;
    }

    if (val == (double)(long long)val && fabs(val) < 1e12) {
        snprintf(buf, max_len, "%lld", (long long)val);
        return;
    }

    snprintf(buf, max_len, "%.8f", val);
    char *p = buf + strlen(buf) - 1;
    while (p > buf && *p == '0') {
        *p-- = '\0';
    }
    if (p > buf && *p == '.') {
        *p = '\0';
    }
}

void calc_engine_init(calc_engine_t *engine) {
    if (!engine) return;
    calc_engine_reset(engine);
}

void calc_engine_reset(calc_engine_t *engine) {
    if (!engine) return;
    snprintf(engine->display, sizeof(engine->display), "0");
    engine->expr[0] = '\0';
    engine->operand_a = 0.0;
    engine->pending_op = CALC_KEY_NONE;
    engine->is_new_entry = true;
    engine->has_error = false;
    engine->evaluated = false;
}

static void handle_digit(calc_engine_t *engine, int digit) {
    if (engine->has_error || engine->evaluated) {
        calc_engine_reset(engine);
    }

    if (engine->is_new_entry) {
        snprintf(engine->display, sizeof(engine->display), "%d", digit);
        engine->is_new_entry = false;
    } else {
        size_t len = strlen(engine->display);
        if (strcmp(engine->display, "0") == 0) {
            snprintf(engine->display, sizeof(engine->display), "%d", digit);
        } else if (len < MAX_INPUT_DIGITS) {
            engine->display[len] = (char)('0' + digit);
            engine->display[len + 1] = '\0';
        }
    }
}

static void handle_dot(calc_engine_t *engine) {
    if (engine->has_error || engine->evaluated) {
        calc_engine_reset(engine);
    }

    if (engine->is_new_entry) {
        snprintf(engine->display, sizeof(engine->display), "0.");
        engine->is_new_entry = false;
    } else if (!strchr(engine->display, '.')) {
        size_t len = strlen(engine->display);
        if (len < MAX_INPUT_DIGITS) {
            engine->display[len] = '.';
            engine->display[len + 1] = '\0';
        }
    }
}

static void handle_backspace(calc_engine_t *engine) {
    if (engine->has_error || engine->evaluated || engine->is_new_entry) {
        return;
    }
    size_t len = strlen(engine->display);
    if (len <= 1 || (len == 2 && engine->display[0] == '-')) {
        snprintf(engine->display, sizeof(engine->display), "0");
        engine->is_new_entry = true;
    } else {
        engine->display[len - 1] = '\0';
    }
}

static void handle_negate(calc_engine_t *engine) {
    if (engine->has_error || strcmp(engine->display, "0") == 0) {
        return;
    }
    if (engine->display[0] == '-') {
        memmove(engine->display, engine->display + 1, strlen(engine->display));
    } else {
        size_t len = strlen(engine->display);
        if (len + 1 < sizeof(engine->display)) {
            memmove(engine->display + 1, engine->display, len + 1);
            engine->display[0] = '-';
        }
    }
}

static void handle_percent(calc_engine_t *engine) {
    if (engine->has_error) return;
    double cur = atof(engine->display);
    if (engine->pending_op == CALC_KEY_ADD || engine->pending_op == CALC_KEY_SUB) {
        cur = engine->operand_a * (cur / 100.0);
    } else {
        cur = cur / 100.0;
    }
    format_double(cur, engine->display, sizeof(engine->display));
    engine->is_new_entry = true;
}

static bool compute_binary(double a, calc_key_t op, double b, double *result) {
    switch (op) {
        case CALC_KEY_ADD:
            *result = a + b;
            return true;
        case CALC_KEY_SUB:
            *result = a - b;
            return true;
        case CALC_KEY_MUL:
            *result = a * b;
            return true;
        case CALC_KEY_DIV:
            if (fabs(b) < 1e-12) return false;
            *result = a / b;
            return true;
        default:
            return false;
    }
}

static void handle_operator(calc_engine_t *engine, calc_key_t op) {
    if (engine->has_error) return;

    if (engine->pending_op != CALC_KEY_NONE && !engine->is_new_entry) {
        double b = atof(engine->display);
        double res = 0.0;
        if (!compute_binary(engine->operand_a, engine->pending_op, b, &res)) {
            snprintf(engine->display, sizeof(engine->display), "Error: Div by 0");
            engine->has_error = true;
            return;
        }
        engine->operand_a = res;
        format_double(res, engine->display, sizeof(engine->display));
    } else {
        engine->operand_a = atof(engine->display);
    }

    engine->pending_op = op;
    snprintf(engine->expr, sizeof(engine->expr), "%.28s %c", engine->display, op_to_char(op));
    engine->is_new_entry = true;
    engine->evaluated = false;
}

static void handle_equals(calc_engine_t *engine) {
    if (engine->has_error) return;

    if (engine->pending_op != CALC_KEY_NONE) {
        double b = atof(engine->display);
        char a_str[24];
        char b_str[24];
        format_double(engine->operand_a, a_str, sizeof(a_str));
        format_double(b, b_str, sizeof(b_str));

        double res = 0.0;
        if (!compute_binary(engine->operand_a, engine->pending_op, b, &res)) {
            snprintf(engine->expr, sizeof(engine->expr), "%.20s %c %.20s =",
                     a_str, op_to_char(engine->pending_op), b_str);
            snprintf(engine->display, sizeof(engine->display), "Error: Div by 0");
            engine->has_error = true;
            return;
        }

        snprintf(engine->expr, sizeof(engine->expr), "%.20s %c %.20s =",
                 a_str, op_to_char(engine->pending_op), b_str);
        format_double(res, engine->display, sizeof(engine->display));
        engine->operand_a = res;
        engine->pending_op = CALC_KEY_NONE;
        engine->is_new_entry = true;
        engine->evaluated = true;
    }
}

void calc_engine_input(calc_engine_t *engine, calc_key_t key) {
    if (!engine) return;

    switch (key) {
        case CALC_KEY_0: case CALC_KEY_1: case CALC_KEY_2: case CALC_KEY_3:
        case CALC_KEY_4: case CALC_KEY_5: case CALC_KEY_6: case CALC_KEY_7:
        case CALC_KEY_8: case CALC_KEY_9:
            handle_digit(engine, (int)key);
            break;
        case CALC_KEY_DOT:
            handle_dot(engine);
            break;
        case CALC_KEY_BACKSPACE:
            handle_backspace(engine);
            break;
        case CALC_KEY_CLEAR:
            if (engine->has_error || engine->evaluated || strcmp(engine->display, "0") == 0) {
                calc_engine_reset(engine);
            } else {
                snprintf(engine->display, sizeof(engine->display), "0");
                engine->is_new_entry = true;
            }
            break;
        case CALC_KEY_NEG:
            handle_negate(engine);
            break;
        case CALC_KEY_PERCENT:
            handle_percent(engine);
            break;
        case CALC_KEY_ADD: case CALC_KEY_SUB:
        case CALC_KEY_MUL: case CALC_KEY_DIV:
            handle_operator(engine, key);
            break;
        case CALC_KEY_EQUALS:
            handle_equals(engine);
            break;
        default:
            break;
    }
}

const char *calc_engine_get_display(const calc_engine_t *engine) {
    if (!engine) return "0";
    return engine->display;
}

const char *calc_engine_get_expr(const calc_engine_t *engine) {
    if (!engine) return "";
    return engine->expr;
}

