/** @file @brief Export geometry stays independent of widgets, renderer state and file dialogs. */
#include "plan.h"
#include "editors/vector/document/selection.h"
#include "editors/vector/geometry/rotation.h"
#include "editors/vector/instance/instance.h"
#include "editors/vector/mask/mask.h"
#include "editors/vector/repeat/repeat.h"
#include <math.h>
#include <stdio.h>

static PUIRect join(PUIRect a, PUIRect b) {
    float r = fmaxf(a.x + a.w, b.x + b.w), bottom = fmaxf(a.y + a.h, b.y + b.h);
    a.x = fminf(a.x, b.x);
    a.y = fminf(a.y, b.y);
    a.w = r - a.x;
    a.h = bottom - a.y;
    return a;
}

static bool intersect(PUIRect *a, PUIRect b) {
    float r = fminf(a->x + a->w, b.x + b.w), bottom = fminf(a->y + a->h, b.y + b.h);
    a->x = fmaxf(a->x, b.x);
    a->y = fmaxf(a->y, b.y);
    a->w = r - a->x;
    a->h = bottom - a->y;
    return a->w > 0 && a->h > 0;
}

static PUIRect bounds(const VectorShape *s) {
    float stroke = s->stroke.alignment == VECTOR_STROKE_INSIDE   ? 0
                   : s->stroke.alignment == VECTOR_STROKE_CENTER ? s->stroke.width / 2
                                                                 : s->stroke.width;
    PUIRect b = vector_rotated_bounds((PUIRect){s->rect.x - stroke, s->rect.y - stroke,
                                                s->rect.w + 2 * stroke, s->rect.h + 2 * stroke},
                                      s->angle);
    if (s->shadow.enabled && (s->shadow.color & 255)) {
        float margin = 3 * ceilf(s->shadow.blur / 3) + s->shadow.spread + 2;
        b = join(b, (PUIRect){b.x + s->shadow.offset_x - margin, b.y + s->shadow.offset_y - margin,
                              b.w + 2 * margin, b.h + 2 * margin});
    }
    return b;
}

static bool selected_group_content(const VectorDocument *d, unsigned group) {
    for (int i = 0; i < d->rectangle_count; i++) {
        const VectorShape *s = &d->rectangles[i];
        if (group && s->group_id == group && !s->clip_mask &&
            vector_document_is_selected(d, s->id)) {
            return true;
        }
    }
    return false;
}

static bool append(VectorExportPlan *p, unsigned board, PUIRect b) {
    float r = ceilf(b.x + b.w), bottom = ceilf(b.y + b.h);
    if (p->scope == VECTOR_EXPORT_SELECTION) {
        b.x = floorf(b.x);
        b.y = floorf(b.y);
        b.w = r - b.x;
        b.h = bottom - b.y;
    } else {
        b.w = ceilf(b.w);
        b.h = ceilf(b.h);
    }
    if (!isfinite(b.w) || !isfinite(b.h) || b.w < 1 || b.h < 1 || b.w > 8192 || b.h > 8192 ||
        (double)b.w * b.h > 16777216) {
        return false;
    }
    p->items[p->count++] = (VectorExportItem){board, b};
    return true;
}

bool vector_export_plan(const VectorDocument *d, VectorExportScope scope, VectorExportPlan *p,
                        char *error, size_t capacity) {
    snprintf(error, capacity, "Ein Dokument und einen Exportbereich auswaehlen");
    *p = (VectorExportPlan){.scope = scope};
    if (!d || scope > VECTOR_EXPORT_SELECTION || scope < VECTOR_EXPORT_ALL) {
        return false;
    }
    if (scope != VECTOR_EXPORT_SELECTION) {
        for (int i = 0; i < d->artboard_count; i++) {
            const VectorArtboard *b = &d->artboards[i];
            if (scope == VECTOR_EXPORT_ALL || b->id == d->active_artboard) {
                if (!append(p, b->id, b->rect)) {
                    goto size_error;
                }
            }
        }
    } else {
        PUIRect total = {0};
        bool any = false;
        for (int i = 0; i < d->rectangle_count; i++) {
            VectorShape resolved;
            vector_instance_resolve(d, &d->rectangles[i], &resolved);
            const VectorShape *s = &resolved;
            if (!vector_document_is_selected(d, s->id) || (s->flags & VECTOR_OBJECT_HIDDEN)) {
                continue;
            }
            if (s->clip_mask && selected_group_content(d, s->group_id)) {
                continue;
            }
            const VectorArtboard *board = NULL;
            for (int j = 0; j < d->artboard_count; j++) {
                if (d->artboards[j].id == s->parent_id) {
                    board = &d->artboards[j];
                }
            }
            if (!board) {
                continue;
            }
            unsigned cols = s->repeat.enabled ? s->repeat.columns : 1,
                     rows = s->repeat.enabled ? s->repeat.rows : 1;
            for (unsigned y = 0; y < rows; y++) {
                for (unsigned x = 0; x < cols; x++) {
                    VectorShape instance = *s;
                    if (s->repeat.enabled &&
                        !vector_repeat_instance(s, &s->repeat, x, y, &instance)) {
                        continue;
                    }
                    PUIRect b = bounds(&instance);
                    const VectorShape *mask = vector_mask_find(d, s->group_id);
                    if (mask && !s->clip_mask) {
                        if ((mask->flags & VECTOR_OBJECT_HIDDEN) ||
                            !intersect(&b, vector_rotated_bounds(vector_instance_rect(d, mask),
                                                                 mask->angle))) {
                            continue;
                        }
                    }
                    if (!intersect(&b, (PUIRect){0, 0, board->rect.w, board->rect.h})) {
                        continue;
                    }
                    b.x += board->rect.x;
                    b.y += board->rect.y;
                    total = any ? join(total, b) : b;
                    any = true;
                }
            }
        }
        if (any && !append(p, 0, total)) {
            goto size_error;
        }
    }
    if (!p->count) {
        snprintf(error, capacity, "Artboard oder sichtbare Objekte auswaehlen");
        return false;
    }
    return true;
size_error:
    snprintf(error, capacity, "Export zu gross: maximal 8192 px pro Achse und 16 Millionen Pixel");
    return false;
}
