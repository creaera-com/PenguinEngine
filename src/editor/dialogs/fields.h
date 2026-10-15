/**
 * @file
 * @brief Private Modulverträge für src/editor/dialogs.
 * @ingroup editor_api
 */

/** @addtogroup editor_api
 * @{
 */
#ifndef PENGUIN_DIALOGS_FIELDS_H
#define PENGUIN_DIALOGS_FIELDS_H

/** @brief Art des aktuell geöffneten modalen Map-Eingabefelds. */
typedef enum {
    FIELD_NONE,
    FIELD_IMPORT,
    FIELD_TILESIZE,
    FIELD_LAYER,
    FIELD_X,
    FIELD_Y,
    FIELD_W,
    FIELD_H,
    FIELD_VX,   ///< Body-Geschwindigkeit in Weltpixeln pro Sekunde.
    FIELD_VY,   ///< Body-Geschwindigkeit in Weltpixeln pro Sekunde.
    FIELD_COLOR ///< Sprite-Farbe als acht hexadezimale RGBA-Ziffern.
} Field;

/**
 * @brief Zeichnet Speicher-Rückfragen oder das aktive Texteingabefeld.
 */
void editor_draw_modal(void);

/**
 * @brief Öffnet ein modales Eingabefeld mit einem kopierten Anfangswert.
 */
void editor_field_start(Field f, const char *value);

/**
 * @brief Schließt das Eingabefeld und stellt den Scriptfokus bei Bedarf wieder her.
 */
void editor_field_stop(void);
/** @} */
#endif
