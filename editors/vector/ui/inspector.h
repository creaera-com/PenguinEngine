/** @file @brief Inspector UI assembly; independent of the editor's private instance. */
#ifndef PE_VECTOR_INSPECTOR_H
#define PE_VECTOR_INSPECTOR_H
#include "editor_host/editor.h"
/** @brief Load the inspector description; caller owns the returned layout. */
PUILayout *vector_inspector_load(const EditorServices *services, char *error, size_t capacity);
#endif
