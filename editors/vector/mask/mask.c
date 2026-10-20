/** @file @brief A clipping group owns one special mask child; original geometry/style survive. */
#include "mask.h"
#include "editors/vector/object/clipboard.h"
#include <limits.h>

const VectorShape *vector_mask_find(const VectorDocument *d, unsigned group) {
    for (int i = 0; group && i < d->rectangle_count; i++) {
        if (d->rectangles[i].group_id == group && d->rectangles[i].clip_mask) {
            return &d->rectangles[i];
        }
    }
    return NULL;
}

static bool complete(VectorDocument *d) {
    unsigned parent = 0;
    if (!d->active_rectangle || !vector_objects_can_delete(d)) {
        return false;
    }
    for (int i = 0; i < d->rectangle_count; i++) {
        const VectorShape *s = &d->rectangles[i];
        if (!vector_document_is_selected(d, s->id)) {
            continue;
        }
        if (parent && parent != s->parent_id) {
            return false;
        }
        parent = s->parent_id;
        for (int j = 0; s->group_id && j < d->rectangle_count; j++) {
            if (d->rectangles[j].group_id == s->group_id &&
                !vector_document_is_selected(d, d->rectangles[j].id)) {
                return false;
            }
        }
    }
    return parent != 0;
}

bool vector_mask_can_create(VectorDocument *d) {
    if (!complete(d)) {
        return false;
    }
    const VectorShape *top = NULL;
    int count = 0;
    for (int i = 0; i < d->rectangle_count; i++) {
        const VectorShape *s = &d->rectangles[i];
        if (vector_document_is_selected(d, s->id)) {
            if (vector_mask_find(d, s->group_id)) {
                return false; // Nested clipping groups need a genuine nested hierarchy first.
            }
            top = s;
            count++;
        }
    }
    if (count < 2 || !top || top->group_id || *top->image.source || top->text.enabled ||
        (top->curve.count && !top->curve.closed)) {
        return false;
    }
    VectorPath path = vector_shape_path(top, top->rect, top->angle);
    return path.count >= 3 && vector_path_valid(&path);
}

bool vector_mask_create(VectorDocument *d) {
    if (!vector_mask_can_create(d)) {
        return false;
    }
    VectorShape shapes[VECTOR_MAX_RECTANGLES];
    int count = 0;
    unsigned group = UINT_MAX;
    for (int i = 0; i < d->rectangle_count; i++) {
        if (vector_document_is_selected(d, d->rectangles[i].id)) {
            shapes[count++] = d->rectangles[i];
            if (d->rectangles[i].id < group) {
                group = d->rectangles[i].id;
            }
        }
    }
    for (int i = 0; i < count; i++) {
        shapes[i].group_id = group;
        shapes[i].clip_mask = i == count - 1;
    }
    return vector_objects_replace_selection_block(d, shapes, count);
}

bool vector_mask_can_release(VectorDocument *d) {
    if (!complete(d)) {
        return false;
    }
    for (int i = 0; i < d->rectangle_count; i++) {
        if (d->rectangles[i].clip_mask && vector_document_is_selected(d, d->rectangles[i].id)) {
            return true;
        }
    }
    return false;
}

bool vector_mask_release(VectorDocument *d) {
    if (!vector_mask_can_release(d)) {
        return false;
    }
    VectorShape shapes[VECTOR_MAX_RECTANGLES];
    int count = 0;
    for (int i = 0; i < d->rectangle_count; i++) {
        if (vector_document_is_selected(d, d->rectangles[i].id)) {
            shapes[count] = d->rectangles[i];
            shapes[count++].clip_mask = 0;
        }
    }
    return vector_objects_replace_selection(d, shapes, count);
}

bool vector_mask_contains(const VectorDocument *d, const VectorShape *s, float x, float y) {
    if (s->clip_mask || !vector_shape_contains(s, x, y)) {
        return false;
    }
    const VectorShape *mask = vector_mask_find(d, s->group_id);
    if (!mask) {
        return true;
    }
    if (mask->flags & VECTOR_OBJECT_HIDDEN) {
        return false;
    }
    VectorPath path = vector_shape_path(mask, mask->rect, mask->angle);
    return vector_path_contains(&path, x, y);
}

bool vector_mask_intersects(const VectorDocument *d, const VectorShape *s, PUIRect box) {
    if (s->clip_mask || !vector_shape_intersects(s, box)) {
        return false;
    }
    const VectorShape *mask = vector_mask_find(d, s->group_id);
    if (!mask) {
        return true;
    }
    if (mask->flags & VECTOR_OBJECT_HIDDEN) {
        return false;
    }
    VectorPath a = vector_shape_path(s, s->rect, s->angle);
    VectorPath b = vector_shape_path(mask, mask->rect, mask->angle), intersection;
    return vector_path_combine(&a, &b, VECTOR_AREA_INTERSECTION, &intersection) &&
           vector_path_intersects(&intersection, box);
}
