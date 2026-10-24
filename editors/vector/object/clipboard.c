/** @file @brief Object hierarchy transfer and reversible structural snapshots. */
#include "clipboard.h"
#include "editors/vector/instance/instance.h"
#include <limits.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct {
    VectorClipboard objects;
    VectorRectangleSelection selection;
    unsigned board, rectangle;
} ObjectState;

typedef struct {
    ObjectState before, after;
} ObjectEdit;

static bool board_valid(VectorArtboard b) {
    return b.id && !(b.flags & ~VECTOR_OBJECT_STATE_MASK) && memchr(b.name, 0, sizeof b.name) &&
           *b.name && isfinite(b.rect.x) && isfinite(b.rect.y) && isfinite(b.rect.w) &&
           isfinite(b.rect.h) && b.rect.w >= 1 && b.rect.h >= 1 &&
           b.rect.w <= VECTOR_MAX_ARTBOARD_SIZE && b.rect.h <= VECTOR_MAX_ARTBOARD_SIZE;
}

bool vector_clipboard_valid(const VectorClipboard *c) {
    if (!c || c->whole_board > 1 || c->board_count < 0 || c->board_count > VECTOR_MAX_ARTBOARDS ||
        c->rectangle_count < 0 || c->rectangle_count > VECTOR_MAX_RECTANGLES) {
        return false;
    }
    for (int i = 0; i < c->board_count; i++) {
        if (!board_valid(c->boards[i])) {
            return false;
        }
        for (int j = 0; j < i; j++) {
            if (c->boards[j].id == c->boards[i].id) {
                return false;
            }
        }
    }
    if (!vector_instances_valid(c->rectangles, c->rectangle_count)) {
        return false;
    }
    unsigned instances = 0;
    for (int i = 0; i < c->rectangle_count; i++) {
        const VectorRepeat *repeat = &c->rectangles[i].repeat;
        instances += repeat->enabled ? repeat->columns * repeat->rows : 1;
        if (instances > VECTOR_REPEAT_DOCUMENT_LIMIT) {
            return false;
        }
        if (!vector_rectangle_valid(c->rectangles[i])) {
            return false;
        }
        bool parent = false;
        for (int j = 0; j < c->board_count; j++) {
            parent |= c->boards[j].id == c->rectangles[i].parent_id;
        }
        if (!parent) {
            return false;
        }
        for (int j = 0; j < i; j++) {
            if ((c->rectangles[i].group_id &&
                 c->rectangles[j].group_id == c->rectangles[i].group_id &&
                 c->rectangles[j].parent_id != c->rectangles[i].parent_id) ||
                c->rectangles[j].id == c->rectangles[i].id) {
                return false;
            }
        }
    }
    for (int i = 0; i < c->rectangle_count; i++) {
        const VectorShape *mask = &c->rectangles[i];
        if (!mask->clip_mask) {
            continue;
        }
        int members = 0;
        for (int j = 0; j < c->rectangle_count; j++) {
            const VectorShape *s = &c->rectangles[j];
            if (s->group_id == mask->group_id) {
                if (j != i && s->clip_mask) {
                    return false;
                }
                members++;
            }
        }
        if (members < 2) {
            return false;
        }
    }
    return true;
}

static ObjectState capture(const VectorDocument *d) {
    ObjectState state = {
        .selection = d->selection, .board = d->active_artboard, .rectangle = d->active_rectangle};
    state.objects.board_count = d->artboard_count;
    state.objects.rectangle_count = d->rectangle_count;
    memcpy(state.objects.boards, d->artboards, d->artboard_count * sizeof *d->artboards);
    memcpy(state.objects.rectangles, d->rectangles, d->rectangle_count * sizeof *d->rectangles);
    return state;
}

static bool replace(VectorDocument *d, const ObjectState *before, const ObjectState *after) {
    const VectorClipboard *a = &before->objects, *b = &after->objects;
    if (d->artboard_count != a->board_count || d->rectangle_count != a->rectangle_count ||
        memcmp(d->artboards, a->boards, a->board_count * sizeof *a->boards) ||
        memcmp(d->rectangles, a->rectangles, a->rectangle_count * sizeof *a->rectangles)) {
        return false;
    }
    d->artboard_count = b->board_count;
    d->rectangle_count = b->rectangle_count;
    memcpy(d->artboards, b->boards, b->board_count * sizeof *b->boards);
    memcpy(d->rectangles, b->rectangles, b->rectangle_count * sizeof *b->rectangles);
    d->active_artboard = after->board;
    d->active_rectangle = after->rectangle;
    d->selection = after->selection;
    return true;
}

static bool apply(void *state, const void *data) {
    const ObjectEdit *edit = data;
    return replace(state, &edit->before, &edit->after);
}

static bool revert(void *state, const void *data) {
    const ObjectEdit *edit = data;
    return replace(state, &edit->after, &edit->before);
}

static bool execute(VectorDocument *d, ObjectEdit *edit) {
    if (vector_clipboard_valid(&edit->after.objects) &&
        vector_history_execute(&d->history, d, (VectorCommand){apply, revert, edit})) {
        return true;
    }
    free(edit);
    return false;
}

bool vector_objects_initialize(VectorDocument *d, const VectorClipboard *objects) {
    if (d->artboard_count || d->rectangle_count || d->history.count ||
        !vector_clipboard_valid(objects)) {
        return false;
    }
    ObjectEdit *edit = calloc(1, sizeof *edit);
    if (!edit) {
        return false;
    }
    edit->after.objects = *objects;
    if (objects->board_count) {
        edit->after.board = objects->boards[0].id;
    }
    if (!execute(d, edit)) {
        return false;
    }
    d->history.floor = d->history.cursor;
    for (int i = 0; i < d->artboard_count; i++) {
        if (d->artboards[i].id >= d->next_artboard_id) {
            d->next_artboard_id = d->artboards[i].id + 1;
        }
    }
    for (int i = 0; i < d->rectangle_count; i++) {
        if (d->rectangles[i].id >= d->next_rectangle_id) {
            d->next_rectangle_id = d->rectangles[i].id + 1;
        }
    }
    return d->next_artboard_id && d->next_rectangle_id;
}

bool vector_objects_copy(VectorDocument *d, VectorClipboard *clipboard) {
    VectorClipboard copy = {0};
    bool rectangles = d->active_rectangle != 0;
    for (int i = 0; i < d->artboard_count; i++) {
        VectorArtboard board = d->artboards[i];
        bool used = !rectangles && board.id == d->active_artboard;
        for (int j = 0; rectangles && j < d->rectangle_count; j++) {
            used |= d->rectangles[j].parent_id == board.id &&
                    vector_document_is_selected(d, d->rectangles[j].id);
        }
        if (used) {
            copy.boards[copy.board_count++] = board;
        }
    }
    if (!copy.board_count) {
        return false;
    }
    copy.whole_board = !rectangles;
    for (int i = 0; i < d->rectangle_count; i++) {
        VectorRectangle r = d->rectangles[i];
        if ((rectangles && vector_document_is_selected(d, r.id)) ||
            (!rectangles && r.parent_id == d->active_artboard)) {
            if (r.instance_source) {
                VectorShape resolved;
                vector_instance_resolve(d, &r, &resolved);
                VectorShape *source = vector_rectangle_find(d, r.instance_source);
                resolved.instance_source = r.instance_source;
                resolved.instance_basis_w = source ? source->rect.w : r.instance_basis_w;
                resolved.instance_basis_h = source ? source->rect.h : r.instance_basis_h;
                r = resolved;
            }
            copy.rectangles[copy.rectangle_count++] = r;
        }
    }
    for (int i = 0; i < copy.rectangle_count; i++) {
        if (copy.rectangles[i].clip_mask) {
            bool content = false;
            for (int j = 0; j < copy.rectangle_count; j++) {
                content |= j != i && copy.rectangles[j].group_id == copy.rectangles[i].group_id;
            }
            if (!content) {
                copy.rectangles[i].clip_mask = 0;
                copy.rectangles[i].group_id = 0;
            }
        }
    }
    if (!vector_clipboard_valid(&copy)) {
        return false;
    }
    *clipboard = copy;
    return true;
}

bool vector_objects_can_delete(VectorDocument *d) {
    if (d->history.cursor >= VECTOR_HISTORY_CAPACITY) {
        return false;
    }
    VectorClipboard selected;
    if (!vector_objects_copy(d, &selected)) {
        return false;
    }
    for (int i = 0; i < selected.board_count; i++) {
        if (selected.boards[i].flags & VECTOR_OBJECT_LOCKED) {
            return false;
        }
    }
    for (int i = 0; i < selected.rectangle_count; i++) {
        if (selected.rectangles[i].flags & VECTOR_OBJECT_LOCKED) {
            return false;
        }
    }
    return true;
}

bool vector_objects_delete(VectorDocument *d) {
    if (!vector_objects_can_delete(d)) {
        return false;
    }
    bool whole_board = !d->active_rectangle;
    ObjectEdit *edit = calloc(1, sizeof *edit);
    if (!edit) {
        return false;
    }
    edit->before = capture(d);
    edit->after = edit->before;
    VectorClipboard *objects = &edit->after.objects;
    int count = 0;
    for (int i = 0; i < objects->rectangle_count; i++) {
        VectorRectangle r = objects->rectangles[i];
        if (!(whole_board ? r.parent_id == d->active_artboard
                          : vector_document_is_selected(d, r.id))) {
            objects->rectangles[count++] = r;
        }
    }
    objects->rectangle_count = count;
    for (int i = 0; i < count; i++) {
        VectorShape *s = &objects->rectangles[i];
        bool source = false;
        for (int j = 0; j < count; j++) {
            source |= objects->rectangles[j].id == s->instance_source;
        }
        if (s->instance_source && !source) {
            VectorShape resolved;
            vector_instance_resolve(d, s, &resolved);
            *s = resolved;
        }
    }
    if (whole_board) {
        count = 0;
        for (int i = 0; i < objects->board_count; i++) {
            if (objects->boards[i].id != d->active_artboard) {
                objects->boards[count++] = objects->boards[i];
            }
        }
        objects->board_count = count;
        edit->after.board = 0;
    }
    edit->after.rectangle = 0;
    edit->after.selection = (VectorRectangleSelection){0};
    return execute(d, edit);
}

static bool replace_selection(VectorDocument *d, const VectorShape *shapes, int count, bool block) {
    if (!d->active_rectangle || count < 0 || count > VECTOR_MAX_RECTANGLES || (count && !shapes) ||
        !vector_objects_can_delete(d)) {
        return false;
    }
    ObjectEdit *edit = calloc(1, sizeof *edit);
    if (!edit) {
        return false;
    }
    edit->before = capture(d);
    edit->after = edit->before;
    VectorClipboard *objects = &edit->after.objects;
    VectorShape result[VECTOR_MAX_RECTANGLES];
    unsigned next = d->next_rectangle_id;
    for (int i = 0; i < count; i++) {
        result[i] = shapes[i];
        VectorShape *before = result[i].id ? vector_rectangle_find(d, result[i].id) : NULL;
        if (before && !vector_instance_edit_valid(before, &result[i])) {
            free(edit);
            return false;
        }
        if (!result[i].id) {
            if (!next || next == UINT_MAX) {
                free(edit);
                return false;
            }
            result[i].id = next++;
            snprintf(result[i].name, sizeof result[i].name, "Pfad %u", result[i].id);
        } else if (!vector_document_is_selected(d, result[i].id)) {
            free(edit);
            return false;
        }
    }
    int selected = 0, last = -1;
    for (int i = 0; i < d->rectangle_count; i++) {
        if (vector_document_is_selected(d, d->rectangles[i].id)) {
            selected++;
            last = i;
        }
    }
    if (d->rectangle_count - selected + count > VECTOR_MAX_RECTANGLES) {
        free(edit);
        return false;
    }
    bool in_place = !block && count == selected;
    for (int i = 0; i < count; i++) {
        in_place &= shapes[i].id != 0;
    }
    if (in_place) {
        for (int i = 0; i < objects->rectangle_count; i++) {
            for (int j = 0; j < count; j++) {
                if (objects->rectangles[i].id == result[j].id) {
                    objects->rectangles[i] = result[j];
                }
            }
        }
    } else {
        objects->rectangle_count = 0;
        for (int i = 0; i < d->rectangle_count; i++) {
            if (!vector_document_is_selected(d, d->rectangles[i].id)) {
                objects->rectangles[objects->rectangle_count++] = d->rectangles[i];
            }
            if (i == last) {
                for (int j = 0; j < count; j++) {
                    objects->rectangles[objects->rectangle_count++] = result[j];
                }
            }
        }
    }
    if (!in_place) {
        edit->after.selection = (VectorRectangleSelection){0};
        edit->after.rectangle = 0;
        for (int i = 0; i < count; i++) {
            edit->after.selection.ids[edit->after.selection.count++] = result[i].id;
            edit->after.rectangle = result[i].id;
            edit->after.board = result[i].parent_id;
        }
    }
    if (!execute(d, edit)) {
        return false;
    }
    d->next_rectangle_id = next;
    return true;
}

bool vector_objects_replace_selection(VectorDocument *d, const VectorShape *shapes, int count) {
    return replace_selection(d, shapes, count, false);
}

bool vector_objects_replace_selection_block(VectorDocument *d, const VectorShape *shapes,
                                            int count) {
    return replace_selection(d, shapes, count, true);
}

bool vector_objects_cut(VectorDocument *d, VectorClipboard *clipboard) {
    VectorClipboard copy;
    if (!vector_objects_copy(d, &copy) || !vector_objects_delete(d)) {
        return false;
    }
    *clipboard = copy;
    return true;
}

static bool paste(VectorDocument *d, const VectorClipboard *c, bool duplicate, bool preview,
                  bool linked) {
    if (d->history.cursor >= VECTOR_HISTORY_CAPACITY || !vector_clipboard_valid(c) ||
        !c->board_count || d->rectangle_count + c->rectangle_count > VECTOR_MAX_RECTANGLES ||
        !d->next_rectangle_id || d->next_rectangle_id > UINT_MAX - c->rectangle_count ||
        !d->next_artboard_id || d->next_artboard_id > UINT_MAX - c->board_count) {
        return false;
    }
    VectorArtboard *target = vector_artboard_find(d, d->active_artboard);
    bool new_boards = c->whole_board || (!target && !duplicate);
    if ((!new_boards && target && (target->flags & VECTOR_OBJECT_STATE_MASK)) ||
        (new_boards && d->artboard_count + c->board_count > VECTOR_MAX_ARTBOARDS)) {
        return false;
    }
    ObjectEdit *edit = calloc(1, sizeof *edit);
    if (!edit) {
        return false;
    }
    edit->before = capture(d);
    edit->after = edit->before;
    edit->after.selection = (VectorRectangleSelection){0};
    edit->after.rectangle = 0;
    VectorClipboard *objects = &edit->after.objects;
    unsigned parent_ids[VECTOR_MAX_ARTBOARDS];
    unsigned next_board = d->next_artboard_id, next_rectangle = d->next_rectangle_id;
    for (int i = 0; i < c->board_count; i++) {
        VectorArtboard board = c->boards[i];
        if (new_boards) {
            board.id = next_board++;
            board.rect.x += 16;
            board.rect.y += 16;
            objects->boards[objects->board_count++] = board;
        } else if (!duplicate) {
            board.id = target->id;
        }
        parent_ids[i] = board.id;
        edit->after.board = board.id;
    }
    for (int i = 0; i < c->rectangle_count; i++) {
        VectorRectangle r = c->rectangles[i];
        int parent = 0;
        while (parent < c->board_count && c->boards[parent].id != r.parent_id) {
            parent++;
        }
        if (linked) {
            VectorShape resolved;
            vector_instance_resolve(d, &r, &resolved);
            unsigned source = r.instance_source ? r.instance_source : r.id;
            VectorShape *original = vector_rectangle_find(d, source);
            if (!original) {
                free(edit);
                return false;
            }
            r = resolved;
            r.instance_source = source;
            r.instance_basis_w = original->rect.w;
            r.instance_basis_h = original->rect.h;
        }
        if (r.group_id) {
            for (int j = 0; j <= i; j++) {
                if (c->rectangles[j].group_id == r.group_id) {
                    r.group_id = d->next_rectangle_id + (unsigned)j;
                    break;
                }
            }
        }
        if (r.instance_source && !linked) {
            unsigned remapped = 0;
            for (int j = 0; j < c->rectangle_count; j++) {
                if (c->rectangles[j].id == r.instance_source) {
                    remapped = d->next_rectangle_id + (unsigned)j;
                    break;
                }
            }
            if (remapped) {
                r.instance_source = remapped;
            } else if (!duplicate) {
                r.instance_source = 0;
                r.instance_basis_w = r.instance_basis_h = 0;
            }
        }
        r.id = next_rectangle++;
        if (linked) {
            snprintf(r.name, sizeof r.name, "Instanz %u", r.id);
        }
        r.parent_id = parent_ids[parent];
        if (!new_boards) {
            r.rect.x += 16 + (duplicate ? 0 : c->boards[parent].rect.x - c->boards[0].rect.x);
            r.rect.y += 16 + (duplicate ? 0 : c->boards[parent].rect.y - c->boards[0].rect.y);
        }
        objects->rectangles[objects->rectangle_count++] = r;
        if (!c->whole_board) {
            edit->after.selection.ids[edit->after.selection.count++] = r.id;
            edit->after.rectangle = r.id;
            edit->after.board = r.parent_id;
        }
    }
    if (preview) {
        bool valid = vector_clipboard_valid(&edit->after.objects);
        free(edit);
        return valid;
    }
    if (!execute(d, edit)) {
        return false;
    }
    // IDs are never reused after Undo; command replay restores values without rewinding counters.
    d->next_artboard_id = next_board;
    d->next_rectangle_id = next_rectangle;
    return true;
}

bool vector_objects_can_paste(VectorDocument *d, const VectorClipboard *clipboard) {
    return paste(d, clipboard, false, true, false);
}

bool vector_objects_can_duplicate(VectorDocument *d) {
    VectorClipboard copy;
    return vector_objects_copy(d, &copy) && paste(d, &copy, true, true, false);
}

bool vector_objects_paste(VectorDocument *d, const VectorClipboard *clipboard) {
    return paste(d, clipboard, false, false, false);
}

bool vector_objects_duplicate(VectorDocument *d) {
    VectorClipboard copy;
    return vector_objects_copy(d, &copy) && paste(d, &copy, true, false, false);
}

bool vector_objects_duplicate_values(VectorDocument *d, const VectorClipboard *values) {
    return paste(d, values, true, false, true);
}

bool vector_objects_command_write(const VectorCommand *command, FILE *file) {
    if (command->apply != apply || command->revert != revert || !command->data) {
        return false;
    }
    unsigned kind = 6;
    return fwrite(&kind, sizeof kind, 1, file) == 1 &&
           fwrite(command->data, sizeof(ObjectEdit), 1, file) == 1;
}

static bool state_valid(const ObjectState *s) {
    if (!vector_clipboard_valid(&s->objects) || s->selection.count < 0 ||
        s->selection.count > VECTOR_MAX_RECTANGLES) {
        return false;
    }
    bool board = !s->board, rectangle = !s->rectangle;
    for (int i = 0; i < s->objects.board_count; i++) {
        board |= s->board == s->objects.boards[i].id;
    }
    for (int i = 0; i < s->objects.rectangle_count; i++) {
        const VectorRectangle *r = &s->objects.rectangles[i];
        rectangle |= r->id == s->rectangle && r->parent_id == s->board;
    }
    for (int i = 0; i < s->selection.count; i++) {
        bool found = false;
        for (int j = 0; j < s->objects.rectangle_count; j++) {
            found |= s->selection.ids[i] == s->objects.rectangles[j].id;
        }
        if (!found) {
            return false;
        }
    }
    return board && rectangle;
}

bool vector_objects_command_read(VectorCommand *command, FILE *file) {
    ObjectEdit *edit = calloc(1, sizeof *edit);
    if (!edit) {
        return false;
    }
    if (fread(edit, sizeof *edit, 1, file) != 1 || !state_valid(&edit->before) ||
        !state_valid(&edit->after)) {
        free(edit);
        return false;
    }
    *command = (VectorCommand){apply, revert, edit};
    return true;
}
