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

/** @brief Art des aktuell geöffneten Map-Eingabefelds. */
typedef enum {
    FIELD_NONE,
    FIELD_IMPORT,
    FIELD_TILESIZE,
    FIELD_LAYER,
    FIELD_X,
    FIELD_Y,
    FIELD_W,
    FIELD_H,
    FIELD_VX,     ///< Body-Geschwindigkeit in Weltpixeln pro Sekunde.
    FIELD_VY,     ///< Body-Geschwindigkeit in Weltpixeln pro Sekunde.
    FIELD_COLOR,  ///< Sprite-Farbe als acht hexadezimale RGBA-Ziffern.
    FIELD_NAME,   ///< Gespeicherter Anzeigename der Entity.
    FIELD_V_NAME, ///< Vector-Objekt-/Artboard-Name.
    FIELD_V_X,
    FIELD_V_Y,
    FIELD_V_W,
    FIELD_V_H,
    FIELD_V_FILL,
    FIELD_V_STROKE,
    FIELD_V_STROKE_WIDTH,
    FIELD_V_BACKGROUND,
    FIELD_V_OPEN,
    FIELD_V_SAVE

} Field;

/**
 * @brief Zeichnet Speicher-Rückfragen oder das aktive Texteingabefeld.
 */
void editor_draw_modal(void);

/**
 * @brief Öffnet ein modales Eingabefeld mit einem kopierten Anfangswert.
 */
void editor_field_start(Field f, const char *value);

/** @brief Zeichnet ein direkt bearbeitbares Feld für die ausgewählte Entity.
 * @param field Zu bearbeitende Eigenschaft.
 * @param value Anzeigewert, solange das Feld keinen Fokus hat.
 * @param input Kopierter Anfangswert beim Aktivieren.
 * @param cell Sichtbarer Feldbereich.
 * @param enabled Ob Bearbeitung erlaubt ist.
 */
void editor_draw_input(Field field, const char *value, const char *input, UiRect cell,
                       bool enabled);

/** @brief Beginnt direkte Komponentenbearbeitung; der Anfangswert wird zum Ersetzen markiert.
 * @param field Zu bearbeitende Eigenschaft.
 * @param value Geliehener Anfangswert; wird kopiert.
 * @param id Stabile Feld-ID für Fokus und Rendering.
 * @param rect Sichtbarer Feldbereich in Fensterpixeln.
 */
void editor_field_start_inline(Field field, const char *value, UiId id, UiRect rect);
/** @brief Verarbeitet Text-/Cursorereignisse und bestätigt direkte Felder beim Fokuswechsel.
 * @param event Geliehenes SDL-Ereignis des Hauptfensters.
 * @return true, wenn das Ereignis vollständig vom Feld verarbeitet wurde.
 */
bool editor_field_event(SDL_Event *event);

/**
 * @brief Schließt das Eingabefeld und stellt den Scriptfokus bei Bedarf wieder her.
 */
void editor_field_stop(void);
/** @} */
#endif
