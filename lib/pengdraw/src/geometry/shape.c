#include "pengdraw/pengdraw.h"
/** @file @brief Generic path mapping and filled-area picking for vector shapes. */
#include <math.h>

/* Derived contour only: the rectangle remains parametric and resizing retains the requested radius.
 */
static PDrawPath rounded_rectangle(PDrawRect rect, float requested) {
    PDrawPath path = {.contours = 1};
    float radius = fminf(requested, fminf(rect.w, rect.h) * .5f);
    float rx = radius / rect.w, ry = radius / rect.h;
    const float cx[4] = {1 - rx, 1 - rx, rx, rx};
    const float cy[4] = {ry, 1 - ry, 1 - ry, ry};
    for (int corner = 0; corner < 4; corner++) {
        for (int step = 0; step <= 12; step++) {
            float a = (-1 + corner + step / 12.f) * 1.57079632679f;
            PDrawPoint p = {cx[corner] + cosf(a) * rx, cy[corner] + sinf(a) * ry};
            if (path.count && hypotf(p.x - path.points[path.count - 1].x,
                                     p.y - path.points[path.count - 1].y) < 1e-7f) {
                continue;
            }
            path.points[path.count++] = p;
        }
    }
    if (hypotf(path.points[0].x - path.points[path.count - 1].x,
               path.points[0].y - path.points[path.count - 1].y) < 1e-7f) {
        path.count--;
    }
    path.ends[0] = path.count;
    return path;
}

PDrawPath pdraw_shape_path(const PDrawShape *shape, PDrawRect rect, float angle) {
    PDrawPath path = shape->path;
    if (shape->curve.count) {
        if (!pdraw_bezier_flatten(&shape->curve, .0005f, &path)) {
            return (PDrawPath){0};
        }
        if (shape->curve.closed && pdraw_path_contour_area(&path, 0) < 0) {
            for (unsigned i = 0; i < path.count / 2; i++) {
                PDrawPoint swap = path.points[i];
                path.points[i] = path.points[path.count - 1 - i];
                path.points[path.count - 1 - i] = swap;
            }
        }
    }
    if (!path.count && shape->corner_radius > 0) {
        path = rounded_rectangle(rect, shape->corner_radius);
    }
    if (!path.count) {
        path = (PDrawPath){
            .count = 4, .contours = 1, .points = {{0, 0}, {1, 0}, {1, 1}, {0, 1}}, .ends = {4}};
    }
    for (unsigned i = 0; i < path.count; i++) {
        float x = rect.x + path.points[i].x * rect.w, y = rect.y + path.points[i].y * rect.h;
        pdraw_rotate_point(&x, &y, rect.x + rect.w / 2, rect.y + rect.h / 2, angle);
        path.points[i] = (PDrawPoint){x, y};
    }
    return path;
}

static PDrawPath rectangle_outline(const PDrawShape *shape, PDrawRect rect, float angle,
                                   float width, int placement) {
    float outside = placement < 0 ? 0 : placement > 0 ? width : width / 2;
    float inside = placement > 0 ? 0 : placement < 0 ? width : width / 2;
    float radius = fminf(shape->corner_radius, fminf(rect.w, rect.h) * .5f);
    PDrawRect boxes[2] = {
        {rect.x - outside, rect.y - outside, rect.w + 2 * outside, rect.h + 2 * outside},
        {rect.x + inside, rect.y + inside, rect.w - 2 * inside, rect.h - 2 * inside}};
    PDrawPath result = {0};
    for (int side = 0; side < 2; side++) {
        PDrawRect box = boxes[side];
        if (box.w <= 0 || box.h <= 0) {
            continue;
        }
        float r = side ? fmaxf(0, radius - inside) : radius + outside;
        PDrawPath contour =
            r > 0 ? rounded_rectangle(box, r)
                  : (PDrawPath){.count = 4, .points = {{0, 0}, {1, 0}, {1, 1}, {0, 1}}};
        for (unsigned i = 0; i < contour.count; i++) {
            PDrawPoint p = contour.points[side ? contour.count - 1 - i : i];
            float x = box.x + p.x * box.w, y = box.y + p.y * box.h;
            pdraw_rotate_point(&x, &y, rect.x + rect.w / 2, rect.y + rect.h / 2, angle);
            result.points[result.count++] = (PDrawPoint){x, y};
        }
        result.ends[result.contours++] = result.count;
    }
    return result;
}

bool pdraw_shape_outline(const PDrawShape *shape, PDrawRect rect, float angle, float width,
                         int placement, PDrawPath *out) {
    if (shape->corner_radius > 0 && !shape->path.count && !shape->curve.count && isfinite(width) &&
        width >= 0 && placement >= -1 && placement <= 1) {
        *out = width > 0 ? rectangle_outline(shape, rect, angle, width, placement) : (PDrawPath){0};
        return pdraw_path_valid(out);
    }
    PDrawPath path = pdraw_shape_path(shape, rect, angle);
    if (pdraw_path_outline(&path, width, placement, out)) {
        return true;
    }
    if (!shape->curve.count || !shape->curve.closed) {
        return false;
    }
    float tolerance = .001f;
    for (int attempt = 0; attempt < 12; attempt++, tolerance *= 2) {
        PDrawShape coarse = *shape;
        if (!pdraw_bezier_flatten(&shape->curve, tolerance, &coarse.path)) {
            continue;
        }
        coarse.curve = (PDrawBezier){0};
        if (pdraw_path_contour_area(&coarse.path, 0) < 0) {
            for (unsigned i = 0; i < coarse.path.count / 2; i++) {
                PDrawPoint swap = coarse.path.points[i];
                coarse.path.points[i] = coarse.path.points[coarse.path.count - 1 - i];
                coarse.path.points[coarse.path.count - 1 - i] = swap;
            }
        }
        path = pdraw_shape_path(&coarse, rect, angle);
        if (pdraw_path_outline(&path, width, placement, out)) {
            return true;
        }
    }
    return false;
}
