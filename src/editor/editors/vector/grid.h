/** @file
 * @brief Begrenzte, SDL-unabhängige Grid-Geometrie in lokalen Artboard-Pixeln. */
#ifndef PENGUIN_VECTOR_GRID_H
#define PENGUIN_VECTOR_GRID_H
#include "editors/vector/document.h"
#include "editors/vector/preview.h"

/** @brief Auf den sichtbaren Bereich beschnittene Grid-Linie. */
typedef struct {
    VePoint a; ///< Anfangspunkt in lokalen Pixeln.
    VePoint b; ///< Endpunkt in lokalen Pixeln.
} VeGridLine;

/**
 * @brief Erzeugt kartesische oder rautenförmige Linien; blendet bei dichtem Zoom Zwischenlinien
 * aus.
 * @param bounds Sichtbarer lokaler Artboardbereich, maximal 100000 Pixel pro Achse.
 * @param width Zellbreite, 1..100000 Pixel; bei Isometrie volle Rautenbreite.
 * @param height Zellhöhe, 1..100000 Pixel; bei Isometrie volle Rautenhöhe.
 * @param isometric true für Rauten; 32 x 16 ergibt die übliche 2:1-Isometrie.
 * @param zoom Bildschirm-Pixel je Dokumentpixel, 0.02..32.
 * @param lines Aufruferpuffer; ungültige Eingaben schreiben nichts.
 * @param capacity Kapazität des Puffers; begrenzt auch die Anzahl der Berechnungsschritte.
 * @return Anzahl geschriebener Linien; 0 bei ungültiger Eingabe.
 */
int ve_grid_lines(VeRegion bounds, float width, float height, bool isometric, float zoom,
                  VeGridLine *lines, int capacity);
/**
 * @brief Sucht die nächste Rasterkreuzung innerhalb eines Bildschirm-Fangradius.
 * @param point Lokaler Artboardpunkt; negative Koordinaten sind erlaubt.
 * @param width Zellbreite in Dokumentpixeln, 1..100000.
 * @param height Zellhöhe in Dokumentpixeln, 1..100000.
 * @param isometric true für Rauten mit voller Breite/Höhe.
 * @param zoom Bildschirm-Pixel je Dokumentpixel, 0.02..32.
 * @param tolerance Radialer Fangradius in Bildschirm-Pixeln, 0..100.
 * @param result Zielpunkt; bleibt bei fehlendem Treffer oder ungültigen Eingaben unverändert.
 * @return true, wenn eine Rasterkreuzung getroffen wurde; unabhängig von sichtbaren Linien.
 */
bool ve_grid_snap(VePoint point, float width, float height, bool isometric, float zoom,
                  float tolerance, VePoint *result);
#endif
