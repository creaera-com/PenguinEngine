/** @file
 * @brief Besitz- und Zustandsbereich map_view; keine globale Instanz. */
#ifndef PENGUIN_PEMAPVIEWSTATE_H
#define PENGUIN_PEMAPVIEWSTATE_H
#include "penguin/engine.h"
#include "penguin/ui.h"
#include <stdbool.h>

/** @brief Zustand für map_view; Lebensdauer wird vom Anwendungshost koordiniert. */
typedef struct {
    PeCamera camera;     ///< Mapkamera in Weltpixeln.
    UiScroll map_view;   ///< Unabhängiger Scrollzustand der Ebenenverwaltung.
    UiScroll tiles_view; ///< Unabhängiger Scrollzustand der Tileset-/Tile-Verwaltung.
    bool layers_open;    ///< Aufklappzustand der Ebenenliste.
    bool palette_open;   ///< Aufklappzustand der Tile-Palette.
    int layer;           ///< Index der aktiven Tileebene, nullbasiert.
    int tile;            ///< Ausgewählter Tilewert für Map-Werkzeuge.
    int tool;            ///< Index des ausgewählten Mapwerkzeugs.
    int palette_scroll;  ///< Scrolloffset der Tile-Palette.
    bool playing;        ///< Ob die getrennte Play-Szene aktualisiert wird.
    bool grid;           ///< Ob das Mapraster sichtbar ist.
    bool collision;      ///< Ob Kollisionen bearbeitet bzw. angezeigt werden.
    bool painting;       ///< Ob eine Map-Malgeste läuft.
    bool panning;        ///< Ob die Kamera per mittlerer Maustaste verschoben wird.
    bool dragging;       ///< Zustand der laufenden Ziehinteraktion.
    int start_x;         ///< Horizontale Tileposition beim Beginn einer Mapgeste.
    int start_y;         ///< Vertikale Tileposition beim Beginn einer Mapgeste.
    int last_x;          ///< Letzte horizontale Tileposition einer Pinselgeste.
    int last_y;          ///< Letzte vertikale Tileposition einer Pinselgeste.
    float drag_x; ///< Horizontaler Abstand zwischen Zeiger und ausgewähltem Objekt in Weltpixeln.
    float drag_y; ///< Vertikaler Abstand zwischen Zeiger und ausgewähltem Objekt in Weltpixeln.
    double accumulator; ///< Noch zu simulierende Playzeit in Sekunden.
} PeMapViewState;
#endif
