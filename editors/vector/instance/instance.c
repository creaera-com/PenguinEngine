/** @file @brief Source references are resolved at read time; history stores only authoritative
 * values. */
#include "instance.h"
#include "editors/vector/document/selection.h"
#include "editors/vector/object/clipboard.h"
#include <math.h>
#include <stdlib.h>
#include <string.h>

static const VectorShape *find(const VectorDocument *d, unsigned id) {
    for (int i = 0; i < d->rectangle_count; i++) {
        if (d->rectangles[i].id == id) {
            return &d->rectangles[i];
        }
    }
    return NULL;
}

static float dimension(float source, float own, float basis) {
    return fmaxf(1, fminf(VECTOR_MAX_ARTBOARD_SIZE, source * own / basis));
}

void vector_instance_resolve(const VectorDocument *d, const VectorShape *s, VectorShape *out) {
    *out = *s;
    const VectorShape *source = s;
    for (int depth = 0; source->instance_source && depth < VECTOR_MAX_RECTANGLES; depth++) {
        source = find(d, source->instance_source);
        if (!source || source == s) {
            return;
        }
    }
    if (!s->instance_source || source->instance_source) {
        return;
    }
    *out = *source;
    out->id = s->id;
    out->parent_id = s->parent_id;
    out->flags = s->flags;
    out->group_id = s->group_id;
    out->clip_mask = s->clip_mask;
    memcpy(out->name, s->name, sizeof out->name);
    out->rect =
        (PUIRect){s->rect.x, s->rect.y, dimension(source->rect.w, s->rect.w, s->instance_basis_w),
                  dimension(source->rect.h, s->rect.h, s->instance_basis_h)};
    out->angle = s->angle;
    out->instance_source = 0;
    out->instance_basis_w = out->instance_basis_h = 0;
}

PUIRect vector_instance_rect(const VectorDocument *d, const VectorShape *s) {
    if (!s->instance_source) {
        return s->rect;
    }
    VectorShape resolved;
    vector_instance_resolve(d, s, &resolved);
    return resolved.rect;
}

PUIRect vector_instance_store_rect(const VectorDocument *d, const VectorShape *s, PUIRect r) {
    PUIRect display = vector_instance_rect(d, s);
    if (s->instance_source) {
        r.w = r.w == display.w ? s->rect.w : r.w * s->rect.w / display.w;
        r.h = r.h == display.h ? s->rect.h : r.h * s->rect.h / display.h;
    }
    return r;
}

bool vector_instance_edit_valid(const VectorShape *before, const VectorShape *after) {
    if (!before->instance_source || !after->instance_source) {
        return true;
    }
    VectorShape allowed = *before;
    allowed.rect = after->rect;
    allowed.angle = after->angle;
    allowed.flags = after->flags;
    allowed.group_id = after->group_id;
    allowed.clip_mask = after->clip_mask;
    memcpy(allowed.name, after->name, sizeof allowed.name);
    return !memcmp(&allowed, after, sizeof allowed);
}

bool vector_instances_valid(const VectorShape *shapes, int count) {
    for (int i = 0; i < count; i++) {
        unsigned source = shapes[i].instance_source;
        for (int depth = 0; source; depth++) {
            if (source == shapes[i].id || depth >= count) {
                return false;
            }
            unsigned next = 0;
            for (int j = 0; j < count; j++) {
                if (shapes[j].id == source) {
                    next = shapes[j].instance_source;
                    break;
                }
            }
            source = next;
        }
    }
    return true;
}

static bool prepare(VectorDocument *d, VectorClipboard *copy) {
    if (!d || !d->active_rectangle || !vector_objects_copy(d, copy) || copy->whole_board ||
        !copy->rectangle_count || !vector_objects_can_duplicate(d)) {
        return false;
    }
    for (int i = 0; i < copy->rectangle_count; i++) {
        VectorShape *s = &copy->rectangles[i];
        if (vector_rectangle_effective_flags(d, s) & VECTOR_OBJECT_STATE_MASK) {
            return false;
        }
        unsigned source = s->instance_source ? s->instance_source : s->id;
        if (!find(d, source)) {
            return false;
        }
    }
    return true;
}

bool vector_instances_can_create(VectorDocument *d) {
    VectorClipboard *copy = malloc(sizeof *copy);
    bool ok = copy && prepare(d, copy);
    free(copy);
    return ok;
}

bool vector_instances_create(VectorDocument *d) {
    VectorClipboard *copy = malloc(sizeof *copy);
    bool ok = copy && prepare(d, copy) && vector_objects_duplicate_values(d, copy);
    free(copy);
    return ok;
}

bool vector_instances_detach(VectorDocument *d) {
    VectorShape *shapes = calloc(VECTOR_MAX_RECTANGLES, sizeof *shapes);
    if (!d || !shapes) {
        free(shapes);
        return false;
    }
    int count = 0;
    bool linked = false;
    for (int i = 0; i < d->rectangle_count; i++) {
        if (vector_document_is_selected(d, d->rectangles[i].id)) {
            linked |= d->rectangles[i].instance_source != 0;
            vector_instance_resolve(d, &d->rectangles[i], &shapes[count++]);
        }
    }
    bool ok = linked && vector_objects_replace_selection(d, shapes, count);
    free(shapes);
    return ok;
}
