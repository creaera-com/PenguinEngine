/** @file @brief Public entry point of the Vector editor module. */
#ifndef PE_VECTOR_ADAPTER_H
#define PE_VECTOR_ADAPTER_H
#include "editor_host/editor.h"
/** @brief Static definition; no editor instance is allocated by this call. */
const EditorDefinition *vector_editor_definition(void);
#endif
