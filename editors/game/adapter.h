/** @file
 * @brief Public entry point of the Game editor module. */
#ifndef PE_GAME_ADAPTER_H
#define PE_GAME_ADAPTER_H
#include "editor_host/editor.h"
/** @brief Static definition; no editor instance is allocated by this call. */
const EditorDefinition *game_editor_definition(void);
#endif
