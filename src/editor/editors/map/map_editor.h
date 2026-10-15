/**
 * @file
 * @brief Private Modulverträge für src/editor/editors/map.
 * @ingroup editor_api
 */

/** @addtogroup editor_api
 * @{
 */
#ifndef PENGUIN_MAP_EDITOR_H
#define PENGUIN_MAP_EDITOR_H

#include "penguin/render.h"

/** @brief Aktives Mapwerkzeug; Reihenfolge entspricht der registrierten Werkzeugauswahl. */
typedef enum { BRUSH, ERASE, RECTANGLE, FILL, SELECT } Tool;

/** @brief Besitzende Mapdaten: bearbeitete Szene, getrennte Playkopie, Verlauf und Texturcache. */
typedef struct {
    PeScene scene;     ///< Bearbeitete Map-Szene.
    PeScene runtime;   ///< Separate Playkopie; verändert die bearbeitete Szene nicht.
    PeHistory history; ///< Besitzender Szenen-Undo-/Redo-Verlauf.
    PeAssets assets;   ///< Besitzender Tilesetcache der geladenen Map.
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
 * @brief Zeichnet Map-Inspector, Ebenenliste, Entity-Werte und Tile-Palette.
 */
void editor_draw_inspector(void);

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
void editor_field_apply(void);

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
