/** @file @brief Session-owned temporary files; editor payloads remain opaque to the host. */
#ifndef PE_EDITOR_HOST_CHECKPOINT_H
#define PE_EDITOR_HOST_CHECKPOINT_H
#include "editor.h"

typedef struct EditorCheckpoint {
    const EditorDefinition *definition;
    FILE *file;
    struct EditorCheckpoint *next;
} EditorCheckpoint;

/** @brief Stage a new snapshot without replacing the last successful checkpoint. */
bool editor_checkpoint_capture(const EditorDefinition *definition, const void *instance,
                               const PUIView *view, EditorCheckpoint **result, char *error,
                               size_t capacity);
/** @brief Borrow a stored session file; NULL means the editor has not been suspended. */
FILE *editor_checkpoint_find(EditorCheckpoint *list, const EditorDefinition *definition);
/** @brief Rewind and restore a candidate before its layout is mounted. */
bool editor_checkpoint_restore(const EditorDefinition *definition, void *instance, FILE *file,
                               char *error, size_t capacity);
/** @brief Publish a staged snapshot after the switch succeeds, closing any older version. */
void editor_checkpoint_publish(EditorCheckpoint **list, EditorCheckpoint *staged);
/** @brief Close files (which removes them) and free the list. NULL is accepted. */
void editor_checkpoint_destroy(EditorCheckpoint *list);
#endif
