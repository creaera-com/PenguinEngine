/** @file @brief Private Vector editor lifecycle and panel assembly. */
#ifndef PE_VECTOR_INTERNAL_H
#define PE_VECTOR_INTERNAL_H
#include "editor_host/editor.h"

typedef struct {
    PUILayout *layout;
} VectorEditor;

void *vector_editor_create(const EditorServices *services, char *error, size_t capacity);
const PUILayout *vector_editor_layout(const void *instance);
void vector_editor_destroy(void *instance);
/** @brief Load the inspector's separate UI description; caller owns the result. */
PUILayout *vector_inspector_load(const EditorServices *services, char *error, size_t capacity);
#endif
