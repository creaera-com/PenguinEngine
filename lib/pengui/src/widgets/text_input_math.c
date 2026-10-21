/** @file @brief Small bounded arithmetic parser for numeric form fields; no script runtime. */
#include "pengui/pengui.h"
#include <errno.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct {
    const char *p;
    bool valid;
    unsigned depth;
} Expression;

static void space(Expression *e) {
    while (*e->p == ' ' || *e->p == '\t') {
        e->p++;
    }
}

static double sum(Expression *e);

static double factor(Expression *e) {
    space(e);
    if (++e->depth > 32) {
        e->valid = false;
        return 0;
    }
    double value = 0;
    if (*e->p == '+' || *e->p == '-') {
        bool negative = *e->p++ == '-';
        value = factor(e);
        if (negative) {
            value = -value;
        }
    } else if (*e->p == '(') {
        e->p++;
        value = sum(e);
        space(e);
        if (*e->p != ')') {
            e->valid = false;
        } else {
            e->p++;
        }
    } else {
        const char *start = e->p;
        while ((*e->p >= '0' && *e->p <= '9') || *e->p == '.' || *e->p == ',' || *e->p == 'e' ||
               *e->p == 'E' ||
               ((*e->p == '-' || *e->p == '+') && e->p > start &&
                (e->p[-1] == 'e' || e->p[-1] == 'E'))) {
            e->p++;
        }
        size_t length = (size_t)(e->p - start);
        char number[256];
        if (!length || length >= sizeof number) {
            e->valid = false;
        } else {
            memcpy(number, start, length);
            number[length] = 0;
            for (char *p = number; *p; p++) {
                if (*p == ',') {
                    *p = '.';
                }
            }
            if (!pui_text_input_valid(PUI_INPUT_DECIMAL, number, true)) {
                e->valid = false;
            }
            char *end;
            errno = 0;
            value = strtod(number, &end);
            if (*end || errno || !isfinite(value)) {
                e->valid = false;
            }
        }
    }
    e->depth--;
    return value;
}

static double product(Expression *e) {
    double value = factor(e);
    space(e);
    while (e->valid && (*e->p == '*' || *e->p == '/')) {
        char op = *e->p++;
        double rhs = factor(e);
        if (op == '/' && rhs == 0) {
            e->valid = false;
            return 0;
        }
        value = op == '*' ? value * rhs : value / rhs;
        if (!isfinite(value)) {
            e->valid = false;
        }
        space(e);
    }
    return value;
}

static double sum(Expression *e) {
    double value = product(e);
    space(e);
    while (e->valid && (*e->p == '+' || *e->p == '-')) {
        char op = *e->p++;
        double rhs = product(e);
        value = op == '+' ? value + rhs : value - rhs;
        if (!isfinite(value)) {
            e->valid = false;
        }
        space(e);
    }
    return value;
}

bool pui_text_input_expression_valid(PUIInputType type, const char *text) {
    if (type == PUI_INPUT_STRING) {
        return pui_text_input_valid(type, text, false);
    }
    if (!text || type < PUI_INPUT_INTEGER || type > PUI_INPUT_DECIMAL) {
        return false;
    }
    const char *p = text;
    while (*p == ' ') {
        p++;
    }
    if (strchr("+-*/", *p) && *p && p[1] == '=') {
        p += 2;
    }
    while (*p) {
        if ((*p >= '0' && *p <= '9') || *p == '.' || *p == ',') {
            const char *start = p;
            while ((*p >= '0' && *p <= '9') || *p == '.' || *p == ',' || *p == 'e' || *p == 'E' ||
                   ((*p == '+' || *p == '-') && p > start && (p[-1] == 'e' || p[-1] == 'E'))) {
                p++;
            }
            size_t length = (size_t)(p - start);
            char number[256];
            if (length >= sizeof number) {
                return false;
            }
            memcpy(number, start, length);
            number[length] = 0;
            if (!pui_text_input_valid(PUI_INPUT_DECIMAL, number, false)) {
                return false;
            }
        } else if (*p == ' ' || *p == '+' || *p == '-' || *p == '*' || *p == '/' || *p == '(' ||
                   *p == ')') {
            p++;
        } else {
            return false;
        }
    }
    return true;
}

bool pui_text_input_evaluate(PUIInputType type, const char *text, double base, char *result,
                             size_t capacity) {
    if (!result || !capacity || !text || type < PUI_INPUT_INTEGER || type > PUI_INPUT_DECIMAL ||
        !pui_text_input_expression_valid(type, text)) {
        return false;
    }
    Expression e = {text, true, 0};
    space(&e);
    char relative = 0;
    if (*e.p && strchr("+-*/", *e.p) && e.p[1] == '=') {
        relative = *e.p;
        e.p += 2;
    }
    double value = sum(&e);
    space(&e);
    if (!e.valid || *e.p) {
        return false;
    }
    if (relative) {
        if (!isfinite(base) || (relative == '/' && value == 0)) {
            return false;
        }
        value = relative == '+'   ? base + value
                : relative == '-' ? base - value
                : relative == '*' ? base * value
                                  : base / value;
    }
    if (!isfinite(value) || (type == PUI_INPUT_INTEGER && trunc(value) != value)) {
        return false;
    }
    if (value == 0) {
        value = 0;
    } // Canonicalize negative zero.
    char formatted[512];
    int length =
        snprintf(formatted, sizeof formatted, type == PUI_INPUT_INTEGER ? "%.0f" : "%.15g", value);
    if (length < 0 || (size_t)length >= sizeof formatted || (size_t)length >= capacity) {
        return false;
    }
    memcpy(result, formatted, (size_t)length + 1);
    return true;
}
