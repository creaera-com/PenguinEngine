/**
 * @file
 * @brief Interne Scriptansicht mit UTF-8-Bearbeitung und Syntaxdiagnose.
 * @ingroup editor_api
 */

/** @addtogroup editor_api
 * @{
 */
#ifndef PENGUIN_SCRIPT_EDITOR_H
#define PENGUIN_SCRIPT_EDITOR_H
#include "penguin/script.h"
#include "penguin/ui_sdl.h"

/** @brief Besitzende Scriptansicht mit Text, Undo/Redo und regenerierbaren Darstellungscaches. */
typedef struct PeScriptEditor {
    SDL_Window *window;            ///< Geliehenes SDL-Fenster; besitzt die Anwendung.
    UiScroll vertical;             ///< Vertikaler Scrollzustand in Pixeln.
    UiScroll horizontal;           ///< Horizontaler Scrollzustand in Pixeln.
    int line_count;                ///< Anzahl Textzeilen im berechneten Layout.
    int max_column;                ///< Maximale Zeichenspaltenzahl im Textlayout.
    uint64_t extent_revision;      ///< Revision des zuletzt berechneten Textumfangs.
    PeText document;               ///< Besitzendes Script-Textdokument mit Undo/Redo.
    PeJsToken tokens[PE_TEXT_MAX]; ///< Regenerierbare Highlightingklasse pro Textbyte.
    PeJsDiagnostic diagnostic;     ///< Ergebnis der letzten Syntaxprüfung.
    PeCompletions completions;     ///< Aktuelle Vervollständigungsvorschläge.
    char root[768];                ///< Besitzender Modellbaum bzw. kopierter Projektwurzelpfad.
    char path[1024];               ///< Nullterminierter Datei- oder Projektpfad.
    char status[256];              ///< Kopierte Statusmeldung.
    bool focused;                  ///< Ob die Scriptansicht Texteingaben erhält.
    bool selecting;                ///< Ob gerade eine Textauswahl per Zeiger gezogen wird.
    bool exists;                   ///< Ob bereits eine Datei am Dokumentpfad existiert.
    bool show_completion;          ///< Ob die Vorschlagsliste angezeigt wird.
    int top_line;                  ///< Oberste sichtbare Textzeile, nullbasiert.
    int left_column;               ///< Linke sichtbare Textspalte, nullbasiert.
    int completion_index;          ///< Ausgewählter Eintrag der Vorschlagsliste.
    int goal_column; ///< Gewünschte Spalte für vertikale Cursorbewegungen; -1 bedeutet ungesetzt.
    uint64_t highlighted_revision; ///< Revision des aktuellen Highlightingcaches; UINT64_MAX
                                   ///< erzwingt Neuberechnung.
    uint64_t checked_revision;     ///< Revision der aktuellen Syntaxdiagnose.
    Uint64 changed_at;             ///< Zeitpunkt der letzten Textänderung in SDL-Nanosekunden.
    SDL_FRect body;                ///< Berechneter Textbereich im Fenster.
    SDL_FRect popup;               ///< Berechneter Bereich der Vorschlagsliste.
} PeScriptEditor;

/**
 * @brief Initialisiert die Scriptansicht und lädt player.js oder bereitet new_script.js vor.
 */
bool se_init(PeScriptEditor *editor, const char *root);

/**
 * @brief Gibt den besitzenden Textverlauf der Scriptansicht frei.
 */
void se_free(PeScriptEditor *editor);

/**
 * @brief Setzt Ansichts- und SDL-Texteingabefokus und beendet aktive Auswahlgesten.
 */
void se_focus(PeScriptEditor *editor, SDL_Window *window, bool focus);

/**
 * @brief Öffnet eine Scriptdatei und aktualisiert Ansicht und Syntaxdiagnose.
 */
bool se_open(PeScriptEditor *editor, const char *path);

/**
 * @brief Speichert das aktive Script und aktualisiert Existenz- und Änderungszustand.
 */
bool se_save(PeScriptEditor *editor);

/**
 * @brief Prüft die Speicherrevision des angezeigten Dokuments.
 */
bool se_dirty(const PeScriptEditor *editor);

/**
 * @brief Aktualisiert JavaScript-Syntaxdiagnose und geprüfte Revision.
 */
void se_check(PeScriptEditor *editor);

/**
 * @brief Aktualisiert Layout, Cursoransicht und optional die Vervollständigung nach Änderungen.
 */
void se_changed(PeScriptEditor *editor, SDL_FRect area, bool completion);
/* Returns true when input belongs to script editor rather than scene tools. */
/**
 * @brief Verarbeitet Script-Eingaben und meldet, ob das Ereignis verbraucht wurde.
 */
bool se_event(PeScriptEditor *editor, SDL_Window *window, const SDL_Event *event, SDL_FRect area);

/**
 * @brief Prüft, ob ein Punkt die Scrollbarbereiche der Scriptansicht trifft.
 */
bool se_scroll_hit(PeScriptEditor *editor, float x, float y, SDL_FRect area);

/**
 * @brief Zeichnet Scripttext, Auswahl, Zeilennummern, Vorschläge und Diagnose.
 */
void se_draw(PeScriptEditor *editor, SDL_Renderer *renderer, UiContext *context, SDL_FRect area,
             float mx, float my, bool click);
/** @} */
#endif
