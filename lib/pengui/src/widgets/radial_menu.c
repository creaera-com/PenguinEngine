/** @file @brief SDL-free annular context-menu hit testing. */
#include "pengui/pengui.h"
#include <math.h>

int pui_radial_item(PUIRect bounds, int count, float x, float y) {
    if (count < 2 || count > 8 || !isfinite(bounds.x) || !isfinite(bounds.y) ||
        !isfinite(bounds.w) || !isfinite(bounds.h) || bounds.w <= 0 || bounds.h <= 0 ||
        !isfinite(x) || !isfinite(y)) {
        return -1;
    }
    float dx = x - bounds.x - bounds.w / 2, dy = y - bounds.y - bounds.h / 2;
    float radius = fminf(bounds.w, bounds.h) / 2;
    float distance = hypotf(dx, dy);
    if (distance < radius * .24f || distance >= radius) {
        return -1;
    }
    float turn = 6.283185307f;
    float step = turn / count;
    float angle = fmodf(atan2f(dy, dx) + turn + turn / 4 + step / 2, turn);
    // A small angular gap makes the sectors visually and interactively separate.
    float part = fmodf(angle, step);
    return part < .018f || part > step - .018f ? -1 : (int)(angle / step);
}
