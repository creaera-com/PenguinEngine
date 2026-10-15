/**
 * @file
 * @brief Private Modulverträge für src/editor/app.
 * @ingroup editor_api
 */

/** @addtogroup editor_api
 * @{
 */
#ifndef PENGUIN_APP_SHELL_H
#define PENGUIN_APP_SHELL_H
#include <SDL3/SDL.h>
#include <stdbool.h>

/**
 * @brief Zeichnet einen vollständigen UI-Frame und verarbeitet danach aufgeschobene Aktionen.
 */
void editor_draw(void);

/**
 * @brief Verteilt SDL-Ereignisse an Shell, Dialoge und den aktiven Editor.
 */
void editor_event(SDL_Event *e);

/**
 * @brief Prüft Splitter, registrierte Widgets und Script-Scrollbars vor Canvas-Eingaben.
 */
bool editor_ui_controls_hit(float x, float y);
/** @} */
#endif
