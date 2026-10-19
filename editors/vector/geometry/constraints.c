/** @file @brief Modifier geometry independent of object types and native keyboard input. */
#include "constraints.h"
#include "resize.h"
#include <math.h>

PUIRect vector_draw_constrained(float x, float y, float end_x, float end_y, bool square,
                                bool centered, const PUIRect *bounds) {
    float dx = end_x - x, dy = end_y - y;
    float w = fabsf(dx), h = fabsf(dy);
    if (square) {
        w = h = fmaxf(w, h);
    }
    if (bounds) {
        float available_x = centered ? fminf(x - bounds->x, bounds->x + bounds->w - x)
                            : dx < 0 ? x - bounds->x
                                     : bounds->x + bounds->w - x;
        float available_y = centered ? fminf(y - bounds->y, bounds->y + bounds->h - y)
                            : dy < 0 ? y - bounds->y
                                     : bounds->y + bounds->h - y;
        w = fminf(w, fmaxf(0, available_x));
        h = fminf(h, fmaxf(0, available_y));
        if (square) {
            w = h = fminf(w, h);
        }
    }
    return (PUIRect){centered || dx < 0 ? x - w : x, centered || dy < 0 ? y - h : y,
                     centered ? 2 * w : w, centered ? 2 * h : h};
}

PUIRect vector_resize_constrained(PUIRect original, unsigned edges, float dx, float dy, bool aspect,
                                  bool centered, float min_w, float min_h, float max_w,
                                  float max_h) {
    bool horizontal = (edges & (VECTOR_RESIZE_LEFT | VECTOR_RESIZE_RIGHT)) != 0;
    bool vertical = (edges & (VECTOR_RESIZE_TOP | VECTOR_RESIZE_BOTTOM)) != 0;
    float factor = centered ? 2 : 1;
    float w = original.w + (edges & VECTOR_RESIZE_LEFT ? -dx : horizontal ? dx : 0) * factor;
    float h = original.h + (edges & VECTOR_RESIZE_TOP ? -dy : vertical ? dy : 0) * factor;
    if (aspect) {
        float sx = w / original.w, sy = h / original.h;
        float scale = !horizontal ? sy : !vertical ? sx : fabsf(sx - 1) >= fabsf(sy - 1) ? sx : sy;
        scale = fmaxf(fmaxf(min_w / original.w, min_h / original.h),
                      fminf(fminf(max_w / original.w, max_h / original.h), scale));
        w = original.w * scale;
        h = original.h * scale;
    } else {
        w = fmaxf(min_w, fminf(max_w, w));
        h = fmaxf(min_h, fminf(max_h, h));
    }
    float x = centered || !horizontal      ? original.x + (original.w - w) / 2
              : edges & VECTOR_RESIZE_LEFT ? original.x + original.w - w
                                           : original.x;
    float y = centered || !vertical       ? original.y + (original.h - h) / 2
              : edges & VECTOR_RESIZE_TOP ? original.y + original.h - h
                                          : original.y;
    return (PUIRect){x, y, w, h};
}
