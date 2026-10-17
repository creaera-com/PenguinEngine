/** @file
 * @brief Integrationsvertrag für austauschbare Editor-Frontends. */
#ifndef PENGUIN_EDITOR_MODULE_H
#define PENGUIN_EDITOR_MODULE_H
#include "cJSON.h"
#include "editor/document/document.h"
#include "editor/workspace/session.h"
#include "penguin/ui_sdl.h"

/** @brief Lebenszyklus-Adapter; alle Funktionen arbeiten mit dem aktiven Anwendungshost.
 * create reserviert einen leeren Payload, initialize erzeugt ein neues Dokument.
 * checkpoint lädt alternativ den reservierten Payload. destroy akzeptiert Teilinitialisierung.
 * Fehler werden im Aufruferpuffer gemeldet. Die gemeinsame Konsole gehört keinem Editor.
 */
typedef struct {
    void (*focus)(void); ///< Übernimmt den Tastaturfokus nach einem Editorwechsel.
    void (*request)(int command,
                    const char *path);           ///< Führt einen registrierten Dokumentbefehl aus.
    bool (*event)(SDL_Event *event, bool modal); ///< Fachliche Eingaben vor allgemeinen Shortcuts.
    bool (*overlay_event)(const SDL_Event *event); ///< Eingaben für offene fachliche Popups.
    void (*pointer)(SDL_Event *event, bool modal); ///< Verbleibende Zeigereingaben.
    void (*key)(SDL_Keycode key);                  ///< Fachliche Tastenkürzel ohne Modifier.
    void (*cancel)(void);           ///< Bricht temporäre Gesten ab; bei Fokusverlust und Escape.
    void (*blur)(void);             ///< Gibt den Textfokus an Shell oder Konsole zurück.
    void (*zoom)(float amount);     ///< Zoomt den fachlichen Canvas.
    bool (*drop)(const char *path); ///< Importiert eine nicht global zugeordnete Datei.
    UiScroll *(*panel_scroll)(UiId tab);    ///< Optionale fachliche Scrollposition, geliehen.
    bool (*controls_hit)(float x, float y); ///< Zusätzliche fachliche Bedienelemente.
    bool (*overlay_open)(void);             ///< Sperrt darunterliegende Bedienelemente.
    void (*begin_frame)(void);              ///< Setzt flüchtige Darstellungsdaten zurück.
    void (*draw_overlay)(void);             ///< Zeichnet fachliche Popups über den Panels.
    void (*draw_node)(const char *type, const cJSON *node,
                      SDL_FRect area); ///< Zeichnet fachliche YAML-Inhalte.
    const char *canvas_type;           ///< YAML-Knoten des interaktiven Canvas.
    void (*field_stopped)(void);       ///< Räumt fachliche Dialogzustände auf, vor Feldreset.
    UiId (*text_focus)(void); ///< Gewünschter Textfokus nach einem Dialog; 0 ohne Textfokus.
    void (*notify)(const char *message);      ///< Empfängt gemeinsame Statusmeldungen, optional.
    void (*update)(double elapsed);           ///< Aktualisiert ausschließlich den aktiven Editor.
    void (*startup)(const char *hint);        ///< Optionale editorbezogene Startansicht.
    bool (*create)(char *error, size_t size); ///< Reserviert den Payload; false bei Speicherfehler.
    bool (*initialize)(char *error, size_t size); ///< Initialisiert ohne Sitzungsdatei.
    void (*destroy)(void);             ///< Gibt Modell, Verlauf und Caches frei; wiederholbar.
    bool (*checkpoint)(PeSession *io); ///< Speichert/lädt den fachlichen Payload.
    bool (*action)(const char *name);  ///< Führt eine bekannte fachliche Aktion aus.
    bool (*action_enabled)(const char *name); ///< Prüft fachliche Aktionsvoraussetzungen.
    bool (*action_active)(const char *name);  ///< Liest den Auswahlzustand einer Werkzeugaktion.
    void (*apply_field)(void); ///< Validiert und übernimmt das aktive fachliche Eingabefeld.
    void (*ready)(void); ///< Erzeugt flüchtige Caches nach Initialisierung oder Wiederherstellung.
    bool (*boolean_binding)(
        const char *name);        ///< Liest editorbezogene YAML-Bools; unbekannt ergibt false.
    const char *(*message)(void); ///< Geliehene editorbezogene Statusmeldung, optional.
    bool (*playing)(void);        ///< Prüft eine laufende Spielvorschau, optional.
    PeDocument (*document)(void); ///< Liefert eine kurzlebige Dokumentbindung.
} PeEditorBackend;

/** @brief Registrierter Arbeitsbereich; Metadaten und ausdrücklich zugewiesene Operationen. */
typedef struct {
    UiId id;                            ///< Stabile Editor-ID; unabhängig von Listenreihenfolge.
    const char *label;                  ///< Statische Beschriftung.
    UiIcon icon;                        ///< Navigationssymbol.
    int action_count;                   ///< Anzahl kontextabhängiger Werkzeugaktionen.
    void (*draw_tools)(SDL_FRect area); ///< Zeichnet die kontextabhängige Werkzeugleiste.
    void (*draw_content)(void);         ///< Zeichnet den Editorinhalt.
    const char *module;                 ///< Modulname in shell.yaml.
    int open_request;                   ///< Befehl zum Öffnen; 0 ohne Unterstützung.
    int new_request;                    ///< Befehl für ein neues Dokument; 0 ohne Unterstützung.
    int import_request;                 ///< Befehl zum Importieren; 0 ohne Unterstützung.
    const PeEditorBackend *backend;     ///< Statischer Lebenszyklus- und Dokumentadapter.
} EditorView;

/** @brief Kurzlebige Dokumentbindung des aktiven Editors; leer, wenn kein Editor geladen ist. */
PeDocument editor_active_document(void);
/** @brief Gibt den aktiven Payload und sein Layout frei; die Konsole bleibt bestehen. */
void editor_release_active(void);
#endif
