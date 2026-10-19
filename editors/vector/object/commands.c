/** @file @brief Atomic shape insertion and value replacement, shared by curves and text. */
#include "commands.h"
#include "editors/vector/document/selection.h"
#include <limits.h>
#include <stdlib.h>
#include <string.h>

typedef struct {
    VectorShape before, after;
    VectorRectangleSelection selection;
    unsigned board, rectangle;
    int index;
    unsigned adding;
} ShapeEdit;

static bool apply(void *state, const void *data) {
    VectorDocument *d = state;
    const ShapeEdit *e = data;
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
    const ShapeEdit *e = data;
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

static bool execute(VectorDocument *d, ShapeEdit *edit) {
    ShapeEdit *owned = malloc(sizeof *owned);
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

bool vector_shape_add(VectorDocument *d, const VectorShape *shape) {
    if (!shape || shape->id) {
        return false;
    }
    VectorArtboard *board = vector_artboard_find(d, shape->parent_id);
    if (!board || board->flags & VECTOR_OBJECT_STATE_MASK || !d->next_rectangle_id ||
        d->next_rectangle_id == UINT_MAX || d->rectangle_count == VECTOR_MAX_RECTANGLES) {
        return false;
    }
    ShapeEdit edit = {.adding = true,
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

bool vector_shape_update(VectorDocument *d, const VectorShape *after) {
    VectorShape *before = after ? vector_rectangle_find(d, after->id) : NULL;
    if (!before || after->parent_id != before->parent_id || !vector_rectangle_valid(*after) ||
        vector_rectangle_effective_flags(d, before) & VECTOR_OBJECT_STATE_MASK) {
        return false;
    }
    if (!memcmp(before, after, sizeof *after)) {
        return true;
    }
    ShapeEdit edit = {.before = *before, .after = *after};
    return execute(d, &edit);
}

bool vector_shape_command_write(const VectorCommand *command, FILE *file) {
    if (command->apply != apply || command->revert != revert) {
        return false;
    }
    unsigned kind = 8;
    return fwrite(&kind, sizeof kind, 1, file) == 1 &&
           fwrite(command->data, sizeof(ShapeEdit), 1, file) == 1;
}

bool vector_shape_command_read(VectorCommand *command, FILE *file) {
    ShapeEdit *e = calloc(1, sizeof *e);
    if (!e) {
        return false;
    }
    if (fread(e, sizeof *e, 1, file) != 1 || e->adding > 1 || !vector_rectangle_valid(e->after) ||
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

bool vector_shape_update_typing(VectorDocument *d, const VectorShape *after, int start) {
    if (d->history.cursor == start + 1 && d->history.cursor == d->history.count) {
        VectorCommand *command = &d->history.commands[d->history.cursor - 1];
        ShapeEdit *e = command->data;
        VectorShape *current = vector_rectangle_find(d, after->id);
        if (command->apply == apply && !e->adding && current && e->after.id == after->id &&
            !memcmp(current, &e->after, sizeof *current) && vector_rectangle_valid(*after) &&
            !(vector_rectangle_effective_flags(d, current) & VECTOR_OBJECT_STATE_MASK)) {
            e->after = *after;
            *current = *after;
            return true;
        }
    }
    return vector_shape_update(d, after);
}
