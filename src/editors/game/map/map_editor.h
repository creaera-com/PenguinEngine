/**
 * @file
 * @brief Private Modulverträge für src/editors/game/map.
 * @ingroup editor_api
 */

/** @addtogroup editor_api
 * @{
 */
#ifndef PENGUIN_MAP_EDITOR_H
#define PENGUIN_MAP_EDITOR_H

#include "editors/game/map/view_state.h"
#include "penguin/render.h"

/** @brief Aktives Mapwerkzeug; Reihenfolge entspricht der registrierten Werkzeugauswahl. */
typedef enum { BRUSH, ERASE, RECTANGLE, FILL, SELECT } Tool;

/** @brief Besitzende Mapdaten: bearbeitete Szene, getrennte Playkopie, Verlauf und Texturcache. */
typedef struct MapPayload {
    PeMapViewState view; ///< Ausschließlich von dieser Map besessene Navigation und Gesten.
    PeScene scene;       ///< Bearbeitete Map-Szene.
    PeScene runtime;     ///< Separate Playkopie; verändert die bearbeitete Szene nicht.
    PeHistory history;   ///< Besitzender Szenen-Undo-/Redo-Verlauf.
    PeAssets assets;     ///< Besitzender Tilesetcache der geladenen Map.
    uint32_t
        collapsed[PE_ENTITIES]; ///< Stabile IDs zugeklappter Scene-Knoten; keine Dokumentdaten.
    uint32_t
        component_ids[PE_ENTITIES]; ///< IDs zur Zuordnung der Inspector-Navigation zu Entity-Slots.
    uint32_t component_closed[PE_ENTITIES]; ///< Bits zugeklappter Komponenten; keine Dokumentdaten.
    bool component_add_open;                ///< Flüchtiger Zustand der Hinzufügen-Auswahl.
    SDL_FRect component_anchor; ///< Bildschirmbereich des Dropdown-Buttons im aktuellen Frame.
    SDL_FRect component_popup;  ///< Bildschirmbereich des geöffneten Komponentenmenüs.
    uint32_t component_owner;   ///< Entity-ID des geöffneten Dropdowns.
    int component_choice;       ///< Tastatur-/Hoverauswahl im Dropdown.
    int component_pressed;      ///< Beim Drücken ausgewählter Menüeintrag, -1 für keinen.
    bool component_anchor_seen; ///< Dropdown-Anker wurde im aktuellen Frame gezeichnet.
    bool reveal_selected;       ///< Ausgewählte Entity beim nächsten Canvas-Frame sichtbar machen.
    int collapsed_count;        ///< Anzahl gültiger Einträge in collapsed.
} MapPayload;

/* Map-specific operations, used only within the editor. */
/**
 * @brief Lädt das Tileset der aktiven Map oder meldet den Ladefehler.
 */
void editor_assets_sync(void);

/**
 * @brief Beginnt eine Map-Bearbeitungstransaktion und meldet Speicherfehler.
 */
bool editor_begin(void);

/**
 * @brief Schließt die Map-Transaktion ab; bei Speicherfehler wird die Änderung zurückgesetzt.
 */
void editor_commit(void);

/**
 * @brief Zeichnet Szenenstatistik und Map-Tastenkürzel im aktuellen Panel.
 */
void editor_draw_info(void);

/**
 * @brief Zeichnet ausschließlich Auswahl und Komponenten der inspizierten Entity.
 */
void editor_draw_inspector(void);
/** @brief Zeichnet Karteninformationen und Ebenenverwaltung im Map-Tab. */
void editor_draw_map_panel(void);
/** @brief Zeichnet Tileset-Import, Quell-Tilegröße und Tile-Palette im Tiles-Tab. */
void editor_draw_tiles_panel(void);
/**
 * @brief Zeichnet Komponenten-Karten der aktuellen Auswahl und liefert die nächste
 * Inhalts-Y-Position.
 * @param x Linke Position in Fensterpixeln.
 * @param y Obere Position in Fensterpixeln.
 * @param width Verfügbare Breite in Fensterpixeln.
 */
float editor_draw_components(float x, float y, float width);
/** @brief Zeichnet das Komponenten-Dropdown nach den Panels als überlagerndes Menü. */
void editor_draw_component_popup(void);
/**
 * @brief Verarbeitet Dropdown-Ereignisse und verhindert Eingaben in verdeckte Widgets.
 * @param event Geliehenes SDL-Ereignis; wird nicht verändert.
 * @return true, wenn das Menü die Eingabe verbraucht hat.
 */
bool editor_component_popup_event(const SDL_Event *event);
/**
 * @brief Fügt eine Komponente zur Auswahl hinzu oder entfernt sie als Undo-Schritt.
 * @param component Genau ein bekanntes Komponentenbit.
 * @param add true zum Hinzufügen, false zum Entfernen.
 */
bool editor_component_change(uint32_t component, bool add);
/** @brief Öffnet den Entity-Inspector und schafft Platz für seine Komponenten. */
void editor_inspect_entity(void);

/**
 * @brief Zeichnet Map-Canvas, Auswahlrahmen und Werkzeugvorschau.
 */
void editor_draw_map_content(void);

/**
 * @brief Zeichnet die YAML-Werkzeugleiste des Map-Editors.
 */
void editor_draw_map_tools(SDL_FRect area);

/**
 * @brief Zeichnet die Entity-Liste und verarbeitet die Auswahl.
 */
void editor_draw_scene_tree(void);

/**
 * @brief Erstellt eine Entity als Wurzel, Geschwister oder Kind der Auswahl, jeweils als
 * Undo-Schritt.
 * @param relation 0 für Wurzel, 1 für Geschwister, 2 für Kind.
 */
void editor_scene_add(int relation);
/** @brief Löscht die ausgewählte Entity samt Unterbaum als gemeinsamen Undo-Schritt. */
void editor_scene_delete(void);

/**
 * @brief Beendet eine Pinsel-/Auswahlgeste als gemeinsamen Undo-Schritt.
 */
void editor_edit_end(bool erase);

/**
 * @brief Startet das gewählte Map-Werkzeug an einer Bildschirmposition.
 */
void editor_edit_start(float sx, float sy, bool erase);

/**
 * @brief Validiert und übernimmt den aktiven Map-Inspector- oder Importwert.
 */
void editor_map_apply_field(void);

/**
 * @brief Importiert eine PNG-Datei ins Projekt und wechselt das Tileset per Undo-Transaktion.
 */
bool editor_import_png(const char *source);

/**
 * @brief Ersetzt das aktive Map-Dokument nach erfolgreichem Laden einer Szene.
 */
void editor_load_scene(const char *path);

/**
 * @brief Malt eine lückenlose Linie zwischen zwei Tilepositionen.
 */
void editor_paint_line(int x0, int y0, int x1, int y1, bool erase);

/**
 * @brief Startet oder stoppt Play mit einer separaten Laufzeitkopie der Szene.
 */
void editor_play_toggle(void);

/**
 * @brief Speichert das aktive Map-Dokument und aktualisiert dessen Speicherrevision.
 */
bool editor_save_scene(void);

/**
 * @brief Bereinigt Layer-/Entity-Auswahl nach Dokumentänderungen und synchronisiert Assets.
 */
void editor_selection_sync(void);

/**
 * @brief Validiert und übernimmt Tilesetpfad und Quell-Tilegröße als Undo-Schritt.
 */
bool editor_set_tileset(const char *relative, int tile_size);

/**
 * @brief Wandelt Bildschirmpositionen über Viewport und Kamera in Tilekoordinaten um.
 */
void editor_tile_coords(float sx, float sy, int *x, int *y);

/**
 * @brief Verarbeitet Map-Werkzeugkürzel und Entity-Löschen.
 */
void editor_map_key(SDL_Keycode key);

/**
 * @brief Zoomt die Map am aktuellen Zeiger mit einer Mausraddistanz.
 */
void editor_map_zoom(float delta);

/**
 * @brief Verarbeitet Map-Zeigeraktionen unter Beachtung von Modal- und Play-Zustand.
 */
void editor_map_pointer(SDL_Event *event, bool modal);

/**
 * @brief Führt registrierte Map- und Play-Aktionen aus der YAML-Oberfläche aus.
 */
void editor_map_action(const char *name);

/**
 * @brief Aktualisiert die Play-Szene in festen Schritten von 1/60 Sekunde.
 *
 * elapsed ist die bereits begrenzte Framedauer in Sekunden; nur der Play-Modus wird aktualisiert.
 */
void editor_map_update(double elapsed);

/** @} */
#endif
