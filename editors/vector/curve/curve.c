/** @file @brief Curve normalization and node edits; value commands belong to objects. */
#include "curve.h"
#include "editors/vector/geometry/rotation.h"
#include "editors/vector/object/commands.h"
#include <math.h>
#include <string.h>

bool vector_curve_shape(VectorShape *s, const VectorBezier *curve) {
    if (!s || !vector_bezier_valid(curve) || curve->count < 2) {
        return false;
    }
    PUIRect bounds = vector_bezier_bounds(curve);
    if (bounds.w < .000001f && bounds.h < .000001f) {
        return false;
    }
    bounds.w = fmaxf(1, bounds.w);
    bounds.h = fmaxf(1, bounds.h);
    if (bounds.w > VECTOR_MAX_ARTBOARD_SIZE || bounds.h > VECTOR_MAX_ARTBOARD_SIZE) {
        return false;
    }
    VectorShape result = *s;
    result.rect = bounds;
    result.angle = 0;
    result.curve = *curve;
    result.curve_kind = VECTOR_CURVE_FREE;
    result.path = (VectorPath){0};
    for (unsigned i = 0; i < curve->count; i++) {
        VectorBezierNode *n = &result.curve.nodes[i];
        VectorPoint *points[] = {&n->point, &n->incoming, &n->outgoing};
        for (int j = 0; j < 3; j++) {
            points[j]->x = (points[j]->x - bounds.x) / bounds.w;
            points[j]->y = (points[j]->y - bounds.y) / bounds.h;
        }
    }
    if (curve->closed && !vector_bezier_flatten(&result.curve, .001f, &result.path)) {
        return false;
    }
    if (result.path.count && vector_path_contour_area(&result.path, 0) < 0) {
        for (unsigned i = 0; i < result.path.count / 2; i++) {
            VectorPoint swap = result.path.points[i];
            result.path.points[i] = result.path.points[result.path.count - 1 - i];
            result.path.points[result.path.count - 1 - i] = swap;
        }
    }
    *s = result;
    return true;
}

VectorBezier vector_curve_parent(const VectorShape *s) {
    VectorBezier curve = s->curve;
    for (unsigned i = 0; i < curve.count; i++) {
        VectorBezierNode *n = &curve.nodes[i];
        VectorPoint *points[] = {&n->point, &n->incoming, &n->outgoing};
        for (int j = 0; j < 3; j++) {
            points[j]->x = s->rect.x + points[j]->x * s->rect.w;
            points[j]->y = s->rect.y + points[j]->y * s->rect.h;
            vector_rotate_point(&points[j]->x, &points[j]->y, s->rect.x + s->rect.w / 2,
                                s->rect.y + s->rect.h / 2, s->angle);
        }
    }
    return curve;
}

bool vector_curve_add(VectorDocument *d, unsigned parent, const VectorBezier *curve) {
    VectorShape shape = {
        .parent_id = parent, .fill = 0, .stroke = {0x394956ff, 2, VECTOR_STROKE_CENTER}};
    snprintf(shape.name, sizeof shape.name, "Kurve %u", d->next_rectangle_id);
    return vector_curve_shape(&shape, curve) && vector_shape_add(d, &shape);
}

bool vector_curve_update(VectorDocument *d, unsigned id, const VectorBezier *curve) {
    VectorShape *s = vector_rectangle_find(d, id);
    if (!s || !s->curve.count ||
        vector_rectangle_effective_flags(d, s) & VECTOR_OBJECT_STATE_MASK) {
        return false;
    }
    VectorShape after = *s;
    return vector_curve_shape(&after, curve) && memcmp(s, &after, sizeof *s) &&
           vector_shape_update(d, &after);
}
