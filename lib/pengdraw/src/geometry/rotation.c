/** @file @brief Clockwise rotation uses the positive-down document coordinate system. */
#include "pengdraw/pengdraw.h"
#include <math.h>

float pdraw_angle_normalize(float degrees) {
    float result = fmodf(degrees + 180, 360);
    if (result < 0) {
        result += 360;
    }
    return result - 180;
}

float pdraw_angle_turn(float degrees) {
    float result = fmodf(degrees, 360);
    if (result == 0) {
        return degrees == 0 ? 0 : copysignf(360, degrees);
    }
    return result;
}

void pdraw_rotate_point(float *x, float *y, float cx, float cy, float degrees) {
    float radians = pdraw_angle_normalize(degrees) * .01745329251994329577f, c = cosf(radians),
          s = sinf(radians);
    float dx = *x - cx, dy = *y - cy;
    *x = cx + dx * c - dy * s;
    *y = cy + dx * s + dy * c;
}

void pdraw_rotated_corners(PDrawRect rect, float degrees, PDrawRect corners[4]) {
    for (int i = 0; i < 4; i++) {
        corners[i] = (PDrawRect){rect.x + (i == 1 || i == 2 ? rect.w : 0),
                                 rect.y + (i >= 2 ? rect.h : 0), 0, 0};
        pdraw_rotate_point(&corners[i].x, &corners[i].y, rect.x + rect.w / 2, rect.y + rect.h / 2,
                           degrees);
    }
}

PDrawRect pdraw_rotated_bounds(PDrawRect rect, float degrees) {
    PDrawRect corners[4];
    pdraw_rotated_corners(rect, degrees, corners);
    float left = corners[0].x, top = corners[0].y, right = left, bottom = top;
    for (int i = 1; i < 4; i++) {
        left = fminf(left, corners[i].x);
        top = fminf(top, corners[i].y);
        right = fmaxf(right, corners[i].x);
        bottom = fmaxf(bottom, corners[i].y);
    }
    return (PDrawRect){left, top, right - left, bottom - top};
}

bool pdraw_rotated_contains(PDrawRect rect, float degrees, float x, float y) {
    pdraw_rotate_point(&x, &y, rect.x + rect.w / 2, rect.y + rect.h / 2, -degrees);
    return x >= rect.x && x < rect.x + rect.w && y >= rect.y && y < rect.y + rect.h;
}

bool pdraw_rotated_intersects(PDrawRect rect, float degrees, PDrawRect box) {
    if (box.w <= 0 || box.h <= 0) {
        return false;
    }
    PDrawRect a[4], b[4];
    pdraw_rotated_corners(rect, degrees, a);
    pdraw_rotated_corners(box, 0, b);
    float rad = degrees * .01745329251994329577f;
    float axes[4][2] = {{1, 0}, {0, 1}, {cosf(rad), sinf(rad)}, {-sinf(rad), cosf(rad)}};
    for (int axis = 0; axis < 4; axis++) {
        float amin = a[0].x * axes[axis][0] + a[0].y * axes[axis][1], amax = amin;
        float bmin = b[0].x * axes[axis][0] + b[0].y * axes[axis][1], bmax = bmin;
        for (int i = 1; i < 4; i++) {
            float ap = a[i].x * axes[axis][0] + a[i].y * axes[axis][1];
            float bp = b[i].x * axes[axis][0] + b[i].y * axes[axis][1];
            amin = fminf(amin, ap);
            amax = fmaxf(amax, ap);
            bmin = fminf(bmin, bp);
            bmax = fmaxf(bmax, bp);
        }
        if (amax <= bmin || bmax <= amin) {
            return false;
        }
    }
    return true;
}
