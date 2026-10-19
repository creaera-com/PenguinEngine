/** @file @brief Generic path mapping and filled-area picking for vector shapes. */
#include "shape.h"
#include "editors/vector/geometry/rotation.h"
#include <math.h>

VectorPath vector_shape_path(const VectorShape *shape, PUIRect rect, float angle) {
    VectorPath path = shape->path;
    if (shape->curve.count) {
        if (!vector_bezier_flatten(&shape->curve, .0005f, &path)) {
            return (VectorPath){0};
        }
        if (shape->curve.closed && vector_path_contour_area(&path, 0) < 0) {
            for (unsigned i = 0; i < path.count / 2; i++) {
                VectorPoint swap = path.points[i];
                path.points[i] = path.points[path.count - 1 - i];
                path.points[path.count - 1 - i] = swap;
            }
        }
    }
    if (!path.count) {
        path = (VectorPath){
            .count = 4, .contours = 1, .points = {{0, 0}, {1, 0}, {1, 1}, {0, 1}}, .ends = {4}};
    }
    for (unsigned i = 0; i < path.count; i++) {
        float x = rect.x + path.points[i].x * rect.w, y = rect.y + path.points[i].y * rect.h;
        vector_rotate_point(&x, &y, rect.x + rect.w / 2, rect.y + rect.h / 2, angle);
        path.points[i] = (VectorPoint){x, y};
    }
    return path;
}

bool vector_shape_outline(const VectorShape *shape, PUIRect rect, float angle, float width,
                          int placement, VectorPath *out) {
    VectorPath path = vector_shape_path(shape, rect, angle);
    if (vector_path_outline(&path, width, placement, out)) {
        return true;
    }
    if (!shape->curve.count || !shape->curve.closed) {
        return false;
    }
    float tolerance = .001f;
    for (int attempt = 0; attempt < 12; attempt++, tolerance *= 2) {
        VectorShape coarse = *shape;
        if (!vector_bezier_flatten(&shape->curve, tolerance, &coarse.path)) {
            continue;
        }
        coarse.curve = (VectorBezier){0};
        if (vector_path_contour_area(&coarse.path, 0) < 0) {
            for (unsigned i = 0; i < coarse.path.count / 2; i++) {
                VectorPoint swap = coarse.path.points[i];
                coarse.path.points[i] = coarse.path.points[coarse.path.count - 1 - i];
                coarse.path.points[coarse.path.count - 1 - i] = swap;
            }
        }
        path = vector_shape_path(&coarse, rect, angle);
        if (vector_path_outline(&path, width, placement, out)) {
            return true;
        }
    }
    return false;
}

static float segment_distance(VectorPoint a, VectorPoint b, float x, float y) {
    float dx = b.x - a.x, dy = b.y - a.y, length = dx * dx + dy * dy;
    float t = length > 0 ? ((x - a.x) * dx + (y - a.y) * dy) / length : 0;
    t = fmaxf(0, fminf(1, t));
    return hypotf(x - a.x - dx * t, y - a.y - dy * t);
}

bool vector_shape_contains(const VectorShape *shape, float x, float y) {
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
    if (!shape->path.count) {
        return vector_rotated_contains(shape->rect, shape->angle, x, y);
    }
    vector_rotate_point(&x, &y, shape->rect.x + shape->rect.w / 2,
                        shape->rect.y + shape->rect.h / 2, -shape->angle);
    return vector_path_contains(&shape->path, (x - shape->rect.x) / shape->rect.w,
                                (y - shape->rect.y) / shape->rect.h);
}

bool vector_shape_intersects(const VectorShape *shape, PUIRect box) {
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
    if (!shape->path.count) {
        return vector_rotated_intersects(shape->rect, shape->angle, box);
    }
    VectorPath path = vector_shape_path(shape, shape->rect, shape->angle);
    return vector_path_intersects(&path, box);
}
