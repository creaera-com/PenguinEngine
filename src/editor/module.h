/** @file
 * @brief Integrationsvertrag für austauschbare Editor-Frontends. */
#ifndef PENGUIN_EDITOR_MODULE_H
#define PENGUIN_EDITOR_MODULE_H
#include "editor/document/document.h"
#include "editor/workspace/session.h"
#include "penguin/ui_sdl.h"

/** @brief Lebenszyklus-Adapter; alle Funktionen arbeiten mit dem aktiven Anwendungshost.
 * create reserviert einen leeren Payload, initialize erzeugt ein neues Dokument.
 * checkpoint lädt alternativ den reservierten Payload. destroy akzeptiert Teilinitialisierung.
 * Fehler werden im Aufruferpuffer gemeldet. Die gemeinsame Konsole gehört keinem Editor.
 */
typedef struct {
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
    const PeEditorBackend *backend;     ///< Statischer Lebenszyklus- und Dokumentadapter.
} EditorView;

/** @brief Kurzlebige Dokumentbindung des aktiven Editors; leer, wenn kein Editor geladen ist. */
PeDocument editor_active_document(void);
/** @brief Gibt den aktiven Payload und sein Layout frei; die Konsole bleibt bestehen. */
void editor_release_active(void);
#endif
