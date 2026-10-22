/** @file @brief Editor geometry adapter and document-specific hit testing. */
#include "shape.h"
#include "editors/vector/geometry/rotation.h"
#include "editors/vector/rendering/drawing.h"
#include "editors/vector/repeat/repeat.h"
#include <math.h>

VectorPath vector_shape_path(const VectorShape *shape, PUIRect rect, float angle) {
    PDrawShape drawing = vector_shape_drawing(shape);
    return pdraw_shape_path(&drawing, vector_draw_rect(rect), angle);
}

bool vector_shape_outline(const VectorShape *shape, PUIRect rect, float angle, float width,
                          int placement, VectorPath *out) {
    PDrawShape drawing = vector_shape_drawing(shape);
    return pdraw_shape_outline(&drawing, vector_draw_rect(rect), angle, width, placement, out);
}

static float segment_distance(VectorPoint a, VectorPoint b, float x, float y) {
    float dx = b.x - a.x, dy = b.y - a.y, length = dx * dx + dy * dy;
    float t = length > 0 ? ((x - a.x) * dx + (y - a.y) * dy) / length : 0;
    t = fmaxf(0, fminf(1, t));
    return hypotf(x - a.x - dx * t, y - a.y - dy * t);
}

bool vector_shape_contains(const VectorShape *shape, float x, float y) {
    if (shape->repeat.enabled) {
        for (unsigned row = 0; row < shape->repeat.rows; row++) {
            for (unsigned col = 0; col < shape->repeat.columns; col++) {
                VectorShape instance;
                if (vector_repeat_instance(shape, &shape->repeat, col, row, &instance) &&
                    vector_shape_contains(&instance, x, y)) {
                    return true;
                }
            }
        }
        return false;
    }
    if (shape->curve.count && !shape->curve.closed) {
        VectorPath path = vector_shape_path(shape, shape->rect, shape->angle);
        for (unsigned i = 1; i < path.count; i++) {
            if (segment_distance(path.points[i - 1], path.points[i], x, y) <=
                fmaxf(4, shape->stroke.width / 2)) {
                return true;
            }
        }
        return false;
    }
    if (shape->curve.count || shape->corner_radius > 0) {
        VectorPath path = vector_shape_path(shape, shape->rect, shape->angle);
        return vector_path_contains(&path, x, y);
    }
    if (!shape->path.count) {
        return vector_rotated_contains(shape->rect, shape->angle, x, y);
    }
    vector_rotate_point(&x, &y, shape->rect.x + shape->rect.w / 2,
                        shape->rect.y + shape->rect.h / 2, -shape->angle);
    return vector_path_contains(&shape->path, (x - shape->rect.x) / shape->rect.w,
                                (y - shape->rect.y) / shape->rect.h);
}

bool vector_shape_intersects(const VectorShape *shape, PUIRect box) {
    if (shape->repeat.enabled) {
        for (unsigned row = 0; row < shape->repeat.rows; row++) {
            for (unsigned col = 0; col < shape->repeat.columns; col++) {
                VectorShape instance;
                if (vector_repeat_instance(shape, &shape->repeat, col, row, &instance) &&
                    vector_shape_intersects(&instance, box)) {
                    return true;
                }
            }
        }
        return false;
    }
    if (box.w <= 0 || box.h <= 0) {
        return false;
    }
    if (shape->curve.count && !shape->curve.closed) {
        VectorPath path = vector_shape_path(shape, shape->rect, shape->angle);
        for (unsigned i = 1; i < path.count; i++) {
            VectorPoint a = path.points[i - 1], b = path.points[i];
            float dx = b.x - a.x, dy = b.y - a.y, lo = 0, hi = 1;
            float p[4] = {-dx, dx, -dy, dy},
                  q[4] = {a.x - box.x, box.x + box.w - a.x, a.y - box.y, box.y + box.h - a.y};
            bool hit = true;
            for (int j = 0; j < 4; j++) {
                if (p[j] == 0) {
                    if (q[j] < 0) {
                        hit = false;
                    }
                } else if (p[j] < 0) {
                    lo = fmaxf(lo, q[j] / p[j]);
                } else {
                    hi = fminf(hi, q[j] / p[j]);
                }
            }
            if (hit && lo <= hi) {
                return true;
            }
        }
        return false;
    }
    if (!shape->path.count && shape->corner_radius <= 0) {
        return vector_rotated_intersects(shape->rect, shape->angle, box);
    }
    VectorPath path = vector_shape_path(shape, shape->rect, shape->angle);
    return vector_path_intersects(&path, box);
}
