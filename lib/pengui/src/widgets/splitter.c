/** @file @brief SDL-free split constraints and pane rectangles. */
#include "core/internal.h"
#include <string.h>

UiSplit ui_split_rects(UiRect area, UiAxis axis, float second_size, float first_min,
                       float second_min, float gap) {
    if (!isfinite(area.w) || !isfinite(area.h) || area.w < 0 || area.h < 0) {
        area.w = area.h = 0;
    }
    float total = axis == UI_AXIS_Y ? area.h : area.w;
    gap = isfinite(gap) ? ui_clamp(gap, 0, total) : 0;
    float available = total - gap;
    first_min = isfinite(first_min) ? fmaxf(0, first_min) : 0;
    second_min = isfinite(second_min) ? fmaxf(0, second_min) : 0;
    if (first_min + second_min > available) {
        float ratio = available / (first_min + second_min);
        first_min *= ratio;
        second_min *= ratio;
    }
    second_size = ui_clamp(isfinite(second_size) ? second_size : second_min, second_min,
                           fmaxf(second_min, available - first_min));
    float first_size = fmaxf(0, available - second_size);
    UiSplit result = {area, area, area, second_size};
    if (axis == UI_AXIS_Y) {
        result.first.h = first_size;
        result.grip.y += first_size;
        result.grip.h = gap;
        result.second.y += first_size + gap;
        result.second.h = second_size;
    } else {
        result.first.w = first_size;
        result.grip.x += first_size;
        result.grip.w = gap;
        result.second.x += first_size + gap;
        result.second.w = second_size;
    }
    return result;
}
