/** @file @brief SDL-free split constraints and pane rectangles. */
#include "core/internal.h"
#include <string.h>

PUISplit pui_split_rects(PUIRect area, PUIAxis axis, float second_size, float first_min,
                         float second_min, float gap) {
    if (!isfinite(area.w) || !isfinite(area.h) || area.w < 0 || area.h < 0) {
        area.w = area.h = 0;
    }
    float total = axis == PUI_AXIS_Y ? area.h : area.w;
    gap = isfinite(gap) ? pui_clamp(gap, 0, total) : 0;
    float available = total - gap;
    first_min = isfinite(first_min) ? fmaxf(0, first_min) : 0;
    second_min = isfinite(second_min) ? fmaxf(0, second_min) : 0;
    if (first_min + second_min > available) {
        float ratio = available / (first_min + second_min);
        first_min *= ratio;
        second_min *= ratio;
    }
    second_size = pui_clamp(isfinite(second_size) ? second_size : second_min, second_min,
                            fmaxf(second_min, available - first_min));
    float first_size = fmaxf(0, available - second_size);
    PUISplit result = {area, area, area, second_size};
    if (axis == PUI_AXIS_Y) {
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

PUIRegions pui_split_layout(PUIRect area, PUIAxis axis, int count, int flexible,
                            const float preferred[3], const float minimum[3], float gap) {
    PUIRegions result = {0};
    if (count < 2 || count > 3 || flexible < 0 || flexible >= count || !preferred || !minimum) {
        return result;
    }
    area.w = isfinite(area.w) ? fmaxf(0, area.w) : 0;
    area.h = isfinite(area.h) ? fmaxf(0, area.h) : 0;
    float total = axis == PUI_AXIS_X ? area.w : area.h;
    gap = isfinite(gap) ? pui_clamp(gap, 0, total / (count - 1)) : 0;
    float available = total - gap * (count - 1);
    float sizes[3] = {0}, limits[3] = {0}, sum = 0;
    for (int i = 0; i < count; i++) {
        limits[i] = isfinite(minimum[i]) ? fmaxf(0, minimum[i]) : 0;
        sum += limits[i];
    }
    if (sum > available) {
        for (int i = 0; i < count; i++) {
            sizes[i] = limits[i] * (available / sum);
        }
    } else {
        float fixed = 0, extra = 0;
        for (int i = 0; i < count; i++) {
            if (i == flexible) {
                continue;
            }
            sizes[i] = fmaxf(limits[i], isfinite(preferred[i]) ? preferred[i] : limits[i]);
            fixed += sizes[i];
            extra += sizes[i] - limits[i];
        }
        float overflow = fmaxf(0, fixed + limits[flexible] - available);
        if (overflow > 0 && extra > 0) {
            fixed = 0;
            for (int i = 0; i < count; i++) {
                if (i == flexible) {
                    continue;
                }
                sizes[i] -= overflow * ((sizes[i] - limits[i]) / extra);
                fixed += sizes[i];
            }
        }
        sizes[flexible] = fmaxf(0, available - fixed);
    }
    result.count = count;
    float offset = 0;
    for (int i = 0; i < count; i++) {
        result.panes[i] = area;
        if (axis == PUI_AXIS_X) {
            result.panes[i].x += offset;
            result.panes[i].w = sizes[i];
        } else {
            result.panes[i].y += offset;
            result.panes[i].h = sizes[i];
        }
        offset += sizes[i];
        if (i + 1 < count) {
            result.grips[i] = area;
            if (axis == PUI_AXIS_X) {
                result.grips[i].x += offset;
                result.grips[i].w = gap;
            } else {
                result.grips[i].y += offset;
                result.grips[i].h = gap;
            }
            offset += gap;
        }
    }
    return result;
}
