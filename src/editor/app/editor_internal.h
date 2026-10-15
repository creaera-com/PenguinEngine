/**
 * @file
 * @brief Private Integrationsschnittstelle der Editor-Anwendungsinstanz; keine Engine-API.
 * @ingroup editor_api
 */

/** @addtogroup editor_api
 * @{
 */
#ifndef PENGUIN_EDITOR_INTERNAL_H
#define PENGUIN_EDITOR_INTERNAL_H

#include "app/shell.h"
#include "app/ui_helpers.h"
#include "app/window.h"
#include "browser/file_tree.h"
#include "console/view.h"
#include "dialogs/fields.h"
#include "editors/map/checkpoint.h"
#include "editors/map/map_editor.h"
#include "editors/scripts/checkpoint.h"
#include "editors/scripts/panels.h"
#include "editors/scripts/script_editor.h"
#include "layout/runtime.h"
#include "layout/ui_layout.h"
#include "penguin/render.h"
#include "penguin/ui_sdl.h"
#include "workspace/session.h"
#include "workspace/workspace.h"
#include <errno.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/** @brief Interne Layout-/Darstellungsvorgabe LEFT. */
#define LEFT ((int)app.left_width)
/** @brief Interne Layout-/Darstellungsvorgabe RIGHT. */
#define RIGHT ((int)app.right_width)
/** @brief Interne Layout-/Darstellungsvorgabe TOP. */
#define TOP 50
/** @brief Interne Layout-/Darstellungsvorgabe TABS. */
#define TABS 32
/** @brief Interne Layout-/Darstellungsvorgabe BOTTOM. */
#define BOTTOM 28
/** @brief Interne Layout-/Darstellungsvorgabe PANEL_GAP. */
#define PANEL_GAP (app.gap)
/** @brief Interne Layout-/Darstellungsvorgabe GLOBAL_NAV_X. */
#define GLOBAL_NAV_X 232
/** @brief Interne Layout-/Darstellungsvorgabe SIDE_TOP. */
#define SIDE_TOP (TOP + TABS)
/** @brief Interne Layout-/Darstellungsvorgabe TREE_MAX. */
#define TREE_MAX 256
/** @brief Interne Widget-ID UI_SPLIT_LEFT. */
#define UI_SPLIT_LEFT 0x10001u
/** @brief Interne Widget-ID UI_SPLIT_RIGHT. */
#define UI_SPLIT_RIGHT 0x10002u
/** @brief Interne Widget-ID UI_TREE_SCROLL. */
#define UI_TREE_SCROLL 0x10003u
/** @brief Interne Widget-ID UI_INSPECTOR_SCROLL. */
#define UI_INSPECTOR_SCROLL 0x10004u
/** @brief Interne Widget-ID UI_SCRIPT_VSCROLL. */
#define UI_SCRIPT_VSCROLL 0x10005u
/** @brief Interne Widget-ID UI_SCRIPT_HSCROLL. */
#define UI_SCRIPT_HSCROLL 0x10006u
/** @brief Interne Widget-ID UI_SCRIPT_DOCUMENT. */
#define UI_SCRIPT_DOCUMENT 0x10007u
/** @brief Interne Widget-ID UI_MAP_VIEW. */
#define UI_MAP_VIEW 0x10008u
/** @brief Interne Widget-ID UI_SCRIPT_VIEW. */
#define UI_SCRIPT_VIEW 0x10009u
/** @brief Interne Widget-ID UI_EXPLORER_TAB. */
#define UI_EXPLORER_TAB 0x1000au
/** @brief Interne Widget-ID UI_SCENE_TAB. */
#define UI_SCENE_TAB 0x1000bu
/** @brief Interne Widget-ID UI_INSPECTOR_TAB. */
#define UI_INSPECTOR_TAB 0x1000cu
/** @brief Interne Widget-ID UI_INFO_TAB. */
#define UI_INFO_TAB 0x1000du
/** @brief Interne Widget-ID UI_SCENE_SCROLL. */
#define UI_SCENE_SCROLL 0x1000eu
/** @brief Interne Widget-ID UI_INFO_SCROLL. */
#define UI_INFO_SCROLL 0x1000fu
/** @brief Stabile ID des linken Map-Tabs. */
#define UI_MAP_TAB 0x10020u
/** @brief Stabile ID des rechten Tiles-Tabs. */
#define UI_TILES_TAB 0x10021u
/** @brief Scrollbar-ID der Ebenenverwaltung. */
#define UI_MAP_SCROLL 0x10022u
/** @brief Scrollbar-ID der Tileset-Verwaltung. */
#define UI_TILES_SCROLL 0x10023u

/** @brief Serialisierbarer Zustand einer YAML-Tabgruppe oder eines klappbaren Bereichs. */
typedef struct {
    UiId id;                 ///< Stabile Widget- oder Editor-ID.
    UiId selected;           ///< Stabile ID der aktuellen Auswahl; 0 bedeutet keine Auswahl.
    UiId order[UI_MAX_TABS]; ///< Stabile Tab-IDs in aktueller Reihenfolge.
    int count;               ///< Anzahl gültiger Einträge.
    int open;                ///< Aufklappzustand eines YAML-Abschnitts.
    int dragging;            ///< Gespeicherter Dragindex der Tabgruppe.
    float scroll;            ///< Horizontaler Tab-Scrolloffset in Pixeln.
    float extent;            ///< Gespeicherte Größe des zweiten Split-Panels.
    int kind;                ///< 0 für Tabs/Abschnitte, 1 für Split-Container.
} WidgetState;

/** @brief Private Anwendungsinstanz; höchstens Map oder Scripts besitzt gleichzeitig ein geladenes
 * Dokument. */
typedef struct {
    bool window_collapsed;       ///< Nur die Titelleiste ist sichtbar.
    bool window_was_maximized;   ///< Maximierung vor dem Wegklappen.
    int window_restore_width;    ///< Fensterbreite vor dem Wegklappen.
    int window_restore_height;   ///< Fensterhöhe vor dem Wegklappen.
    SDL_Window *window;          ///< Besitzendes Hauptfenster; vor SDL_Quit zerstören.
    SDL_Renderer *renderer;      ///< Besitzender SDL-Renderer; nach seinen Texturen freigeben.
    SDL_Cursor *vertical_cursor; ///< Besitzender Cursor für vertikale Größenänderung.
    PeConsole *console;          ///< Gemeinsame Shell; unabhängig vom geladenen Editor.
    bool layout_resize_x;        ///< Horizontaler YAML-Splitter unter dem Zeiger.
    bool layout_resize_y;        ///< Vertikaler YAML-Splitter unter dem Zeiger.
    SDL_Cursor *resize_cursor;   ///< Besitzender SDL-Cursor für Spaltengrenzen.
    UiContext ui;                ///< Gemeinsamer Eingabe-, Fokus- und Clipzustand.
    UiId widget_scope;           ///< ID-Namensraum des aktuell gezeichneten YAML-Knotens.
    UiTabs workspaces;           ///< Globale Editor-Navigation.
    UiTabs left_tabs;            ///< Linke kontextabhängige Tabgruppe.
    UiTabs right_tabs;           ///< Rechte kontextabhängige Tabgruppe.
    UiScroll tree_view;          ///< Scrollzustand des Projektbaums.
    UiScroll map_view;           ///< Unabhängiger Scrollzustand der Ebenenverwaltung.
    UiScroll tiles_view;         ///< Unabhängiger Scrollzustand der Tileset-/Tile-Verwaltung.
    UiScroll inspector_view;     ///< Scrollzustand des Inspectors.
    UiScroll scene_view;         ///< Scrollzustand der Entity-Liste.
    UiScroll info_view;          ///< Scrollzustand der Szeneninformationen.
    float left_width;            ///< Breite der linken Spalte in Pixeln.
    float right_width;           ///< Breite der rechten Spalte in Pixeln.
    bool layers_open;            ///< Aufklappzustand der Ebenenliste.
    bool palette_open;           ///< Aufklappzustand der Tile-Palette.
    MapPayload *map;             ///< Besitzende Mapinstanz oder NULL bei entladenem Map-Editor.
    PeScriptEditor *script; ///< Besitzende Scriptinstanz oder NULL bei entladenem Script-Editor.
    UiId loaded_view;       ///< ID der tatsächlich geladenen Editorinstanz.
    char session_paths[2][1200];  ///< Temporäre Checkpointpfade für Map und Scripts.
    bool session_exists[2];       ///< Ob für den jeweiligen Editor ein Checkpoint vorliegt.
    bool session_dirty[2];        ///< Änderungszustand des jeweils entladenen Editors.
    PeUiLayout *shell_layout;     ///< Besitzendes globales YAML-Shellmodell.
    PeUiLayout *active_layout;    ///< Besitzendes YAML-Modell des geladenen Editors.
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
    UiId pending_view;  ///< Nach dem Frame zu aktivierende Editor-ID; 0 bedeutet keine.
    Request deferred_request; ///< Nach dem Frame auszuführende Datei-/Beendenanfrage.
    char deferred_path[1024]; ///< Kopierter Pfad der aufgeschobenen Anfrage.
    PeCamera camera;          ///< Mapkamera in Weltpixeln.
    char root[768];           ///< Besitzender Modellbaum bzw. kopierter Projektwurzelpfad.
    char scene_path[1024];    ///< Pfad der aktuell bearbeiteten Szenendatei.
    char status[256];         ///< Kopierte Statusmeldung.
    int width;                ///< Fensterbreite in Pixeln.
    int height;               ///< Fensterhöhe in Pixeln.
    int layer;                ///< Index der aktiven Tileebene, nullbasiert.
    int tile;                 ///< Ausgewählter Tilewert für Map-Werkzeuge.
    int tool;                 ///< Index des ausgewählten Mapwerkzeugs.
    int palette_scroll;       ///< Scrolloffset der Tile-Palette.
    int tree_scroll;          ///< Index des obersten Projektbaumeintrags.
    bool playing;             ///< Ob die getrennte Play-Szene aktualisiert wird.
    bool grid;                ///< Ob das Mapraster sichtbar ist.
    bool collision;           ///< Ob Kollisionen bearbeitet bzw. angezeigt werden.
    bool running;             ///< Ob die Anwendungshauptschleife weiterläuft.
    bool smoke;               ///< Ob der kurze headless Prüfmodus aktiv ist.
    bool painting;            ///< Ob eine Map-Malgeste läuft.
    bool panning;             ///< Ob die Kamera per mittlerer Maustaste verschoben wird.
    bool dragging;            ///< Zustand der laufenden Ziehinteraktion.
    int start_x;              ///< Horizontale Tileposition beim Beginn einer Mapgeste.
    int start_y;              ///< Vertikale Tileposition beim Beginn einer Mapgeste.
    int last_x;               ///< Letzte horizontale Tileposition einer Pinselgeste.
    int last_y;               ///< Letzte vertikale Tileposition einer Pinselgeste.
    float drag_x; ///< Horizontaler Abstand zwischen Zeiger und ausgewähltem Objekt in Weltpixeln.
    float drag_y; ///< Vertikaler Abstand zwischen Zeiger und ausgewähltem Objekt in Weltpixeln.
    uint32_t selected;  ///< Stabile ID der ausgewählten Entity; 0 bedeutet keine.
    float mx;           ///< Aktuelle horizontale Zeigerposition in Fensterpixeln.
    float my;           ///< Aktuelle vertikale Zeigerposition in Fensterpixeln.
    bool click;         ///< Noch zu verarbeitender UI-Klick.
    SDL_FRect viewport; ///< Sichtbarer Inhaltsbereich bzw. dessen Ausdehnung in Pixeln.
    Field field;        ///< Aktives modales oder direktes Eingabefeld.
    char input[1024];   ///< Nullterminierter Eingabepuffer; erst Bestätigung ändert das Dokument.
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
    double accumulator;         ///< Noch zu simulierende Playzeit in Sekunden.
    Uint64 last_time;           ///< Zeitpunkt des vorigen Frames in SDL-Nanosekunden.
} Editor;

/** @brief Registrierter Editoradapter mit stabiler ID und Zeichenrückrufen. */
typedef struct {
    UiId id;                            ///< Stabile Widget- oder Editor-ID.
    const char *label;                  ///< Geliehene, nullterminierte Beschriftung.
    UiIcon icon;                        ///< Eingebautes Editorsymbol.
    int action_count;                   ///< Anzahl der kontextabhängigen Werkzeugaktionen.
    void (*draw_tools)(SDL_FRect area); ///< Rückruf zum Zeichnen der Editor-Werkzeugleiste.
    void (*draw_content)(void);         ///< Rückruf zum Zeichnen des Editorinhalts.
} EditorView;

/* Private integration state. Engine and reusable UI never include this header. */
/** @brief Private gemeinsame Editor-Anwendungsinstanz. */
extern Editor app;

/** @brief Registrierte Editoradapter; Reihenfolge ist keine Dokumentidentität. */
extern const EditorView editor_views[];

/** @brief Anzahl registrierter Editoradapter. */
extern const int editor_view_count;

/** @brief Geliehener Tooltipzeiger für den aktuellen Frame. */
extern const char *editor_tooltip;

/** @brief Geliehener optionaler BMP-Ausgabepfad aus den Programmargumenten. */
extern const char *editor_screenshot_path;

/**
 * @brief Prüft, ob eine Map-Transaktion noch offen ist.
 */
bool editor_edit_pending(void);

/**
 * @brief Liefert die registrierte Editorbeschreibung; unbekannte IDs fallen auf Map zurück.
 */
const EditorView *editor_view(UiId id);

/**
 * @brief Prüft die ID des tatsächlich geladenen Editors auf Map.
 */
bool editor_map_active(void);

/**
 * @brief Liest den Map-Änderungszustand aus Dokument oder entladenem Sitzungsstatus.
 */
bool editor_map_dirty(void);

/**
 * @brief Kopiert eine Statusmeldung in Shell und gegebenenfalls aktive Scriptansicht.
 */
void editor_message(const char *s);

/**
 * @brief Liest den Script-Änderungszustand aus Dokument oder entladenem Sitzungsstatus.
 */
bool editor_script_dirty(void);

/**
 * @brief Prüft die ID des tatsächlich geladenen Editors auf Scripts.
 */
bool editor_scripts_active(void);

/* File identity prevents equal line numbers in different modules sharing IDs. */
/** @brief Erzeugt eine auf Dateiname und Zeile basierende interne Button-ID. */
#define icon_button(...) editor_icon_button_impl(ui_id(__FILE__) ^ (UiId)__LINE__, __VA_ARGS__)
/** @brief Erzeugt eine auf Dateiname und Zeile basierende interne Button-ID. */
#define button(...) editor_button_impl(ui_id(__FILE__) ^ (UiId)__LINE__, __VA_ARGS__)

/** @} */
#endif
