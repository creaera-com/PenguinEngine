/** @file @brief Document-owned command history; callbacks must apply/revert atomically. */
#ifndef PE_VECTOR_HISTORY_H
#define PE_VECTOR_HISTORY_H
#include <stdbool.h>
#define VECTOR_HISTORY_CAPACITY 128

typedef struct {
    bool (*apply)(void *document, const void *data);
    bool (*revert)(void *document, const void *data);
    void *data; ///< Owned heap allocation, released when discarded or history is destroyed.
} VectorCommand;

typedef struct {
    VectorCommand commands[VECTOR_HISTORY_CAPACITY];
    int count, cursor;
    int floor; ///< Imported initial state cannot be undone; edits above it can.
} VectorHistory;

/** @brief Execute and take ownership on success; caller retains data on failure.
 * A successful edit after Undo discards the previous Redo branch.
 */
bool vector_history_execute(VectorHistory *history, void *document, VectorCommand command);
bool vector_history_undo(VectorHistory *history, void *document);
bool vector_history_redo(VectorHistory *history, void *document);
void vector_history_destroy(VectorHistory *history);
#endif
