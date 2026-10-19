/** @file @brief Editable curves retain their cubic controls; polygons are derived fill geometry. */
#include "curve.h"
#include "editors/vector/document/selection.h"
#include "editors/vector/geometry/rotation.h"
#include <limits.h>
#include <math.h>
#include <stdlib.h>
#include <string.h>

typedef struct {
    VectorShape before, after;
    VectorRectangleSelection selection;
    unsigned board, rectangle;
    int index;
    unsigned adding;
} CurveEdit;

bool vector_curve_shape(VectorShape *s, const VectorBezier *curve) {
    if (!s || !vector_bezier_valid(curve) || curve->count < 2) {
        return false;
    }
    PUIRect bounds = vector_bezier_bounds(curve);
    if (bounds.w < .000001f && bounds.h < .000001f) {
        return false;
    }
    bounds.w = fmaxf(1, bounds.w);
    bounds.h = fmaxf(1, bounds.h);
    if (bounds.w > VECTOR_MAX_ARTBOARD_SIZE || bounds.h > VECTOR_MAX_ARTBOARD_SIZE) {
        return false;
    }
    VectorShape result = *s;
    result.rect = bounds;
    result.angle = 0;
    result.curve = *curve;
    result.curve_kind = VECTOR_CURVE_FREE;
    result.path = (VectorPath){0};
    for (unsigned i = 0; i < curve->count; i++) {
        VectorBezierNode *n = &result.curve.nodes[i];
        VectorPoint *points[] = {&n->point, &n->incoming, &n->outgoing};
        for (int j = 0; j < 3; j++) {
            points[j]->x = (points[j]->x - bounds.x) / bounds.w;
            points[j]->y = (points[j]->y - bounds.y) / bounds.h;
        }
    }
    if (curve->closed && !vector_bezier_flatten(&result.curve, .001f, &result.path)) {
        return false;
    }
    if (result.path.count && vector_path_contour_area(&result.path, 0) < 0) {
        for (unsigned i = 0; i < result.path.count / 2; i++) {
            VectorPoint swap = result.path.points[i];
            result.path.points[i] = result.path.points[result.path.count - 1 - i];
            result.path.points[result.path.count - 1 - i] = swap;
        }
    }
    *s = result;
    return true;
}

VectorBezier vector_curve_parent(const VectorShape *s) {
    VectorBezier curve = s->curve;
    for (unsigned i = 0; i < curve.count; i++) {
        VectorBezierNode *n = &curve.nodes[i];
        VectorPoint *points[] = {&n->point, &n->incoming, &n->outgoing};
        for (int j = 0; j < 3; j++) {
            points[j]->x = s->rect.x + points[j]->x * s->rect.w;
            points[j]->y = s->rect.y + points[j]->y * s->rect.h;
            vector_rotate_point(&points[j]->x, &points[j]->y, s->rect.x + s->rect.w / 2,
                                s->rect.y + s->rect.h / 2, s->angle);
        }
    }
    return curve;
}

static bool apply(void *state, const void *data) {
    VectorDocument *d = state;
    const CurveEdit *e = data;
    if (e->adding) {
        if (d->rectangle_count != e->index || d->rectangle_count == VECTOR_MAX_RECTANGLES) {
            return false;
        }
        d->rectangles[d->rectangle_count++] = e->after;
        vector_document_select(d, e->after.id);
        return true;
    }
    VectorShape *s = vector_rectangle_find(d, e->before.id);
    if (!s || memcmp(s, &e->before, sizeof *s)) {
        return false;
    }
    *s = e->after;
    return true;
}

static bool revert(void *state, const void *data) {
    VectorDocument *d = state;
    const CurveEdit *e = data;
    if (e->adding) {
        if (d->rectangle_count != e->index + 1 || d->rectangles[e->index].id != e->after.id) {
            return false;
        }
        d->rectangle_count--;
        d->selection = e->selection;
        d->active_artboard = e->board;
        d->active_rectangle = e->rectangle;
        return true;
    }
    VectorShape *s = vector_rectangle_find(d, e->after.id);
    if (!s || memcmp(s, &e->after, sizeof *s)) {
        return false;
    }
    *s = e->before;
    return true;
}

static bool execute(VectorDocument *d, CurveEdit *edit) {
    CurveEdit *owned = malloc(sizeof *owned);
    if (!owned) {
        return false;
    }
    *owned = *edit;
    if (!vector_history_execute(&d->history, d, (VectorCommand){apply, revert, owned})) {
        free(owned);
        return false;
    }
    return true;
}

bool vector_curve_add_shape(VectorDocument *d, const VectorShape *shape) {
    if (!shape || shape->id || shape->curve.count < 2) {
        return false;
    }
    VectorArtboard *board = vector_artboard_find(d, shape->parent_id);
    if (!board || board->flags & VECTOR_OBJECT_STATE_MASK || !d->next_rectangle_id ||
        d->next_rectangle_id == UINT_MAX || d->rectangle_count == VECTOR_MAX_RECTANGLES) {
        return false;
    }
    CurveEdit edit = {.adding = true,
                      .index = d->rectangle_count,
                      .selection = d->selection,
                      .board = d->active_artboard,
                      .rectangle = d->active_rectangle,
                      .after = *shape};
    edit.after.id = d->next_rectangle_id;
    if (!vector_rectangle_valid(edit.after) || !execute(d, &edit)) {
        return false;
    }
    d->next_rectangle_id++;
    return true;
}

bool vector_curve_add(VectorDocument *d, unsigned parent, const VectorBezier *curve) {
    VectorShape shape = {
        .parent_id = parent, .fill = 0, .stroke = {0x394956ff, 2, VECTOR_STROKE_CENTER}};
    snprintf(shape.name, sizeof shape.name, "Kurve %u", d->next_rectangle_id);
    return vector_curve_shape(&shape, curve) && vector_curve_add_shape(d, &shape);
}

bool vector_curve_update(VectorDocument *d, unsigned id, const VectorBezier *curve) {
    VectorShape *s = vector_rectangle_find(d, id);
    if (!s || !s->curve.count ||
        vector_rectangle_effective_flags(d, s) & VECTOR_OBJECT_STATE_MASK) {
        return false;
    }
    CurveEdit edit = {.before = *s, .after = *s};
    if (!vector_curve_shape(&edit.after, curve) || !vector_rectangle_valid(edit.after) ||
        !memcmp(s, &edit.after, sizeof *s)) {
        return false;
    }
    return execute(d, &edit);
}

bool vector_curve_command_write(const VectorCommand *command, FILE *file) {
    if (command->apply != apply || command->revert != revert) {
        return false;
    }
    unsigned kind = 8;
    return fwrite(&kind, sizeof kind, 1, file) == 1 &&
           fwrite(command->data, sizeof(CurveEdit), 1, file) == 1;
}

bool vector_curve_command_read(VectorCommand *command, FILE *file) {
    CurveEdit *e = calloc(1, sizeof *e);
    if (!e) {
        return false;
    }
    if (fread(e, sizeof *e, 1, file) != 1 || e->adding > 1 || !vector_rectangle_valid(e->after) ||
        e->after.curve.count < 2 ||
        (e->adding ? e->index < 0 || e->index >= VECTOR_MAX_RECTANGLES || e->selection.count < 0 ||
                         e->selection.count > VECTOR_MAX_RECTANGLES
                   : !vector_rectangle_valid(e->before) || e->before.id != e->after.id ||
                         e->before.parent_id != e->after.parent_id)) {
        free(e);
        return false;
    }
    *command = (VectorCommand){apply, revert, e};
    return true;
}
