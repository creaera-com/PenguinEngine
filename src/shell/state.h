/** @file
 * @brief Besitz- und Zustandsbereich shell; keine globale Instanz. */
#ifndef PENGUIN_PESHELLSTATE_H
#define PENGUIN_PESHELLSTATE_H
#include "editor/layout/ui_layout.h"
#include "editor/workspace/workspace.h"
#include "penguin/ui_sdl.h"
#include "shell/console/view.h"
#include "shell/dialogs/fields.h"
#include "shell/explorer/file_tree.h"
#include <stdbool.h>

/** @brief Maximale Zahl sichtbarer Projektbaumeinträge. */
#define TREE_MAX 256

/** @brief Serialisierbarer Zustand einer YAML-Tabgruppe oder eines klappbaren Bereichs. */
typedef struct {
    UiId id;                 ///< Stabile Widget- oder Editor-ID.
    UiId selected;           ///< Stabile ID der aktuellen Auswahl; 0 bedeutet keine Auswahl.
    UiId order[UI_MAX_TABS]; ///< Stabile Tab-IDs in aktueller Reihenfolge.
    int count;               ///< Anzahl gültiger Einträge.
    int open;                ///< Aufklappzustand eines YAML-Abschnitts.
    int dragging;            ///< Gespeicherter Dragindex der Tabgruppe.
    float scroll;            ///< Tab- oder Tool-Panel-Scrolloffset in Pixeln.
    float extent;            ///< Gespeicherte Größe des zweiten Split-Panels.
    int kind;                ///< 0 für Tabs/Abschnitte, 1 für Split-Container, 2 für Tool-Panels.
} WidgetState;

/** @brief Zustand für shell; Lebensdauer wird vom Anwendungshost koordiniert. */
typedef struct {
    PeConsole *console;           ///< Gemeinsame Shell; unabhängig vom geladenen Editor.
    bool layout_resize_x;         ///< Horizontaler YAML-Splitter unter dem Zeiger.
    bool layout_resize_y;         ///< Vertikaler YAML-Splitter unter dem Zeiger.
    UiContext ui;                 ///< Gemeinsamer Eingabe-, Fokus- und Clipzustand.
    UiId widget_scope;            ///< ID-Namensraum des aktuell gezeichneten YAML-Knotens.
    UiTabs workspaces;            ///< Globale Editor-Navigation.
    UiTabs left_tabs;             ///< Linke kontextabhängige Tabgruppe.
    UiTabs right_tabs;            ///< Rechte kontextabhängige Tabgruppe.
    UiScroll tree_view;           ///< Scrollzustand des Projektbaums.
    float left_width;             ///< Breite der linken Spalte in Pixeln.
    float right_width;            ///< Breite der rechten Spalte in Pixeln.
    PeUiLayout *shell_layout;     ///< Besitzendes globales YAML-Shellmodell.
    char ui_path[1024];           ///< Pfad zur Shell-YAML-Datei.
    int gap;                      ///< Abstand zwischen UI-Flächen bzw. Tabs in Pixeln.
    int tab_gap;                  ///< Abstand zwischen Tabs in Pixeln.
    float min_left;               ///< Mindestbreite der linken Spalte in Pixeln.
    float min_right;              ///< Mindestbreite der rechten Spalte in Pixeln.
    float min_center;             ///< Mindestbreite der mittleren Spalte in Pixeln.
    SDL_FRect panel_area;         ///< Momentan zugewiesener Zeichenbereich eines YAML-Adapters.
    float panel_top;              ///< Oberkante des aktuellen Panels in Pixeln.
    int global_nav_x;             ///< Horizontaler Start der globalen Editor-Navigation.
    WidgetState widgets[64];      ///< Serialisierbare YAML-Widgetzustände.
    int widget_count;             ///< Anzahl belegter Widgetzustände.
    UiRect hits[PE_UI_MAX_NODES]; ///< Im aktuellen Frame registrierte Widget-Trefferbereiche.
    int hit_count;                ///< Anzahl gültiger Widget-Trefferbereiche.
    float wheel_delta;            ///< Mauseraddistanz des aktuellen Frames.
    bool canvas_enabled;          ///< Ob der aktive Canvas Eingaben annehmen darf.
    bool rendering; ///< Ob gerade YAML-Knoten gezeichnet werden; Wechsel werden dann aufgeschoben.
    bool deferred_save; ///< Nach dem Frame auszuführender Speichern-/Fortsetzenbefehl.
    bool needs_layout;  ///< Ob vor dem nächsten Eingabeereignis die Geometrie neu gezeichnet werden
                        ///< muss.
    Request deferred_request; ///< Nach dem Frame auszuführende Datei-/Beendenanfrage.
    char deferred_path[1024]; ///< Kopierter Pfad der aufgeschobenen Anfrage.
    char status[256];         ///< Kopierte Statusmeldung.
    int tree_scroll;          ///< Index des obersten Projektbaumeintrags.
    float mx;                 ///< Aktuelle horizontale Zeigerposition in Fensterpixeln.
    float my;                 ///< Aktuelle vertikale Zeigerposition in Fensterpixeln.
    bool click;               ///< Noch zu verarbeitender UI-Klick.
    SDL_FRect viewport;       ///< Sichtbarer Inhaltsbereich bzw. dessen Ausdehnung in Pixeln.
    Field field;              ///< Aktives modales oder direktes Eingabefeld.
    char input[1024]; ///< Nullterminierter Eingabepuffer; erst Bestätigung ändert das Dokument.
    UiTextEdit field_edit;      ///< UTF-8-Cursor, Auswahl und horizontaler Scrollzustand.
    UiRect field_rect;          ///< Aktueller sichtbarer Bereich des aktiven Felds.
    UiId field_id;              ///< Stabile UI-ID des aktiven Eingabefelds.
    uint32_t field_entity;      ///< Besitzer-ID eines direkt bearbeiteten Komponentenfelds.
    Uint64 field_blink;         ///< SDL-Zeitpunkt der letzten Cursoraktion in Millisekunden.
    bool field_inline;          ///< Ob direkt im Inspector statt im Dialog bearbeitet wird.
    bool field_invalid;         ///< Ob die letzte Validierung fehlgeschlagen ist.
    bool field_seen;            ///< Aktives direktes Feld wurde im aktuellen Frame gezeichnet.
    bool field_selecting;       ///< Laufende Textauswahl mit gedrückter Maustaste.
    Request request;            ///< Aktive bestätigungspflichtige Anfrage.
    char open_path[1024];       ///< Kopierter Pfad der aktiven Anfrage.
    TreeRow tree[TREE_MAX];     ///< Sichtbare Projektbaumeinträge.
    int tree_count;             ///< Anzahl sichtbarer Projektbaumeinträge.
    char expanded[64][PE_PATH]; ///< Aufklappzustand bzw. Liste der aufgeklappten Projektpfade.
    int expanded_count;         ///< Anzahl aufgeklappter Projektpfade.
    UiScroll inspector_view;    ///< Scrollzustand des Inspectors.
    UiScroll scene_view;        ///< Scrollzustand der Entity-Liste.
    UiScroll info_view;         ///< Scrollzustand der Szeneninformationen.
    uint32_t selected;          ///< Stabile ID der ausgewählten Entity; 0 bedeutet keine.
} PeShellState;
#endif
