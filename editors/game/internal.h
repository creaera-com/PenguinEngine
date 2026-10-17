/** @file @brief Private Game editor lifecycle and panel assembly. */
#ifndef PE_GAME_INTERNAL_H
#define PE_GAME_INTERNAL_H
#include "editor_host/editor.h"

typedef struct {
    PUILayout *layout;
} GameEditor;

void *game_editor_create(const EditorServices *services, char *error, size_t capacity);
const PUILayout *game_editor_layout(const void *instance);
void game_editor_destroy(void *instance);
/** @brief Load the inspector's separate UI description; caller owns the result. */
PUILayout *game_inspector_load(const EditorServices *services, char *error, size_t capacity);
#endif
