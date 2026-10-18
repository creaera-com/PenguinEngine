/** @file @brief Undoable artboard creation. */
#include "document.h"
#include <math.h>
#include <stdlib.h>

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
        rect.w < 1 || rect.h < 1 || document->artboard_count >= VECTOR_MAX_ARTBOARDS) {
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

bool vector_artboard_command_write(const VectorCommand *command, FILE *file) {
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
    if (fread(&kind, sizeof kind, 1, file) != 1 || kind != 1 ||
        fread(&data.artboard, sizeof data.artboard, 1, file) != 1 ||
        fread(&data.index, sizeof data.index, 1, file) != 1 ||
        fread(&data.previous, sizeof data.previous, 1, file) != 1 || !valid_command(&data)) {
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
