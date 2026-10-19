/** @file @brief Artboard-local lattice geometry shared by creation and transform tools. */
#include "snap.h"
#include "editors/vector/geometry/constraints.h"
#include "editors/vector/geometry/resize.h"
#include "editors/vector/geometry/rotation.h"
#include <float.h>
#include <math.h>

static bool enabled(const VectorGrid *grid, float zoom) {
    return grid->snap_enabled && isfinite(zoom) && zoom > 0 && isfinite(grid->width) &&
           isfinite(grid->height) && grid->width >= 1 && grid->height >= 1 &&
           isfinite(grid->snap_tolerance) && grid->snap_tolerance >= 0;
}

/* Find nearby lattice vertices rather than projecting onto one infinite slanted line. */
static void vertex(const VectorGrid *grid, float x, float y, float *out_x, float *out_y) {
    float best = FLT_MAX;
    float step = grid->type == VECTOR_GRID_HEXAGONAL ? .75f * grid->height : grid->height / 2;
    int center = (int)floorf(y / step);
    for (int row = center - 2; row <= center + 2; row++) {
        float shift = row % 2 != 0 ? grid->width / 2 : 0;
        for (int part = 0; part < (grid->type == VECTOR_GRID_HEXAGONAL ? 3 : 1); part++) {
            float vy = row * step, vx = shift;
            if (grid->type == VECTOR_GRID_HEXAGONAL) {
                vy += part == 0 ? 0 : part == 1 ? grid->height / 4 : grid->height * .75f;
                vx += part == 0 ? grid->width / 2 : 0;
            }
            vx += roundf((x - vx) / grid->width) * grid->width;
            float distance = (vx - x) * (vx - x) + (vy - y) * (vy - y);
            if (distance < best) {
                best = distance;
                *out_x = vx;
                *out_y = vy;
            }
        }
    }
}

bool vector_grid_snap_point(const VectorGrid *grid, float zoom, float *x, float *y) {
    if (!enabled(grid, zoom) || !isfinite(*x) || !isfinite(*y)) {
        return false;
    }
    float sx = *x, sy = *y, radius = grid->snap_tolerance / zoom;
    if (grid->type == VECTOR_GRID_STANDARD) {
        float nx = roundf(*x / grid->width) * grid->width;
        float ny = roundf(*y / grid->height) * grid->height;
        if (fabsf(nx - *x) <= radius) {
            sx = nx;
        }
        if (fabsf(ny - *y) <= radius) {
            sy = ny;
        }
    } else {
        vertex(grid, *x, *y, &sx, &sy);
        if (hypotf(sx - *x, sy - *y) > radius) {
            return false;
        }
    }
    bool changed = sx != *x || sy != *y;
    *x = sx;
    *y = sy;
    return changed;
}

PUIRect vector_grid_snap_rect(const VectorGrid *grid, float zoom, PUIRect original, unsigned edges,
                              float dx, float dy, float maximum) {
    PUIRect rect = vector_resize_rect(original, edges, dx, dy, maximum);
    if (!edges) {
        rect.x += dx;
        rect.y += dy;
    }
    if (!enabled(grid, zoom) || (dx == 0 && dy == 0)) {
        return rect;
    }
    if (edges) {
        float x = rect.x + (edges & VECTOR_RESIZE_LEFT    ? 0
                            : edges & VECTOR_RESIZE_RIGHT ? rect.w
                                                          : rect.w / 2);
        float y = rect.y + (edges & VECTOR_RESIZE_TOP      ? 0
                            : edges & VECTOR_RESIZE_BOTTOM ? rect.h
                                                           : rect.h / 2);
        float sx = x, sy = y;
        vector_grid_snap_point(grid, zoom, &sx, &sy);
        /* Reapply constraints to the snapped candidate, never move an opposite edge. */
        return vector_resize_rect(
            rect, edges, edges & (VECTOR_RESIZE_LEFT | VECTOR_RESIZE_RIGHT) ? sx - x : 0,
            edges & (VECTOR_RESIZE_TOP | VECTOR_RESIZE_BOTTOM) ? sy - y : 0, maximum);
    }
    float best = FLT_MAX, correction_x = 0, correction_y = 0;
    float axis_x = FLT_MAX, axis_y = FLT_MAX;
    for (int i = 0; i < 4; i++) {
        float x = rect.x + (i & 1 ? rect.w : 0), y = rect.y + (i & 2 ? rect.h : 0);
        float sx = x, sy = y;
        if (grid->type == VECTOR_GRID_STANDARD) {
            vector_grid_snap_point(grid, zoom, &sx, &sy);
        } else {
            vertex(grid, x, y, &sx, &sy);
        }
        float cx = sx - x, cy = sy - y;
        if (grid->type == VECTOR_GRID_STANDARD) {
            float nx = roundf(x / grid->width) * grid->width - x;
            float ny = roundf(y / grid->height) * grid->height - y;
            float radius = grid->snap_tolerance / zoom;
            if (fabsf(nx) <= radius && fabsf(nx) < axis_x) {
                axis_x = fabsf(nx);
                correction_x = nx;
            }
            if (fabsf(ny) <= radius && fabsf(ny) < axis_y) {
                axis_y = fabsf(ny);
                correction_y = ny;
            }
        } else if (hypotf(cx, cy) < best && hypotf(sx - x, sy - y) <= grid->snap_tolerance / zoom) {
            best = hypotf(cx, cy);
            correction_x = cx;
            correction_y = cy;
        }
    }
    rect.x += correction_x;
    rect.y += correction_y;
    return rect;
}

PUIRect vector_grid_resize_constrained(const VectorGrid *grid, float zoom, PUIRect original,
                                       unsigned edges, float dx, float dy, bool aspect,
                                       bool centered, float min_w, float min_h, float max_w,
                                       float max_h) {
    PUIRect rect = vector_resize_constrained(original, edges, dx, dy, aspect, centered, min_w,
                                             min_h, max_w, max_h);
    if (!enabled(grid, zoom) || (dx == 0 && dy == 0)) {
        return rect;
    }
    bool horizontal = (edges & (VECTOR_RESIZE_LEFT | VECTOR_RESIZE_RIGHT)) != 0;
    bool vertical = (edges & (VECTOR_RESIZE_TOP | VECTOR_RESIZE_BOTTOM)) != 0;
    float x = rect.x + (edges & VECTOR_RESIZE_LEFT    ? 0
                        : edges & VECTOR_RESIZE_RIGHT ? rect.w
                                                      : rect.w / 2);
    float y = rect.y + (edges & VECTOR_RESIZE_TOP      ? 0
                        : edges & VECTOR_RESIZE_BOTTOM ? rect.h
                                                       : rect.h / 2);
    float sx = x, sy = y;
    vector_grid_snap_point(grid, zoom, &sx, &sy);
    float ox = original.x + (edges & VECTOR_RESIZE_LEFT    ? 0
                             : edges & VECTOR_RESIZE_RIGHT ? original.w
                                                           : original.w / 2);
    float oy = original.y + (edges & VECTOR_RESIZE_TOP      ? 0
                             : edges & VECTOR_RESIZE_BOTTOM ? original.h
                                                            : original.h / 2);
    return vector_resize_constrained(original, edges, horizontal ? sx - ox : 0,
                                     vertical ? sy - oy : 0, aspect, centered, min_w, min_h, max_w,
                                     max_h);
}

PUIRect vector_grid_snap_rotated_rect(const VectorGrid *grid, float zoom, PUIRect original,
                                      float angle, float dx, float dy) {
    PUIRect rect = original;
    rect.x += dx;
    rect.y += dy;
    if (!enabled(grid, zoom) || (dx == 0 && dy == 0)) {
        return rect;
    }
    PUIRect corners[4];
    vector_rotated_corners(rect, angle, corners);
    float radius = grid->snap_tolerance / zoom;
    float best = FLT_MAX, best_x = FLT_MAX, best_y = FLT_MAX, cx = 0, cy = 0;
    for (int i = 0; i < 4; i++) {
        float x = corners[i].x, y = corners[i].y;
        if (grid->type == VECTOR_GRID_STANDARD) {
            float sx = roundf(x / grid->width) * grid->width - x;
            float sy = roundf(y / grid->height) * grid->height - y;
            if (fabsf(sx) <= radius && fabsf(sx) < best_x) {
                best_x = fabsf(sx);
                cx = sx;
            }
            if (fabsf(sy) <= radius && fabsf(sy) < best_y) {
                best_y = fabsf(sy);
                cy = sy;
            }
        } else {
            float sx = x, sy = y;
            vertex(grid, x, y, &sx, &sy);
            float distance = hypotf(sx - x, sy - y);
            if (distance <= radius && distance < best) {
                best = distance;
                cx = sx - x;
                cy = sy - y;
            }
        }
    }
    rect.x += cx;
    rect.y += cy;
    return rect;
}
