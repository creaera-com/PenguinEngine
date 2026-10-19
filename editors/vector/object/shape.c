/** @file @brief Generic path mapping and filled-area picking for vector shapes. */
#include "shape.h"
#include "editors/vector/geometry/rotation.h"

VectorPath vector_shape_path(const VectorShape *shape, PUIRect rect, float angle) {
    VectorPath path = shape->path;
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

bool vector_shape_contains(const VectorShape *shape, float x, float y) {
    if (!shape->path.count) {
        return vector_rotated_contains(shape->rect, shape->angle, x, y);
    }
    vector_rotate_point(&x, &y, shape->rect.x + shape->rect.w / 2,
                        shape->rect.y + shape->rect.h / 2, -shape->angle);
    return vector_path_contains(&shape->path, (x - shape->rect.x) / shape->rect.w,
                                (y - shape->rect.y) / shape->rect.h);
}

bool vector_shape_intersects(const VectorShape *shape, PUIRect box) {
    if (!shape->path.count) {
        return vector_rotated_intersects(shape->rect, shape->angle, box);
    }
    VectorPath path = vector_shape_path(shape, shape->rect, shape->angle);
    return vector_path_intersects(&path, box);
}
