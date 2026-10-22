/** @file
 * @brief Editor compatibility names for independent pengDraw geometry. */
#pragma once
#include "pengdraw/pengdraw.h"
#include "pengui/pengui.h"
#define vector_angle_normalize pdraw_angle_normalize
#define vector_angle_turn pdraw_angle_turn
#define vector_rotate_point pdraw_rotate_point

static inline PUIRect vector_rotated_bounds(PUIRect r, float a) {
    PDrawRect out = pdraw_rotated_bounds((PDrawRect){r.x, r.y, r.w, r.h}, a);
    return (PUIRect){out.x, out.y, out.w, out.h};
}

static inline bool vector_rotated_contains(PUIRect r, float a, float x, float y) {
    return pdraw_rotated_contains((PDrawRect){r.x, r.y, r.w, r.h}, a, x, y);
}

static inline void vector_rotated_corners(PUIRect r, float a, PUIRect c[4]) {
    PDrawRect out[4];
    pdraw_rotated_corners((PDrawRect){r.x, r.y, r.w, r.h}, a, out);
    for (int i = 0; i < 4; i++) {
        c[i] = (PUIRect){out[i].x, out[i].y, out[i].w, out[i].h};
    }
}

static inline bool vector_rotated_intersects(PUIRect r, float a, PUIRect b) {
    return pdraw_rotated_intersects((PDrawRect){r.x, r.y, r.w, r.h}, a,
                                    (PDrawRect){b.x, b.y, b.w, b.h});
}
