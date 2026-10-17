/** @file @brief Public entry point of the Scripts editor module. */
#ifndef PE_SCRIPTS_ADAPTER_H
#define PE_SCRIPTS_ADAPTER_H
#include "editor_host/editor.h"
/** @brief Static definition; no editor instance is allocated by this call. */
const EditorDefinition *scripts_editor_definition(void);
#endif
