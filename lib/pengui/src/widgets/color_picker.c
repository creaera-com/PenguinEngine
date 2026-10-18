/** @file @brief HSL/RGBA conversions, strict hex input and color-picker child descriptions. */
#include "color_picker.h"
#include <math.h>
#include <stdio.h>
#include <string.h>

static float unit(float value) {
    return isfinite(value) ? fmaxf(0, fminf(1, value)) : 0;
}

PUIHSLColor pui_color_to_hsl(uint32_t rgba) {
    float r = (rgba >> 24) / 255.f, g = ((rgba >> 16) & 255) / 255.f,
          b = ((rgba >> 8) & 255) / 255.f;
    float high = fmaxf(r, fmaxf(g, b)), low = fminf(r, fminf(g, b)), delta = high - low;
    PUIHSLColor color = {.l = (high + low) / 2, .a = (rgba & 255) / 255.f};
    if (delta > 0) {
        color.s = delta / (1 - fabsf(2 * color.l - 1));
        color.h = 60 * (high == r   ? fmodf((g - b) / delta, 6)
                        : high == g ? (b - r) / delta + 2
                                    : (r - g) / delta + 4);
        if (color.h < 0) {
            color.h += 360;
        }
    }
    return color;
}

uint32_t pui_color_from_hsl(PUIHSLColor color) {
    float hue = isfinite(color.h) ? fmodf(color.h, 360) : 0;
    if (hue < 0) {
        hue += 360;
    }
    float s = unit(color.s), l = unit(color.l), c = (1 - fabsf(2 * l - 1)) * s,
          x = c * (1 - fabsf(fmodf(hue / 60, 2) - 1)), m = l - c / 2;
    float r = 0, g = 0, b = 0;
    if (hue < 60) {
        r = c;
        g = x;
    } else if (hue < 120) {
        r = x;
        g = c;
    } else if (hue < 180) {
        g = c;
        b = x;
    } else if (hue < 240) {
        g = x;
        b = c;
    } else if (hue < 300) {
        r = x;
        b = c;
    } else {
        r = c;
        b = x;
    }
    return ((uint32_t)lroundf((r + m) * 255) << 24) | ((uint32_t)lroundf((g + m) * 255) << 16) |
           ((uint32_t)lroundf((b + m) * 255) << 8) | (uint32_t)lroundf(unit(color.a) * 255);
}

static int hex(char value) {
    if (value >= '0' && value <= '9') {
        return value - '0';
    }
    if (value >= 'a' && value <= 'f') {
        return value - 'a' + 10;
    }
    if (value >= 'A' && value <= 'F') {
        return value - 'A' + 10;
    }
    return -1;
}

bool pui_color_parse_hex(const char *text, uint32_t *rgba) {
    if (!text || !rgba || *text != '#') {
        return false;
    }
    size_t length = strlen(++text);
    if (length != 3 && length != 4 && length != 6 && length != 8) {
        return false;
    }
    uint32_t result = 0;
    for (size_t i = 0; i < length; i++) {
        int value = hex(text[i]);
        if (value < 0) {
            return false;
        }
        result = (result << 4) | (unsigned)value;
    }
    if (length <= 4) {
        uint32_t expanded = 0;
        for (size_t i = 0; i < length; i++) {
            expanded = (expanded << 8) | ((result >> ((length - i - 1) * 4) & 15) * 17);
        }
        result = expanded;
    }
    if (length == 3 || length == 6) {
        result = (result << 8) | 255;
    }
    *rgba = result;
    return true;
}

void pui_color_format_hex(uint32_t rgba, char text[10]) {
    snprintf(text, 10, "#%08X", (unsigned)rgba);
}

bool pui_color_picker_expand(PUILayout *layout, int index, char *error, size_t capacity) {
    static const char *names[9] = {"h", "s", "l", "a", "r", "g", "b", "alpha", "hex"};
    PUINode *picker = &layout->nodes[index];
    uint32_t rgba;
    if (!*picker->value) {
        snprintf(picker->value, sizeof picker->value, "#FFFFFFFF");
    }
    if (layout->count + 9 > PUI_LAYOUT_NODES || !pui_color_parse_hex(picker->value, &rgba)) {
        if (error && capacity) {
            snprintf(error, capacity,
                     "color_picker requires a valid hex value and nine free layout nodes");
        }
        return false;
    }
    char prefix[64];
    snprintf(prefix, sizeof prefix, "%s", picker->name);
    int previous = -1;
    for (int j = 0; j < 9; j++) {
        int child = layout->count;
        PUINode *node = &layout->nodes[child];
        *node = (PUINode){.type = PUI_NODE_INPUT,
                          .enabled = true,
                          .child = -1,
                          .next = -1,
                          .height = 28,
                          .generated_id = picker->generated_id};
        if (snprintf(node->name, sizeof node->name, "%s.%s", prefix, names[j]) >=
            (int)sizeof node->name) {
            if (error && capacity) {
                snprintf(error, capacity,
                         "color_picker ID leaves insufficient room for field suffixes");
            }
            return false;
        }
        node->id = pui_id(node->name);
        for (int k = 0; k < child; k++) {
            if (layout->nodes[k].id == node->id) {
                if (error && capacity) {
                    snprintf(error, capacity, "duplicate color_picker field ID");
                }
                return false;
            }
        }
        snprintf(node->action, sizeof node->action, "pengui.color.input");
        if (previous < 0) {
            picker->child = child;
        } else {
            layout->nodes[previous].next = child;
        }
        previous = child;
        layout->count++;
    }
    return true;
}
