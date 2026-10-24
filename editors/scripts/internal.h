/** @file
 * @brief Private Scripts editor lifecycle and panel assembly. */
#ifndef PE_SCRIPTS_INTERNAL_H
#define PE_SCRIPTS_INTERNAL_H
#include "editor_host/editor.h"

typedef struct {
    PUILayout *layout;
} ScriptsEditor;

void *scripts_editor_create(const EditorServices *services, char *error, size_t capacity);
const PUILayout *scripts_editor_layout(const void *instance);
void scripts_editor_destroy(void *instance);
/** @brief Load the inspector's separate UI description; caller owns the result. */
PUILayout *scripts_inspector_load(const EditorServices *services, char *error, size_t capacity);
#endif
