/** @file @brief The ellipse factory owns its geometry; mouse gestures remain in the workspace. */
#include "ellipse.h"
#include "editors/vector/object/commands.h"
#include <math.h>

bool vector_ellipse_shape(PUIRect rect, VectorShape *out) {
    if (!out || !isfinite(rect.x) || !isfinite(rect.y) || !isfinite(rect.w) || !isfinite(rect.h) ||
        fabsf(rect.x) > VECTOR_MAX_ARTBOARD_SIZE || fabsf(rect.y) > VECTOR_MAX_ARTBOARD_SIZE ||
        rect.w < 1 || rect.h < 1 || rect.w > VECTOR_MAX_ARTBOARD_SIZE ||
        rect.h > VECTOR_MAX_ARTBOARD_SIZE) {
        return false;
    }
    const float k = .276142375f; // Half the standard cubic circle constant (4/3*tan(pi/8)).
    VectorShape shape = {.rect = rect,
                         .fill = VECTOR_RECTANGLE_DEFAULT_FILL,
                         .curve_kind = VECTOR_CURVE_ELLIPSE,
                         .ellipse = {.end = 360},
                         .curve = {.count = 4,
                                   .closed = 1,
                                   .nodes = {{{.5f, 0}, {.5f - k, 0}, {.5f + k, 0}},
                                             {{1, .5f}, {1, .5f - k}, {1, .5f + k}},
                                             {{.5f, 1}, {.5f + k, 1}, {.5f - k, 1}},
                                             {{0, .5f}, {0, .5f + k}, {0, .5f - k}}}}};
    if (!vector_bezier_flatten(&shape.curve, .001f, &shape.path)) {
        return false;
    }
    *out = shape;
    return true;
}

bool vector_ellipse_add(VectorDocument *document, unsigned parent, PUIRect rect) {
    VectorShape shape;
    if (!vector_ellipse_shape(rect, &shape)) {
        return false;
    }
    shape.parent_id = parent;
    snprintf(shape.name, sizeof shape.name, "Ellipse %u", document->next_rectangle_id);
    return vector_shape_add(document, &shape);
}

bool vector_ellipse_parameters_valid(VectorEllipse value) {
    float sweep = value.end - value.start;
    return value.mode <= VECTOR_ELLIPSE_CHORD && isfinite(value.start) && isfinite(value.end) &&
           fabsf(value.start) <= 360 && fabsf(value.end) <= 360 &&
           (value.mode == VECTOR_ELLIPSE_FULL || (fabsf(sweep) >= .01f && fabsf(sweep) <= 360));
}

bool vector_ellipse_set_segment(VectorDocument *document, unsigned id, VectorEllipse value) {
    VectorShape *current = vector_rectangle_find(document, id), geometry;
    if (!current || current->curve_kind != VECTOR_CURVE_ELLIPSE ||
        !vector_ellipse_parameters_valid(value) ||
        !vector_ellipse_shape(current->rect, &geometry)) {
        return false;
    }
    VectorShape after = *current;
    after.ellipse = value;
    after.curve = geometry.curve;
    after.path = geometry.path;
    if (value.mode != VECTOR_ELLIPSE_FULL &&
        (value.mode == VECTOR_ELLIPSE_ARC || fabsf(value.end - value.start) < 360)) {
        VectorBezier curve = {0};
        float sweep = (value.end - value.start) * .01745329252f;
        int segments = (int)ceilf(fabsf(sweep) / 1.570796327f);
        float step = sweep / segments, tangent = (4.f / 3.f) * tanf(step / 4);
        curve.count = (unsigned)segments + 1;
        curve.closed = value.mode != VECTOR_ELLIPSE_ARC;
        for (int i = 0; i <= segments; i++) {
            float a = value.start * .01745329252f + i * step;
            VectorPoint p = {.5f + .5f * cosf(a), .5f + .5f * sinf(a)};
            VectorPoint delta = {-.5f * sinf(a) * tangent, .5f * cosf(a) * tangent};
            curve.nodes[i] = (VectorBezierNode){
                p, {p.x - delta.x, p.y - delta.y}, {p.x + delta.x, p.y + delta.y}};
        }
        curve.nodes[0].incoming = curve.nodes[0].point;
        curve.nodes[segments].outgoing = curve.nodes[segments].point;
        if (value.mode == VECTOR_ELLIPSE_PIE) {
            VectorPoint center = {.5f, .5f};
            curve.nodes[curve.count++] = (VectorBezierNode){center, center, center};
        }
        after.curve = curve;
        after.path = (VectorPath){0};
        if (curve.closed && !vector_bezier_flatten(&curve, .001f, &after.path)) {
            return false;
        }
        /* Open arcs have no fill; give formerly fill-only ellipses a visible contour. */
        if (!curve.closed && after.stroke.width == 0) {
            after.stroke.width = 1;
            after.stroke.color = after.fill;
        }
    }
    return vector_shape_update(document, &after);
}
