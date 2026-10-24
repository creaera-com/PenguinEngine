/** @file @brief Reusable rectangle geometry without document, input or renderer dependencies. */
#include "resize.h"
#include <math.h>
static const unsigned handles[8] = {9, 1, 3, 2, 6, 4, 12, 8};

unsigned vector_resize_handle_edges(int index) {
    return index >= 0 && index < 8 ? handles[index] : 0;
}

PUIRect vector_resize_handle_rect(PUIRect bounds, int index, float size) {
    unsigned edges = vector_resize_handle_edges(index);
    if (!edges) {
        return (PUIRect){0};
    }
    float x = bounds.x + (edges & VECTOR_RESIZE_LEFT    ? 0
                          : edges & VECTOR_RESIZE_RIGHT ? bounds.w
                                                        : bounds.w / 2);
    float y = bounds.y + (edges & VECTOR_RESIZE_TOP      ? 0
                          : edges & VECTOR_RESIZE_BOTTOM ? bounds.h
                                                         : bounds.h / 2);
    return (PUIRect){x - size / 2, y - size / 2, size, size};
}

unsigned vector_resize_hit(PUIRect bounds, float x, float y, float size) {
    const int order[8] = {0, 2, 4, 6, 1, 3, 5, 7};
    for (int i = 0; i < 8; i++) {
        int index = order[i];
        PUIRect r = vector_resize_handle_rect(bounds, index, size);
        if (x >= r.x && x < r.x + r.w && y >= r.y && y < r.y + r.h) {
            return handles[index];
        }
    }
    return 0;
}

PUIRect vector_resize_rect(PUIRect original, unsigned edges, float dx, float dy, float maximum) {
    PUIRect rect = original;
    if (edges & VECTOR_RESIZE_LEFT) {
        rect.w = fmaxf(1, fminf(maximum, original.w - dx));
        rect.x = original.x + original.w - rect.w;
    } else if (edges & VECTOR_RESIZE_RIGHT) {
        rect.w = fmaxf(1, fminf(maximum, original.w + dx));
    }
    if (edges & VECTOR_RESIZE_TOP) {
        rect.h = fmaxf(1, fminf(maximum, original.h - dy));
        rect.y = original.y + original.h - rect.h;
    } else if (edges & VECTOR_RESIZE_BOTTOM) {
        rect.h = fmaxf(1, fminf(maximum, original.h + dy));
    }
    return rect;
}
