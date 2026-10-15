/**
 * @file
 * @brief Private Modulverträge für src/editor/app.
 * @ingroup editor_api
 */

/** @addtogroup editor_api
 * @{
 */
#ifndef PENGUIN_APP_UI_HELPERS_H
#define PENGUIN_APP_UI_HELPERS_H
#include "penguin/ui_sdl.h"

/**
 * @brief Zeichnet eine gefüllte Editorfläche in RGBA.
 */
void editor_box(SDL_FRect r, uint32_t c);
/** @brief Liefert die um 5 Prozent Weiß aufgehellte Theme-Surface als Widget-Hintergrund. */
Uint32 editor_widget_background(void);
/** @brief Liefert dezente alternierende Tree-Zeilenfarben aus dem aktuellen Theme.
 * @param row Nullbasierter Index in der aufgeklappten Liste, unabhängig vom Scrolloffset.
 */
Uint32 editor_tree_row_background(int row);

/**
 * @brief Zeichnet und verarbeitet einen Textbutton mit aufruferspezifischer ID.
 */
bool editor_button_impl(UiId callsite, float x, float y, float w, const char *label, bool active,
                        bool enabled);

/**
 * @brief Zeichnet und verarbeitet eine klappbare Überschrift.
 */
void editor_disclosure(float x, float y, float w, const char *title, bool *open);

/**
 * @brief Setzt die SDL-Zeichenfarbe des Editor-Renderers.
 */
void editor_draw_color(uint32_t c);

/**
 * @brief Zeichnet den mittigen Drei-Punkte-Griff einer Spaltengrenze.
 */
void editor_draw_split_grip(UiRect area, bool highlighted);

/**
 * @brief Zeichnet und verarbeitet einen Iconbutton einschließlich Tooltip und Trefferbereich.
 */
bool editor_icon_button_impl(UiId callsite, float x, float y, float w, UiIcon icon,
                             const char *label, const char *hint, bool active, bool enabled);

/**
 * @brief Prüft einen Punkt gegen ein SDL-Rechteck.
 */
bool editor_inside(float x, float y, SDL_FRect r);

/**
 * @brief Registriert einen Widget-Trefferbereich unter Berücksichtigung des aktuellen Clips.
 */
void editor_register_ui_hit(UiRect rect);

/**
 * @brief Zeichnet Icon und Titel eines Editorabschnitts.
 */
void editor_section(float x, float y, UiIcon icon, const char *title);

/**
 * @brief Zeichnet einen Editorbeschriftungstext an der verwendeten Textbasisposition.
 */
void editor_text(float x, float y, const char *label, uint32_t color);
/** @} */
#endif
