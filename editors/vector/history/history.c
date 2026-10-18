/** @file @brief Reusable command stack within the Vector editor domain. */
#include "history.h"
#include <stdlib.h>

bool vector_history_execute(VectorHistory *history, void *document, VectorCommand command) {
    if (!command.apply || !command.revert || history->cursor >= VECTOR_HISTORY_CAPACITY ||
        !command.apply(document, command.data)) {
        return false;
    }
    for (int i = history->cursor; i < history->count; i++) {
        free(history->commands[i].data);
    }
    history->count = history->cursor;
    history->commands[history->count++] = command;
    history->cursor = history->count;
    return true;
}

bool vector_history_undo(VectorHistory *history, void *document) {
    if (!history->cursor || !history->commands[history->cursor - 1].revert(
                                document, history->commands[history->cursor - 1].data)) {
        return false;
    }
    history->cursor--;
    return true;
}

bool vector_history_redo(VectorHistory *history, void *document) {
    if (history->cursor >= history->count ||
        !history->commands[history->cursor].apply(document,
                                                  history->commands[history->cursor].data)) {
        return false;
    }
    history->cursor++;
    return true;
}

void vector_history_destroy(VectorHistory *history) {
    for (int i = 0; i < history->count; i++) {
        free(history->commands[i].data);
    }
    *history = (VectorHistory){0};
}
