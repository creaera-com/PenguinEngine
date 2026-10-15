/**
 * @file
 * @brief Private Modulverträge für src/editor/editors/scripts.
 * @ingroup editor_api
 */

/** @addtogroup editor_api
 * @{
 */
#ifndef PENGUIN_EDITORS_SCRIPTS_PANELS_H
#define PENGUIN_EDITORS_SCRIPTS_PANELS_H
#include <SDL3/SDL.h>
#include <stdbool.h>

/**
 * @brief Zeichnet die aktive Scriptansicht im Canvasbereich.
 */
void editor_draw_script_content(void);

/**
 * @brief Zeichnet Scriptinformationen und Tastenkürzel im aktuellen Panel.
 */
void editor_draw_script_inspector(void);

/**
 * @brief Zeichnet die YAML-Werkzeugleiste des Script-Editors.
 */
void editor_draw_script_tools(SDL_FRect area);

/**
 * @brief Verarbeitet eine bekannte Scriptaktion und meldet, ob der Name erkannt wurde.
 */
bool editor_script_action(const char *name);
/** @} */
#endif
