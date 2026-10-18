/** @file @brief Undoable artboard creation. */
#include "document.h"
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

bool vector_artboard_add(VectorDocument *document) {
    if (document->artboard_count >= VECTOR_MAX_ARTBOARDS) {
        return false;
    }
    AddArtboard *command = malloc(sizeof *command);
    if (!command) {
        return false;
    }
    int index = document->artboard_count;
    *command =
        (AddArtboard){.artboard = {.id = document->next_artboard_id,
                                   .rect = {(index % 2) * 576.f, (index / 2) * 576.f, 512, 512}},
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
