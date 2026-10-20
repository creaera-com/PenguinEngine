/** @file @brief Reflection reverses contour winding back to preserve nonzero fills and holes. */
#include "orientation.h"
#include "clipboard.h"
#include "editors/vector/geometry/rotation.h"
#include <math.h>
#include <string.h>

typedef struct {
    VectorShape shapes[VECTOR_MAX_RECTANGLES];
    int count;
} Orientation;

static void mirror_path(VectorPath *path, bool horizontal) {
    for (unsigned c = 0; c < path->contours; c++) {
        unsigned first = c ? path->ends[c - 1] : 0, end = path->ends[c];
        for (unsigned i = first; i < end; i++) {
            if (horizontal) {
                path->points[i].x = 1 - path->points[i].x;
            } else {
                path->points[i].y = 1 - path->points[i].y;
            }
        }
        for (unsigned i = 0; i < (end - first) / 2; i++) {
            VectorPoint swap = path->points[first + i];
            path->points[first + i] = path->points[end - 1 - i];
            path->points[end - 1 - i] = swap;
        }
    }
}

static bool prepare(VectorDocument *d, VectorOrientation op, Orientation *plan) {
    if (op < VECTOR_MIRROR_HORIZONTAL || op > VECTOR_ROTATE_RIGHT || !d->active_rectangle ||
        d->history.cursor >= VECTOR_HISTORY_CAPACITY) {
        return false;
    }
    plan->count = 0;
    float left = INFINITY, top = INFINITY, right = -INFINITY, bottom = -INFINITY;
    for (int i = 0; i < d->rectangle_count; i++) {
        VectorShape *shape = &d->rectangles[i];
        if (!vector_document_is_selected(d, shape->id)) {
            continue;
        }
        if (vector_rectangle_effective_flags(d, shape) & VECTOR_OBJECT_STATE_MASK) {
            return false;
        }
        VectorArtboard *parent = vector_artboard_find(d, shape->parent_id);
        if (!parent) {
            return false;
        }
        PUIRect bounds = vector_rotated_bounds(shape->rect, shape->angle);
        bounds.x += parent->rect.x;
        bounds.y += parent->rect.y;
        left = fminf(left, bounds.x);
        top = fminf(top, bounds.y);
        right = fmaxf(right, bounds.x + bounds.w);
        bottom = fmaxf(bottom, bounds.y + bounds.h);
        plan->shapes[plan->count++] = *shape;
    }
    if (!plan->count) {
        return false;
    }
    float px = (left + right) / 2, py = (top + bottom) / 2;
    bool changed = false;
    for (int i = 0; i < plan->count; i++) {
        VectorShape *after = &plan->shapes[i];
        VectorShape *before = vector_rectangle_find(d, after->id);
        VectorArtboard *parent = vector_artboard_find(d, after->parent_id);
        float cx = parent->rect.x + after->rect.x + after->rect.w / 2;
        float cy = parent->rect.y + after->rect.y + after->rect.h / 2;
        if (op == VECTOR_MIRROR_HORIZONTAL || op == VECTOR_MIRROR_VERTICAL) {
            bool horizontal = op == VECTOR_MIRROR_HORIZONTAL;
            if (horizontal) {
                cx = 2 * px - cx;
            } else {
                cy = 2 * py - cy;
            }
            if (*after->image.source) {
                if (horizontal) {
                    after->image.flip_x ^= 1u;
                } else {
                    after->image.flip_y ^= 1u;
                }
            }
            if (horizontal) {
                after->gradient.x1 = 1 - after->gradient.x1;
                after->gradient.x2 = 1 - after->gradient.x2;
            } else {
                after->gradient.y1 = 1 - after->gradient.y1;
                after->gradient.y2 = 1 - after->gradient.y2;
            }
            if (!after->gradient.count) {
                after->gradient = (VectorGradient){0};
            }
            if (after->gradient.kind == VECTOR_GRADIENT_CONICAL) {
                VectorGradient original = after->gradient;
                for (unsigned i = 0; i < original.count; i++) {
                    after->gradient.stops[i] = original.stops[original.count - 1 - i];
                    after->gradient.stops[i].position = 1 - after->gradient.stops[i].position;
                }
            }
            mirror_path(&after->path, horizontal);
            for (unsigned n = 0; n < after->curve.count; n++) {
                VectorBezierNode *node = &after->curve.nodes[n];
                VectorPoint *points[] = {&node->point, &node->incoming, &node->outgoing};
                for (int j = 0; j < 3; j++) {
                    if (horizontal) {
                        points[j]->x = 1 - points[j]->x;
                    } else {
                        points[j]->y = 1 - points[j]->y;
                    }
                }
            }
            after->angle = vector_angle_turn(-after->angle);
        } else {
            float dx = cx - px, dy = cy - py;
            bool left_turn = op == VECTOR_ROTATE_LEFT;
            cx = px + (left_turn ? dy : -dy);
            cy = py + (left_turn ? -dx : dx);
            after->angle = vector_angle_turn(after->angle + (left_turn ? -90 : 90));
        }
        if (after->angle == 0) {
            after->angle = 0;
        } // Canonical positive zero.
        // A single object's pivot is its own center: avoid a lossy world/local round-trip.
        if (plan->count > 1) {
            after->rect.x = cx - parent->rect.x - after->rect.w / 2;
            after->rect.y = cy - parent->rect.y - after->rect.h / 2;
        }
        if (!vector_rectangle_valid(*after)) {
            return false;
        }
        changed |= memcmp(before, after, sizeof *after) != 0;
    }
    return changed;
}

bool vector_objects_can_orient(VectorDocument *document, VectorOrientation operation) {
    Orientation plan;
    return prepare(document, operation, &plan);
}

bool vector_objects_orient(VectorDocument *document, VectorOrientation operation) {
    Orientation plan;
    return prepare(document, operation, &plan) &&
           vector_objects_replace_selection(document, plan.shapes, plan.count);
}
