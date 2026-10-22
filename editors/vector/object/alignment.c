/** @file @brief Axis-aligned bounds of rotated objects drive alignment and equal-gap placement. */
#include "alignment.h"
#include "editors/vector/geometry/rotation.h"
#include "editors/vector/instance/instance.h"
#include "editors/vector/rectangle/transform.h"
#include <math.h>

/** @brief Target rectangles stay parent-local; bounds are world-space and exclude stroke width. */
typedef struct {
    unsigned ids[VECTOR_MAX_RECTANGLES];
    PUIRect rects[VECTOR_MAX_RECTANGLES], bounds[VECTOR_MAX_RECTANGLES];
    int count;
} Alignment;

static float start(PUIRect rect, bool horizontal) {
    return horizontal ? rect.x : rect.y;
}

static float extent(PUIRect rect, bool horizontal) {
    return horizontal ? rect.w : rect.h;
}

static bool prepare(VectorDocument *d, VectorAlignment kind, Alignment *plan) {
    if (kind < 0 || kind >= VECTOR_ALIGNMENT_COUNT ||
        d->history.cursor >= VECTOR_HISTORY_CAPACITY) {
        return false;
    }
    bool distribute = kind == VECTOR_DISTRIBUTE_X || kind == VECTOR_DISTRIBUTE_Y;
    bool horizontal = kind <= VECTOR_DISTRIBUTE_X;
    *plan = (Alignment){0};
    for (int i = 0; i < d->rectangle_count; i++) {
        VectorRectangle *r = &d->rectangles[i];
        if (!vector_document_is_selected(d, r->id)) {
            continue;
        }
        VectorArtboard *parent = vector_artboard_find(d, r->parent_id);
        if (!parent || (vector_rectangle_effective_flags(d, r) & VECTOR_OBJECT_STATE_MASK)) {
            return false;
        }
        int at = plan->count++;
        plan->ids[at] = r->id;
        plan->rects[at] = vector_instance_rect(d, r);
        plan->bounds[at] = vector_rotated_bounds(vector_instance_rect(d, r), r->angle);
        plan->bounds[at].x += parent->rect.x;
        plan->bounds[at].y += parent->rect.y;
    }
    if (plan->count < (distribute ? 3 : 2)) {
        return false;
    }
    float first = start(plan->bounds[0], horizontal);
    float last = first + extent(plan->bounds[0], horizontal), total = 0;
    int sorted[VECTOR_MAX_RECTANGLES];
    for (int i = 0; i < plan->count; i++) {
        float edge = start(plan->bounds[i], horizontal);
        first = fminf(first, edge);
        last = fmaxf(last, edge + extent(plan->bounds[i], horizontal));
        total += extent(plan->bounds[i], horizontal);
        // Stable spatial ordering: ties retain the current model/paint order.
        int j = i;
        while (j > 0 && start(plan->bounds[sorted[j - 1]], horizontal) > edge) {
            sorted[j] = sorted[j - 1];
            j--;
        }
        sorted[j] = i;
    }
    float gap = (last - first - total) / (plan->count - 1);
    float cursor = first;
    bool changed = false;
    for (int i = 0; i < plan->count; i++) {
        int at = distribute ? sorted[i] : i;
        float edge = start(plan->bounds[at], horizontal);
        float size = extent(plan->bounds[at], horizontal);
        float target = first;
        if (distribute) {
            target = cursor;
            cursor += size + gap;
        } else if (kind == VECTOR_ALIGN_CENTER_X || kind == VECTOR_ALIGN_CENTER_Y) {
            target = (first + last - size) / 2;
        } else if (kind == VECTOR_ALIGN_RIGHT || kind == VECTOR_ALIGN_BOTTOM) {
            target = last - size;
        }
        float delta = target - edge;
        if (fabsf(delta) > .0001f) {
            if (horizontal) {
                plan->rects[at].x += delta;
            } else {
                plan->rects[at].y += delta;
            }
            changed = true;
        }
        PUIRect rect = plan->rects[at];
        if (!isfinite(rect.x) || !isfinite(rect.y) || fabsf(rect.x) > VECTOR_MAX_ARTBOARD_SIZE ||
            fabsf(rect.y) > VECTOR_MAX_ARTBOARD_SIZE) {
            return false;
        }
    }
    return changed;
}

bool vector_objects_can_align(VectorDocument *d, VectorAlignment alignment) {
    Alignment plan;
    return prepare(d, alignment, &plan);
}

bool vector_objects_align(VectorDocument *d, VectorAlignment alignment) {
    Alignment plan;
    return prepare(d, alignment, &plan) &&
           vector_rectangles_set_rects(d, plan.ids, plan.rects, plan.count);
}
