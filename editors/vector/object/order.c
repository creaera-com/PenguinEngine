/** @file @brief Stable sibling permutations with reversible ID-based ordering commands. */
#include "order.h"
#include <stdlib.h>
#include <string.h>

_Static_assert(VECTOR_MAX_ARTBOARDS <= VECTOR_MAX_RECTANGLES, "sibling buffers must fit artboards");

typedef struct {
    unsigned boards[VECTOR_MAX_ARTBOARDS], rectangles[VECTOR_MAX_RECTANGLES];
} OrderIds;

typedef struct {
    int board_count, rectangle_count;
    OrderIds before, after;
} OrderEdit;

static bool permutation(const unsigned *before, const unsigned *after, int count) {
    for (int i = 0; i < count; i++) {
        bool found = false;
        if (!before[i] || !after[i]) {
            return false;
        }
        for (int j = 0; j < count; j++) {
            found |= before[j] == after[i];
            if (j < i && (before[j] == before[i] || after[j] == after[i])) {
                return false;
            }
        }
        if (!found) {
            return false;
        }
    }
    return true;
}

static bool valid(const OrderEdit *edit) {
    return edit->board_count >= 0 && edit->board_count <= VECTOR_MAX_ARTBOARDS &&
           edit->rectangle_count >= 0 && edit->rectangle_count <= VECTOR_MAX_RECTANGLES &&
           permutation(edit->before.boards, edit->after.boards, edit->board_count) &&
           permutation(edit->before.rectangles, edit->after.rectangles, edit->rectangle_count);
}

static bool replace(VectorDocument *d, const OrderEdit *edit, bool undo) {
    const OrderIds *before = undo ? &edit->after : &edit->before;
    const OrderIds *after = undo ? &edit->before : &edit->after;
    if (d->artboard_count != edit->board_count || d->rectangle_count != edit->rectangle_count) {
        return false;
    }
    for (int i = 0; i < d->artboard_count; i++) {
        if (d->artboards[i].id != before->boards[i]) {
            return false;
        }
    }
    for (int i = 0; i < d->rectangle_count; i++) {
        if (d->rectangles[i].id != before->rectangles[i]) {
            return false;
        }
    }
    VectorArtboard boards[VECTOR_MAX_ARTBOARDS];
    VectorRectangle rectangles[VECTOR_MAX_RECTANGLES];
    for (int i = 0; i < d->artboard_count; i++) {
        VectorArtboard *board = vector_artboard_find(d, after->boards[i]);
        if (!board) {
            return false;
        }
        boards[i] = *board;
    }
    for (int i = 0; i < d->rectangle_count; i++) {
        VectorRectangle *rectangle = vector_rectangle_find(d, after->rectangles[i]);
        if (!rectangle) {
            return false;
        }
        // A rectangle can only exchange slots with siblings, never another parent's children.
        if (rectangle->parent_id != d->rectangles[i].parent_id) {
            return false;
        }
        rectangles[i] = *rectangle;
    }
    memcpy(d->artboards, boards, d->artboard_count * sizeof *boards);
    memcpy(d->rectangles, rectangles, d->rectangle_count * sizeof *rectangles);
    return true;
}

static bool apply(void *state, const void *data) {
    return replace(state, data, false);
}

static bool revert(void *state, const void *data) {
    return replace(state, data, true);
}

static void swap(unsigned *ids, bool *selected, int first, int second) {
    unsigned id = ids[first];
    bool selection = selected[first];
    ids[first] = ids[second];
    selected[first] = selected[second];
    ids[second] = id;
    selected[second] = selection;
}

static void reorder(unsigned *ids, bool *selected, int count, VectorOrder order) {
    if (order == VECTOR_ORDER_FORWARD) {
        for (int i = count - 2; i >= 0; i--) {
            if (selected[i] && !selected[i + 1]) {
                swap(ids, selected, i, i + 1);
            }
        }
    } else if (order == VECTOR_ORDER_BACKWARD) {
        for (int i = 1; i < count; i++) {
            if (selected[i] && !selected[i - 1]) {
                swap(ids, selected, i, i - 1);
            }
        }
    } else {
        unsigned result[VECTOR_MAX_RECTANGLES];
        int cursor = 0;
        // Stable partition: internal order of both selected and unselected objects is retained.
        for (int pass = 0; pass < 2; pass++) {
            bool select = order == VECTOR_ORDER_FRONT ? pass != 0 : pass == 0;
            for (int i = 0; i < count; i++) {
                if (selected[i] == select) {
                    result[cursor++] = ids[i];
                }
            }
        }
        memcpy(ids, result, count * sizeof *ids);
    }
}

static bool prepare(VectorDocument *d, VectorOrder order, OrderEdit *edit) {
    if (order < VECTOR_ORDER_FRONT || order > VECTOR_ORDER_BACKWARD ||
        d->history.cursor >= VECTOR_HISTORY_CAPACITY) {
        return false;
    }
    *edit = (OrderEdit){.board_count = d->artboard_count, .rectangle_count = d->rectangle_count};
    for (int i = 0; i < d->artboard_count; i++) {
        edit->before.boards[i] = d->artboards[i].id;
    }
    for (int i = 0; i < d->rectangle_count; i++) {
        edit->before.rectangles[i] = d->rectangles[i].id;
        if (vector_document_is_selected(d, d->rectangles[i].id) &&
            (vector_rectangle_effective_flags(d, &d->rectangles[i]) & VECTOR_OBJECT_LOCKED)) {
            return false;
        }
    }
    edit->after = edit->before;
    bool selected[VECTOR_MAX_RECTANGLES] = {0};
    if (!d->active_rectangle) {
        VectorArtboard *board = vector_artboard_find(d, d->active_artboard);
        if (!board || board->flags & VECTOR_OBJECT_LOCKED) {
            return false;
        }
        for (int i = 0; i < d->artboard_count; i++) {
            selected[i] = d->artboards[i].id == d->active_artboard;
        }
        reorder(edit->after.boards, selected, d->artboard_count, order);
    } else {
        for (int parent = 0; parent < d->artboard_count; parent++) {
            int slots[VECTOR_MAX_RECTANGLES], count = 0;
            unsigned ids[VECTOR_MAX_RECTANGLES];
            for (int i = 0; i < d->rectangle_count; i++) {
                if (d->rectangles[i].parent_id == d->artboards[parent].id) {
                    slots[count] = i;
                    ids[count] = d->rectangles[i].id;
                    selected[count++] = vector_document_is_selected(d, d->rectangles[i].id);
                }
            }
            reorder(ids, selected, count, order);
            for (int i = 0; i < count; i++) {
                edit->after.rectangles[slots[i]] = ids[i];
            }
        }
    }
    return memcmp(&edit->before, &edit->after, sizeof edit->before) != 0;
}

bool vector_objects_can_order(VectorDocument *d, VectorOrder order) {
    OrderEdit edit;
    return prepare(d, order, &edit);
}

bool vector_objects_order(VectorDocument *d, VectorOrder order) {
    OrderEdit *edit = malloc(sizeof *edit);
    if (!edit) {
        return false;
    }
    if (prepare(d, order, edit) &&
        vector_history_execute(&d->history, d, (VectorCommand){apply, revert, edit})) {
        return true;
    }
    free(edit);
    return false;
}

bool vector_order_command_write(const VectorCommand *command, FILE *file) {
    if (command->apply != apply || command->revert != revert || !command->data) {
        return false;
    }
    unsigned kind = 7;
    return fwrite(&kind, sizeof kind, 1, file) == 1 &&
           fwrite(command->data, sizeof(OrderEdit), 1, file) == 1;
}

bool vector_order_command_read(VectorCommand *command, FILE *file) {
    OrderEdit *edit = malloc(sizeof *edit);
    if (!edit) {
        return false;
    }
    if (fread(edit, sizeof *edit, 1, file) != 1 || !valid(edit)) {
        free(edit);
        return false;
    }
    *command = (VectorCommand){apply, revert, edit};
    return true;
}
