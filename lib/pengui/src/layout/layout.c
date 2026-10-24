/** @file @brief Shared column, property-row and image-fit geometry; no SDL. */
#include "core/internal.h"
#include <string.h>

void pui_columns_fit(float total, float *left, float *right, float ml, float mr, float mc) {
    if (!isfinite(total) || total <= 0) {
        *left = *right = 0;
        return;
    }
    ml = fmaxf(0, ml);
    mr = fmaxf(0, mr);
    mc = fmaxf(0, mc);
    float available = fmaxf(0, total - fminf(total, mc));
    if (available < ml + mr) {
        float ratio = ml + mr > 0 ? available / (ml + mr) : 0;
        *left = ml * ratio;
        *right = mr * ratio;
        return;
    }
    *left = pui_clamp(*left, ml, available - mr);
    *right = pui_clamp(*right, mr, available - *left);
}

PUIPropertyRow pui_property_row(PUIRect area, float label_fraction, float gap) {
    area.w = fmaxf(0, area.w);
    area.h = fmaxf(0, area.h);
    gap = fminf(area.w, fmaxf(0, gap));
    float left = (area.w - gap) * fminf(1, fmaxf(0, label_fraction));
    return (PUIPropertyRow){{area.x, area.y, left, area.h},
                            {area.x + left + gap, area.y, area.w - left - gap, area.h}};
}

PUIRect pui_image_fit(PUIRect bounds, float width, float height, float padding) {
    if (!isfinite(bounds.x) || !isfinite(bounds.y) || !isfinite(bounds.w) || !isfinite(bounds.h) ||
        !isfinite(width) || !isfinite(height) || !isfinite(padding) || width <= 0 || height <= 0) {
        return (PUIRect){0};
    }
    padding = fmaxf(2, padding);
    float available_w = bounds.w - 2 * padding, available_h = bounds.h - 2 * padding;
    if (available_w <= 0 || available_h <= 0) {
        return (PUIRect){0};
    }
    float scale = fminf(available_w / width, available_h / height);
    float w = width * scale, h = height * scale;
    return (PUIRect){bounds.x + (bounds.w - w) * .5f, bounds.y + (bounds.h - h) * .5f, w, h};
}
