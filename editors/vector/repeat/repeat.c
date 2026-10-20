/** @file @brief Instances borrow current source values; linked grids allocate no object copies. */
#include "repeat.h"
#include "editors/vector/geometry/rotation.h"
#include "editors/vector/object/clipboard.h"
#include "editors/vector/object/commands.h"
#include <math.h>
#include <stdlib.h>

static float clamp(float value, float lo, float hi) {
    return fmaxf(lo, fminf(hi, value));
}

static uint32_t color(uint32_t rgba, const float *delta) {
    PUIHSLColor hsl = pui_color_to_hsl(rgba);
    hsl.h += delta[0];
    hsl.s += delta[1] / 100;
    hsl.l += delta[2] / 100;
    hsl.a += delta[3] / 100;
    return pui_color_from_hsl(hsl);
}

bool vector_repeat_instance(const VectorShape *source, const VectorRepeat *r, unsigned x,
                            unsigned y, VectorShape *out) {
    if (!source || !out || !r->enabled || !vector_repeat_valid(r) || x >= r->columns ||
        y >= r->rows) {
        return false;
    }
    VectorShape result = *source;
    result.repeat = (VectorRepeat){0};
    result.group_id = result.clip_mask = 0;
    float d[VECTOR_REPEAT_PROPERTIES];
    for (int i = 0; i < VECTOR_REPEAT_PROPERTIES; i++) {
        d[i] = x * r->column.value[i] + y * r->row.value[i];
    }
    result.rect.x = clamp(source->rect.x + x * (source->rect.w + r->gap_x),
                          -VECTOR_MAX_ARTBOARD_SIZE, VECTOR_MAX_ARTBOARD_SIZE);
    result.rect.y = clamp(source->rect.y + y * (source->rect.h + r->gap_y),
                          -VECTOR_MAX_ARTBOARD_SIZE, VECTOR_MAX_ARTBOARD_SIZE);
    result.rect.w = clamp(source->rect.w + d[VECTOR_REPEAT_WIDTH], 1, VECTOR_MAX_ARTBOARD_SIZE);
    result.rect.h = clamp(source->rect.h + d[VECTOR_REPEAT_HEIGHT], 1, VECTOR_MAX_ARTBOARD_SIZE);
    result.angle = vector_angle_turn(source->angle + d[VECTOR_REPEAT_ANGLE]);
    result.stroke.width =
        clamp(source->stroke.width + d[VECTOR_REPEAT_STROKE_WIDTH], 0, VECTOR_MAX_ARTBOARD_SIZE);
    // Preserve exact source colors for the original and for zero color deltas.
    if (d[4] || d[5] || d[6] || d[7]) {
        result.fill = color(source->fill, d + VECTOR_REPEAT_FILL_HUE);
    }
    if (d[8] || d[9] || d[10] || d[11]) {
        result.stroke.color = color(source->stroke.color, d + VECTOR_REPEAT_STROKE_HUE);
    }
    *out = result;
    return true;
}

VectorShape *vector_repeat_source(VectorDocument *d) {
    if (!d || !d->active_rectangle || d->selection.count > 1) {
        return NULL;
    }
    VectorShape *s = vector_rectangle_find(d, d->active_rectangle);
    return s && !s->group_id && !s->clip_mask &&
                   !(vector_rectangle_effective_flags(d, s) & VECTOR_OBJECT_STATE_MASK)
               ? s
               : NULL;
}

bool vector_repeat_apply(VectorDocument *d, const VectorRepeat *r, bool linked) {
    VectorShape *source = vector_repeat_source(d);
    if (!source || !r || !r->enabled || !vector_repeat_valid(r)) {
        return false;
    }
    unsigned count = r->columns * r->rows;
    unsigned total = count;
    for (int i = 0; i < d->rectangle_count; i++) {
        const VectorShape *s = &d->rectangles[i];
        if (s->id != source->id) {
            total += s->repeat.enabled ? s->repeat.columns * s->repeat.rows : 1;
        }
    }
    if (total > VECTOR_REPEAT_DOCUMENT_LIMIT) {
        return false;
    }
    if (!linked && count > (unsigned)(VECTOR_MAX_RECTANGLES - d->rectangle_count + 1)) {
        return false;
    }
    VectorShape *shapes = calloc(linked ? 1 : count, sizeof *shapes);
    if (!shapes) {
        return false;
    }
    bool ok = true;
    for (unsigned y = 0; y < r->rows && ok; y++) {
        for (unsigned x = 0; x < r->columns && ok; x++) {
            unsigned at = linked ? 0 : y * r->columns + x;
            ok = vector_repeat_instance(source, r, x, y, &shapes[at]) &&
                 vector_rectangle_valid(shapes[at]);
            if (!linked && (x || y)) {
                shapes[at].id = 0;
            }
        }
    }
    if (ok && linked) {
        shapes[0] = *source;
        shapes[0].repeat = *r;
        ok = vector_shape_update(d, &shapes[0]);
    } else if (ok) {
        ok = vector_objects_replace_selection_block(d, shapes, (int)count);
    }
    free(shapes);
    return ok;
}

bool vector_repeat_detach(VectorDocument *d) {
    VectorShape *s = vector_repeat_source(d);
    if (!s || !s->repeat.enabled) {
        return false;
    }
    VectorRepeat r = s->repeat;
    return vector_repeat_apply(d, &r, false);
}

bool vector_repeat_remove(VectorDocument *d) {
    VectorShape *s = vector_repeat_source(d);
    if (!s || !s->repeat.enabled) {
        return false;
    }
    VectorShape after = *s;
    after.repeat = (VectorRepeat){0};
    return vector_shape_update(d, &after);
}
