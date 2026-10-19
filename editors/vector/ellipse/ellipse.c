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
