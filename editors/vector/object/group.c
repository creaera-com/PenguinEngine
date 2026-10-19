/** @file @brief Group membership is a structural document edit, independent of UI and input. */
#include "group.h"
#include "clipboard.h"
#include <limits.h>

static bool selection(VectorDocument *d, unsigned *parent, unsigned *group, int *count) {
    *parent = *group = 0;
    *count = 0;
    if (!d->active_rectangle || !vector_objects_can_delete(d)) {
        return false;
    }
    for (int i = 0; i < d->rectangle_count; i++) {
        VectorShape *shape = &d->rectangles[i];
        if (!vector_document_is_selected(d, shape->id)) {
            continue;
        }
        if (*parent && *parent != shape->parent_id) {
            return false;
        }
        *parent = shape->parent_id;
        *group |= shape->group_id;
        (*count)++;
        /* A partial group must never be silently regrouped or split. */
        for (int j = 0; shape->group_id && j < d->rectangle_count; j++) {
            if (d->rectangles[j].group_id == shape->group_id &&
                !vector_document_is_selected(d, d->rectangles[j].id)) {
                return false;
            }
        }
    }
    return *count > 0;
}

bool vector_objects_can_group(VectorDocument *d) {
    unsigned parent, group;
    int count;
    if (!selection(d, &parent, &group, &count) || count < 2) {
        return false;
    }
    unsigned existing = 0;
    for (int i = 0; i < d->rectangle_count; i++) {
        VectorShape r = d->rectangles[i];
        if (vector_document_is_selected(d, r.id)) {
            if (!r.group_id || (existing && existing != r.group_id)) {
                return true;
            }
            existing = r.group_id;
        }
    }
    return false;
}

bool vector_objects_can_ungroup(VectorDocument *d) {
    unsigned parent, group;
    int count;
    return selection(d, &parent, &group, &count) && group != 0;
}

static bool change(VectorDocument *d, bool grouped) {
    if (grouped ? !vector_objects_can_group(d) : !vector_objects_can_ungroup(d)) {
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
        shapes[i].group_id = grouped ? group : 0;
    }
    return grouped ? vector_objects_replace_selection_block(d, shapes, count)
                   : vector_objects_replace_selection(d, shapes, count);
}

bool vector_objects_group(VectorDocument *d) {
    return change(d, true);
}

bool vector_objects_ungroup(VectorDocument *d) {
    return change(d, false);
}
