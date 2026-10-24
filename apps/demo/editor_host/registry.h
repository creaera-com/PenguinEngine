/** @file
 * @brief Explicit editor registration; no runtime modules are retained here. */
#ifndef PE_EDITOR_REGISTRY_H
#define PE_EDITOR_REGISTRY_H
#include "editor.h"
/** @brief Find a module definition by stable ID; NULL for unknown IDs. */
const EditorDefinition *editor_registry_find(const char *id);
/** @brief Resolve a supported document extension without exposing concrete adapters. */
const EditorDefinition *editor_registry_for_file(const char *path);
#endif
