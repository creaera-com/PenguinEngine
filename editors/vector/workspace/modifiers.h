/** @file
 * @brief Native event translation for live Shift/Alt transform modifiers. */
#ifndef PE_VECTOR_WORKSPACE_MODIFIERS_H
#define PE_VECTOR_WORKSPACE_MODIFIERS_H
#include "pengui/pengui.h"
/** @brief Refresh flags from pointer state or modifier-key events. True for Shift/Alt keys. */
bool vector_transform_modifiers(const SDL_Event *event, bool *aspect, bool *centered);
#endif
