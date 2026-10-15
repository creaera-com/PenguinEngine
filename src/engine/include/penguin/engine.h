/**
 * @file
 * @brief Öffentliche, SDL-unabhängige Szenen-, ECS-, Tilemap-, Kamera- und Laufzeitschnittstelle.
 * @ingroup engine_api
 */

/** @addtogroup engine_api
 * @{
 */
#ifndef PENGUIN_ENGINE_H
#define PENGUIN_ENGINE_H
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

/** @brief Maximale Entity-Slotanzahl pro Szene. */
#define PE_ENTITIES 128
/** @brief Maximale Anzahl visueller Tileebenen. */
#define PE_LAYERS 4
/** @brief Maximale Kartenkante in Tiles; zugleich Zeilenstride der festen Tile-/Kollisionspuffer.
 */
#define PE_MAP_MAX 64
/** @brief Kapazität fester Projekt-/Assetpfade in Bytes einschließlich Nullabschluss. */
#define PE_PATH 256
/** @brief Maximale Anzahl gespeicherter Szenen-Undo-Schritte. */
#define PE_HISTORY 64
/** @brief Komponentenbit PE_TRANSFORM. */
#define PE_TRANSFORM 1u
/** @brief Komponentenbit PE_SPRITE. */
#define PE_SPRITE 2u
/** @brief Komponentenbit PE_BODY. */
#define PE_BODY 4u
/** @brief Komponentenbit PE_PLAYER. */
#define PE_PLAYER 8u

/** @brief Laufzeitkennung aus Slot und Generation; keine persistente Objekt-ID. */
typedef struct {
    uint32_t index;      ///< Entity-Slotindex; UINT32_MAX kennzeichnet eine ungültige Kennung.
    uint32_t generation; ///< Slotgeneration zur Erkennung veralteter Entity-Kennungen.
} PeEntity;

/** @brief Feste ECS-Komponentendaten eines Entity-Slots; mask bestimmt die aktiven Komponenten. */
typedef struct {
    float x;        ///< Horizontale lokale Position relativ zum Parent; ohne Parent Weltposition.
    float y;        ///< Vertikale lokale Position relativ zum Parent; ohne Parent Weltposition.
    float w;        ///< Breite.
    float h;        ///< Höhe.
    float vx;       ///< Horizontale Geschwindigkeit in Weltpixeln pro Sekunde.
    float vy;       ///< Vertikale Geschwindigkeit in Weltpixeln pro Sekunde.
    uint32_t color; ///< Gepackte Darstellungsfarbe im Format 0xRRGGBBAA.
    uint32_t mask;  ///< Bitmaske der aktiven Komponenten.
    uint32_t stable_id; ///< Im Szenenformat gespeicherte Objekt-ID.
    uint32_t
        parent_id; ///< Stabile ID des Parents; 0 bezeichnet eine Wurzel. Über Parent-API ändern.
    uint32_t generation; ///< Slotgeneration zur Erkennung veralteter Entity-Kennungen.
    bool alive;          ///< Ob dieser Entity-Slot belegt ist.
} PeObject;

/** @brief Benannte visuelle Tileebene mit festem Zeilenstride PE_MAP_MAX. */
typedef struct {
    char name[32];                           ///< Nullterminierter Anzeigename.
    bool visible;                            ///< Ob die visuelle Ebene gezeichnet wird.
    uint16_t tiles[PE_MAP_MAX * PE_MAP_MAX]; ///< Tilewerte mit Zeilenstride PE_MAP_MAX; 0
                                             ///< bezeichnet ein leeres Tile.
} PeLayer;

/** @brief Besitzendes Szenenmodell mit festen Kapazitäten; Kollision ist unabhängig von visuellen
 * Ebenen. */
typedef struct {
    int width;                 ///< Kartenbreite in Tiles; höchstens PE_MAP_MAX.
    int height;                ///< Kartenhöhe in Tiles; höchstens PE_MAP_MAX.
    int tile_size;             ///< Größe eines Welt-Tiles in Pixeln.
    int layer_count;           ///< Anzahl aktiver visueller Ebenen.
    char tileset[PE_PATH];     ///< Projektbezogener, nullterminierter Pfad zur Tilesetdatei.
    int tileset_tile_size;     ///< Quell-Tilegröße im Tilesetbild in Pixeln.
    PeLayer layers[PE_LAYERS]; ///< Fest reservierte visuelle Ebenen.
    uint8_t
        solid[PE_MAP_MAX * PE_MAP_MAX]; ///< Separates Kollisionsraster mit Zeilenstride PE_MAP_MAX.
    PeObject objects[PE_ENTITIES];      ///< Fest reservierte Entity-Slots.
    uint32_t next_id;                   ///< Nächste stabile Objekt-ID.
} PeScene;

/** @brief Kameraursprung in Weltpixeln und dimensionsloser Zoomfaktor. */
typedef struct {
    float x;    ///< Weltpixel am linken Viewportrand.
    float y;    ///< Weltpixel am oberen Viewportrand.
    float zoom; ///< Dimensionsloser Zoomfaktor; regulär zwischen 0,25 und 8.
} PeCamera;

/** @brief Besitzendes Vorher-/Nachher-Snapshotpaar einer Szenenänderung. */
typedef struct {
    PeScene *before;          ///< Besitzender Dokument-Snapshot vor der Änderung.
    PeScene *after;           ///< Besitzender Dokument-Snapshot nach der Änderung.
    uint64_t before_revision; ///< Revision vor der Änderung.
    uint64_t after_revision;  ///< Revision nach der Änderung.
} PeCommand;

/** @brief Begrenzter Szenenverlauf mit Speicherrevision und höchstens einer offenen Transaktion. */
typedef struct {
    PeCommand commands[PE_HISTORY]; ///< Besitzende Szenen-Undo-Schritte.
    int count;                      ///< Anzahl gültiger Einträge.
    int cursor;                     ///< Position im Änderungsverlauf bzw. UTF-8-Byteoffset im Text.
    PeScene *pending;        ///< Besitzender Vorher-Snapshot der offenen Transaktion oder NULL.
    uint64_t revision;       ///< Aktuelle Dokumentrevision.
    uint64_t next_revision;  ///< Nächste eindeutige Änderungsrevision.
    uint64_t saved_revision; ///< Zuletzt als gespeichert markierte Revision.
} PeHistory;

/**
 * @brief Initialisiert eine leere Szene mit einer sichtbaren Ground-Ebene und Entity-Generationen.
 *
 * Ungültige Größen verwenden 16 Tiles bzw. 16 Pixel. Eine Szene besitzt keine dynamischen
 * Ressourcen.
 *
 * @param scene Szenenmodell; muss während des Aufrufs gültig sein.
 * @param width Breite in den von der Funktion beschriebenen Einheiten.
 * @param height Höhe in den von der Funktion beschriebenen Einheiten.
 * @param tile_size Tilekantenlänge in Pixeln.
 */
void pe_scene_init(PeScene *scene, int width, int height, int tile_size);

/**
 * @brief Belegt einen freien Entity-Slot mit der angegebenen Komponentenmaske.
 *
 * @return Eine gültige Kennung oder {UINT32_MAX, 0}, falls kein Slot frei ist.
 *
 * @param scene Szenenmodell; muss während des Aufrufs gültig sein.
 * @param mask Bitmaske der gewünschten Komponenten.
 */
PeEntity pe_entity_create(PeScene *scene, uint32_t mask);

/**
 * @brief Prüft Slot, Lebensstatus und Generation einer Laufzeitkennung.
 *
 * @param scene Szenenmodell; muss während des Aufrufs gültig sein.
 * @param entity Laufzeitkennung mit Slot und Generation.
 */
bool pe_entity_valid(const PeScene *scene, PeEntity entity);

/**
 * @brief Entfernt eine Entity samt Unterbaum und erhöht die Generationen aller entfernten Slots.
 *
 * @param scene Szenenmodell; muss während des Aufrufs gültig sein.
 * @param entity Laufzeitkennung mit Slot und Generation.
 */
bool pe_entity_destroy(PeScene *scene, PeEntity entity);

/**
 * @brief Liefert einen geliehenen Objektzeiger oder NULL für eine ungültige Kennung.
 *
 * @param scene Szenenmodell; muss während des Aufrufs gültig sein.
 * @param entity Laufzeitkennung mit Slot und Generation.
 */
PeObject *pe_entity_get(PeScene *scene, PeEntity entity);

/**
 * @brief Sucht eine lebende Entity anhand ihrer gespeicherten stabilen ID.
 *
 * @return Die gefundene Kennung oder {UINT32_MAX, 0}.
 *
 * @param scene Szenenmodell; muss während des Aufrufs gültig sein.
 * @param stable_id Gespeicherte Objekt-ID, keine Slotnummer.
 */
PeEntity pe_entity_find(const PeScene *scene, uint32_t stable_id);

/**
 * @brief Berechnet die Weltposition aus lokalen Translationen der Elternkette; false bei Fehlern.
 * @param scene Geliehene Szene; bleibt unverändert.
 * @param entity Gültige Entity mit PE_TRANSFORM.
 * @param x Ausgabe der Welt-X-Position; nur bei Erfolg beschrieben.
 * @param y Ausgabe der Welt-Y-Position; nur bei Erfolg beschrieben.
 */
bool pe_entity_world_position(const PeScene *scene, PeEntity entity, float *x, float *y);

/**
 * @brief Setzt die Weltposition durch Umrechnung in die lokale Position; Kinder folgen automatisch.
 * @param scene Zu ändernde Szene.
 * @param entity Gültige Entity mit PE_TRANSFORM.
 * @param x Endliche Welt-X-Position.
 * @param y Endliche Welt-Y-Position.
 * @return false ohne Änderung bei ungültiger Hierarchie oder Position.
 */
bool pe_entity_set_world_position(PeScene *scene, PeEntity entity, float x, float y);

/**
 * @brief Ändert den Parent atomar; lehnt Zyklen, veraltete Kennungen und fehlende Transforms ab.
 * @param scene Zu ändernde Szene.
 * @param entity Zu verschiebende Entity mit PE_TRANSFORM.
 * @param parent Neue Parent-Entity mit PE_TRANSFORM; {UINT32_MAX, 0} löst die Beziehung.
 * @param keep_world true erhält die Weltposition; false erhält die lokale Position.
 */
bool pe_entity_set_parent(PeScene *scene, PeEntity entity, PeEntity parent, bool keep_world);

/**
 * @brief Prüft IDs, Elternreferenzen, Transform-Voraussetzungen, Zyklen und endliche Positionen.
 * @param scene Geliehene Szene; bleibt unverändert.
 */
bool pe_scene_hierarchy_valid(const PeScene *scene);

/**
 * @brief Fügt eine Komponente mit Standardwerten und nötigen Abhängigkeiten hinzu.
 * @param scene Zu ändernde Szene.
 * @param entity Gültige Entity.
 * @param component Genau eines der Bits PE_TRANSFORM, PE_SPRITE, PE_BODY oder PE_PLAYER.
 * @return false ohne Änderung bei ungültigem Bit, ungültiger Entity oder bereits vorhandener
 * Komponente.
 */
bool pe_entity_add_component(PeScene *scene, PeEntity entity, uint32_t component);

/**
 * @brief Prüft, ob eine Komponente ohne Verletzung von Abhängigkeiten/Hierarchie entfernbar ist.
 * @param scene Geliehene Szene.
 * @param entity Gültige Entity.
 * @param component Genau ein bekanntes Komponentenbit.
 */
bool pe_entity_component_removable(const PeScene *scene, PeEntity entity, uint32_t component);

/**
 * @brief Entfernt eine Komponente; abhängige Komponenten und Hierarchien bleiben geschützt.
 * @param scene Zu ändernde Szene.
 * @param entity Gültige Entity.
 * @param component Genau ein bekanntes Komponentenbit.
 * @return false ohne Änderung, wenn pe_entity_component_removable false liefert.
 */
bool pe_entity_remove_component(PeScene *scene, PeEntity entity, uint32_t component);

/**
 * @brief Setzt ein Tile auf einer gültigen Ebene und Kartenposition.
 *
 * @param scene Szenenmodell; muss während des Aufrufs gültig sein.
 * @param layer Nullbasierter Index einer aktiven Tileebene.
 * @param x Horizontale Tileposition.
 * @param y Vertikale Tileposition.
 * @param tile Tilewert; 0 bezeichnet ein leeres Tile.
 */
bool pe_tile_set(PeScene *scene, int layer, int x, int y, uint16_t tile);

/** @brief Liest ein Tile; ungültige Positionen und Ebenen liefern 0. *
 * @param scene Szenenmodell; muss während des Aufrufs gültig sein.
 * @param layer Nullbasierter Index einer aktiven Tileebene.
 * @param x Horizontale Tileposition.
 * @param y Vertikale Tileposition.
 */
uint16_t pe_tile_get(const PeScene *scene, int layer, int x, int y);

/**
 * @brief Füllt zusammenhängende Tiles desselben Werts über vier Nachbarn.
 *
 * @param scene Szenenmodell; muss während des Aufrufs gültig sein.
 * @param layer Nullbasierter Index einer aktiven Tileebene.
 * @param x Horizontale Tileposition.
 * @param y Vertikale Tileposition.
 * @param tile Tilewert; 0 bezeichnet ein leeres Tile.
 */
bool pe_fill(PeScene *scene, int layer, int x, int y, uint16_t tile);

/**
 * @brief Füllt ein inklusives Rechteck und beschränkt es auf die Kartengrenzen.
 *
 * @param scene Szenenmodell; muss während des Aufrufs gültig sein.
 * @param layer Nullbasierter Index einer aktiven Tileebene.
 * @param x0 Horizontale erste Tileposition.
 * @param y0 Vertikale erste Tileposition.
 * @param x1 Horizontale zweite Tileposition, inklusive.
 * @param y1 Vertikale zweite Tileposition, inklusive.
 * @param tile Tilewert; 0 bezeichnet ein leeres Tile.
 */
void pe_rectangle(PeScene *scene, int layer, int x0, int y0, int x1, int y1, uint16_t tile);

/**
 * @brief Initialisiert einen leeren Szenenverlauf; bereits belegte Verläufe vorher freigeben.
 *
 * @param history Besitzender, initialisierter Szenenverlauf.
 */
void pe_history_init(PeHistory *history);

/**
 * @brief Gibt alle Snapshots und eine offene Transaktion frei und setzt den Verlauf zurück.
 *
 * @param history Besitzender, initialisierter Szenenverlauf.
 */
void pe_history_free(PeHistory *history);

/**
 * @brief Legt einen Vorher-Snapshot an; erlaubt nur eine offene Transaktion.
 *
 * @return false bei offener Transaktion oder Speichermangel; die Szene bleibt unverändert.
 *
 * @param history Besitzender, initialisierter Szenenverlauf.
 * @param scene Szenenmodell; muss während des Aufrufs gültig sein.
 */
bool pe_history_begin(PeHistory *history, const PeScene *scene);

/**
 * @brief Schließt die Transaktion ab und speichert eine tatsächliche Änderung im Verlauf.
 *
 * @return true auch für unveränderte Szenen; bei Speichermangel false mit wiederhergestellter
 * Szene.
 *
 * @param history Besitzender, initialisierter Szenenverlauf.
 * @param scene Szenenmodell; muss während des Aufrufs gültig sein.
 */
bool pe_history_commit(PeHistory *history, PeScene *scene);

/**
 * @brief Stellt den Vorher-Zustand einer offenen Transaktion wieder her.
 *
 * @param history Besitzender, initialisierter Szenenverlauf.
 * @param scene Szenenmodell; muss während des Aufrufs gültig sein.
 */
void pe_history_cancel(PeHistory *history, PeScene *scene);

/**
 * @brief Stellt den vorherigen Szenenzustand einschließlich Revisionsnummer wieder her.
 *
 * @param history Besitzender, initialisierter Szenenverlauf.
 * @param scene Szenenmodell; muss während des Aufrufs gültig sein.
 */
bool pe_undo(PeHistory *history, PeScene *scene);

/**
 * @brief Stellt den nächsten zuvor rückgängig gemachten Szenenzustand wieder her.
 *
 * @param history Besitzender, initialisierter Szenenverlauf.
 * @param scene Szenenmodell; muss während des Aufrufs gültig sein.
 */
bool pe_redo(PeHistory *history, PeScene *scene);

/**
 * @brief Markiert die aktuelle Revision als gespeichert.
 *
 * @param history Besitzender, initialisierter Szenenverlauf.
 */
void pe_history_saved(PeHistory *history);

/**
 * @brief Prüft auf eine abweichende Speicherrevision oder eine offene Transaktion.
 *
 * @param history Besitzender, initialisierter Szenenverlauf.
 */
bool pe_history_dirty(const PeHistory *history);

/**
 * @brief Zoomt zwischen 0,25 und 8 und hält den Weltpunkt unter dem Bildschirmanker fest.
 *
 * @param camera Kamera mit gültigem positivem Zoom.
 * @param screen_x Horizontaler Zoomanker relativ zum Viewport in Pixeln.
 * @param screen_y Vertikaler Zoomanker relativ zum Viewport in Pixeln.
 * @param factor Multiplikativer Zoomfaktor.
 */
void pe_camera_zoom(PeCamera *camera, float screen_x, float screen_y, float factor);

/**
 * @brief Wandelt viewport-relative Bildschirmkoordinaten in Weltkoordinaten um.
 *
 * @param camera Kamera mit gültigem positivem Zoom.
 * @param sx Horizontale Position relativ zum Viewport in Pixeln.
 * @param sy Vertikale Position relativ zum Viewport in Pixeln.
 * @param wx Ausgabe der horizontalen Weltposition in Pixeln.
 * @param wy Ausgabe der vertikalen Weltposition in Pixeln.
 */
void pe_screen_to_world(const PeCamera *camera, float sx, float sy, float *wx, float *wy);
/* Rectangle against collision grid, including world boundaries. */
/**
 * @brief Prüft ein Rechteck gegen das Kollisionsraster und die Weltgrenzen.
 *
 * Ungültige oder nichtendliche Rechtecke gelten als Kollision. Koordinaten und Größen sind
 * Weltpixel.
 *
 * @param scene Szenenmodell; muss während des Aufrufs gültig sein.
 * @param x Horizontale Position in Pixeln.
 * @param y Vertikale Position in Pixeln.
 * @param w Rechteckbreite in Weltpixeln.
 * @param h Rechteckhöhe in Weltpixeln.
 */
bool pe_collides(const PeScene *scene, float x, float y, float w, float h);

/**
 * @brief Bewegt ein Objekt mit Teilbewegungen und achsenweiser Kollisionsprüfung.
 *
 * Verschiebungen sind Weltpixel. Nichtendliche Werte oder mehr als 4096 Teilschritte werden
 * verworfen.
 *
 * @param scene Szenenmodell; muss während des Aufrufs gültig sein.
 * @param object Geliehener Modellknoten bzw. zu bewegendes Objekt.
 * @param dx Horizontale Verschiebung in Weltpixeln.
 * @param dy Vertikale Verschiebung in Weltpixeln.
 */
void pe_move(PeScene *scene, PeObject *object, float dx, float dy);

/**
 * @brief Aktualisiert bewegliche Entities und die normalisierte Spielersteuerung.
 *
 * input_x/input_y liegen effektiv in [-1,1]. dt ist in Sekunden; nur 0 < dt <= 0.1 wird
 * verarbeitet.
 *
 * @param scene Szenenmodell; muss während des Aufrufs gültig sein.
 * @param input_x Horizontale Spieler-Eingabe; wird auf [-1,1] begrenzt.
 * @param input_y Vertikale Spieler-Eingabe; wird auf [-1,1] begrenzt.
 * @param dt Zeitschritt in Sekunden, 0 < dt <= 0.1.
 */
void pe_update(PeScene *scene, float input_x, float input_y, float dt);
/* On failure load leaves destination unchanged. Errors are caller-owned buffers. */
/**
 * @brief Speichert eine Szene über eine temporäre JSON-Datei und ersetzt das Ziel bei Erfolg.
 *
 * @param scene Szenenmodell; muss während des Aufrufs gültig sein.
 * @param path Nullterminierter Dateipfad.
 * @param error Aufruferpuffer für eine Fehlermeldung; NULL ist bei Größe 0 erlaubt.
 * @param size Kapazität des zugehörigen Ausgabepuffers in Bytes einschließlich Nullabschluss.
 */
bool pe_scene_save(const PeScene *scene, const char *path, char *error, size_t size);

/**
 * @brief Lädt und validiert eine JSON-Szene; bei Fehler bleibt die Zielszene unverändert.
 *
 * @param scene Szenenmodell; muss während des Aufrufs gültig sein.
 * @param path Nullterminierter Dateipfad.
 * @param error Aufruferpuffer für eine Fehlermeldung; NULL ist bei Größe 0 erlaubt.
 * @param size Kapazität des zugehörigen Ausgabepuffers in Bytes einschließlich Nullabschluss.
 */
bool pe_scene_load(PeScene *scene, const char *path, char *error, size_t size);
/* Explicit roots are authoritative. Otherwise resolve beside the executable, then cwd.
   executable_directory may be NULL. No SDL dependency; output is caller-owned. */
/**
 * @brief Ermittelt ein Projekt aus explizitem Pfad, Programmverzeichnis oder Arbeitsverzeichnis.
 *
 * Ein expliziter Pfad ist verbindlich. executable_directory darf NULL sein. Ausgabepuffer gehören
 * dem Aufrufer.
 *
 * @param explicit_root Verbindlicher Projektpfad oder NULL für die automatische Suche.
 * @param executable_directory Programmverzeichnis oder NULL.
 * @param root Projektwurzelpfad bzw. Aufruferpuffer für den ermittelten Pfad.
 * @param size Kapazität des zugehörigen Ausgabepuffers in Bytes einschließlich Nullabschluss.
 * @param error Aufruferpuffer für eine Fehlermeldung; NULL ist bei Größe 0 erlaubt.
 * @param error_size Kapazität von error in Bytes einschließlich Nullabschluss.
 */
bool pe_project_resolve(const char *explicit_root, const char *executable_directory, char *root,
                        size_t size, char *error, size_t error_size);

/**
 * @brief Lädt das Projektmanifest und bildet den Pfad zur Startszene.
 *
 * @param root Projektwurzelpfad bzw. Aufruferpuffer für den ermittelten Pfad.
 * @param scene_path Aufruferpuffer für den vollständigen Start-Szenenpfad.
 * @param size Kapazität des zugehörigen Ausgabepuffers in Bytes einschließlich Nullabschluss.
 * @param error Aufruferpuffer für eine Fehlermeldung; NULL ist bei Größe 0 erlaubt.
 * @param error_size Kapazität von error in Bytes einschließlich Nullabschluss.
 */
bool pe_project_load(const char *root, char *scene_path, size_t size, char *error,
                     size_t error_size);
/** @} */
#endif
