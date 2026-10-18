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
