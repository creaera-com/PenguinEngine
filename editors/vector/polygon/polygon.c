/** @file @brief Polygon geometry and history-backed parameter changes. */
#include "polygon.h"
#include "editors/vector/object/commands.h"
#include <math.h>

bool vector_polygon_shape(PUIRect rect, unsigned sides, VectorShape *out) {
    if (!out || sides < 3 || sides > VECTOR_POLYGON_MAX_SIDES || !isfinite(rect.x) ||
        !isfinite(rect.y) || !isfinite(rect.w) || !isfinite(rect.h) ||
        fabsf(rect.x) > VECTOR_MAX_ARTBOARD_SIZE || fabsf(rect.y) > VECTOR_MAX_ARTBOARD_SIZE ||
        rect.w < 1 || rect.h < 1 || rect.w > VECTOR_MAX_ARTBOARD_SIZE ||
        rect.h > VECTOR_MAX_ARTBOARD_SIZE) {
        return false;
    }
    VectorShape shape = {.rect = rect,
                         .fill = VECTOR_RECTANGLE_DEFAULT_FILL,
                         .curve_kind = VECTOR_CURVE_POLYGON,
                         .curve = {.count = sides, .closed = 1},
                         .path = {.count = sides, .contours = 1}};
    shape.path.ends[0] = sides;
    for (unsigned i = 0; i < sides; i++) {
        float angle = -1.570796327f + i * 6.283185307f / sides;
        VectorPoint point = {.5f + .5f * cosf(angle), .5f + .5f * sinf(angle)};
        shape.curve.nodes[i] = (VectorBezierNode){point, point, point};
        shape.path.points[i] = point;
    }
    *out = shape;
    return true;
}

bool vector_polygon_add(VectorDocument *document, unsigned parent, PUIRect rect, unsigned sides) {
    VectorShape shape;
    if (!vector_polygon_shape(rect, sides, &shape)) {
        return false;
    }
    shape.parent_id = parent;
    snprintf(shape.name, sizeof shape.name, "Polygon %u", document->next_rectangle_id);
    return vector_shape_add(document, &shape);
}

bool vector_polygon_set_sides(VectorDocument *document, unsigned id, unsigned sides) {
    VectorShape *current = vector_rectangle_find(document, id), geometry;
    if (!current || current->curve_kind != VECTOR_CURVE_POLYGON ||
        !vector_polygon_shape(current->rect, sides, &geometry)) {
        return false;
    }
    VectorShape after = *current;
    after.curve = geometry.curve;
    after.path = geometry.path;
    return vector_shape_update(document, &after);
}
