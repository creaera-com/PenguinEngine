/** @file @brief Undoable artboard creation. */
#include "editors/vector/document/document.h"
#include <math.h>
#include <stdlib.h>
#include <string.h>

typedef struct {
    VectorArtboard artboard;
    int index;
    unsigned previous;
} AddArtboard;

static bool apply(void *state, const void *data) {
    VectorDocument *document = state;
    const AddArtboard *command = data;
    if (document->artboard_count >= VECTOR_MAX_ARTBOARDS ||
        command->index != document->artboard_count) {
        return false;
    }
    document->artboards[document->artboard_count++] = command->artboard;
    document->active_artboard = command->artboard.id;
    return true;
}

static bool revert(void *state, const void *data) {
    VectorDocument *document = state;
    const AddArtboard *command = data;
    if (document->artboard_count != command->index + 1 ||
        document->artboards[command->index].id != command->artboard.id) {
        return false;
    }
    document->artboard_count--;
    document->active_artboard = command->previous;
    return true;
}

bool vector_artboard_add(VectorDocument *document, PUIRect rect) {
    if (!isfinite(rect.x) || !isfinite(rect.y) || !isfinite(rect.w) || !isfinite(rect.h) ||
        rect.w < 1 || rect.h < 1 || rect.w > VECTOR_MAX_ARTBOARD_SIZE ||
        rect.h > VECTOR_MAX_ARTBOARD_SIZE || document->artboard_count >= VECTOR_MAX_ARTBOARDS) {
        return false;
    }
    AddArtboard *command = malloc(sizeof *command);
    if (!command) {
        return false;
    }
    int index = document->artboard_count;
    *command = (AddArtboard){.artboard = {.id = document->next_artboard_id, .rect = rect},
                             .index = index,
                             .previous = document->active_artboard};
    snprintf(command->artboard.name, sizeof command->artboard.name, "Artboard %u",
             command->artboard.id);
    if (!vector_history_execute(&document->history, document,
                                (VectorCommand){apply, revert, command})) {
        free(command);
        return false;
    }
    document->next_artboard_id++;
    return true;
}

static bool valid_command(const AddArtboard *data) {
    PUIRect rect = data->artboard.rect;
    return data->artboard.id > 0 && data->index >= 0 && data->index < VECTOR_MAX_ARTBOARDS &&
           isfinite(rect.x) && isfinite(rect.y) && isfinite(rect.w) && isfinite(rect.h) &&
           rect.w >= 1 && rect.h >= 1;
}

typedef struct {
    VectorArtboard before, after;
} EditArtboard;

static bool replace_board(void *state, const VectorArtboard *expected,
                          const VectorArtboard *replacement) {
    VectorDocument *document = state;
    for (int i = 0; i < document->artboard_count; i++) {
        VectorArtboard *board = &document->artboards[i];
        if (board->id == expected->id) {
            if (strcmp(board->name, expected->name) ||
                memcmp(&board->rect, &expected->rect, sizeof board->rect)) {
                return false;
            }
            *board = *replacement;
            return true;
        }
    }
    return false;
}

static bool edit_apply(void *state, const void *payload) {
    const EditArtboard *edit = payload;
    return replace_board(state, &edit->before, &edit->after);
}

static bool edit_revert(void *state, const void *payload) {
    const EditArtboard *edit = payload;
    return replace_board(state, &edit->after, &edit->before);
}

bool vector_artboard_update(VectorDocument *document, unsigned id, const char *name, float width,
                            float height) {
    if (!name || !*name || strlen(name) >= 64 || !isfinite(width) || !isfinite(height) ||
        width < 1 || height < 1 || width > VECTOR_MAX_ARTBOARD_SIZE ||
        height > VECTOR_MAX_ARTBOARD_SIZE) {
        return false;
    }
    for (int i = 0; i < document->artboard_count; i++) {
        VectorArtboard before = document->artboards[i];
        if (before.id != id) {
            continue;
        }
        if (!strcmp(before.name, name) && before.rect.w == width && before.rect.h == height) {
            return true;
        }
        EditArtboard *edit = calloc(1, sizeof *edit);
        if (!edit) {
            return false;
        }
        edit->before = edit->after = before;
        snprintf(edit->after.name, sizeof edit->after.name, "%s", name);
        edit->after.rect.w = width;
        edit->after.rect.h = height;
        if (vector_history_execute(&document->history, document,
                                   (VectorCommand){edit_apply, edit_revert, edit})) {
            return true;
        }
        free(edit);
        return false;
    }
    return false;
}

static bool valid_board(VectorArtboard board) {
    return board.id && memchr(board.name, 0, sizeof board.name) && *board.name &&
           isfinite(board.rect.x) && isfinite(board.rect.y) && isfinite(board.rect.w) &&
           isfinite(board.rect.h) && board.rect.w >= 1 && board.rect.h >= 1 &&
           board.rect.w <= VECTOR_MAX_ARTBOARD_SIZE && board.rect.h <= VECTOR_MAX_ARTBOARD_SIZE;
}

bool vector_artboard_command_write(const VectorCommand *command, FILE *file) {
    if (command->apply == edit_apply && command->revert == edit_revert && command->data) {
        unsigned kind = 2;
        return fwrite(&kind, sizeof kind, 1, file) == 1 &&
               fwrite(command->data, sizeof(EditArtboard), 1, file) == 1;
    }
    if (command->apply != apply || command->revert != revert || !command->data) {
        return false;
    }
    const AddArtboard *data = command->data;
    unsigned kind = 1;
    return valid_command(data) && fwrite(&kind, sizeof kind, 1, file) == 1 &&
           fwrite(&data->artboard, sizeof data->artboard, 1, file) == 1 &&
           fwrite(&data->index, sizeof data->index, 1, file) == 1 &&
           fwrite(&data->previous, sizeof data->previous, 1, file) == 1;
}

bool vector_artboard_command_read(VectorCommand *command, FILE *file) {
    unsigned kind;
    AddArtboard data;
    if (fread(&kind, sizeof kind, 1, file) != 1) {
        return false;
    }
    if (kind == 2) {
        EditArtboard edit;
        if (fread(&edit, sizeof edit, 1, file) != 1 || !valid_board(edit.before) ||
            !valid_board(edit.after) || edit.before.id != edit.after.id ||
            edit.before.rect.x != edit.after.rect.x || edit.before.rect.y != edit.after.rect.y) {
            return false;
        }
        EditArtboard *owned = malloc(sizeof *owned);
        if (!owned) {
            return false;
        }
        *owned = edit;
        *command = (VectorCommand){edit_apply, edit_revert, owned};
        return true;
    }
    if (kind != 1 || fread(&data.artboard, sizeof data.artboard, 1, file) != 1 ||
        fread(&data.index, sizeof data.index, 1, file) != 1 ||
        fread(&data.previous, sizeof data.previous, 1, file) != 1 || !valid_command(&data) ||
        !valid_board(data.artboard)) {
        return false;
    }
    AddArtboard *owned = malloc(sizeof *owned);
    if (!owned) {
        return false;
    }
    *owned = data;
    *command = (VectorCommand){apply, revert, owned};
    return true;
}
