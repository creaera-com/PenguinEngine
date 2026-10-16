/** @file
 * @brief SDL-unabhängige Berechnung eines unskalierten Vorschauausschnitts. */
#ifndef PENGUIN_VECTOR_PREVIEW_H
#define PENGUIN_VECTOR_PREVIEW_H
#include <stdbool.h>

/** @brief Achsenparalleler Bereich in Pixeln. */
typedef struct {
    float x; ///< Linke Kante.
    float y; ///< Obere Kante.
    float w; ///< Breite.
    float h; ///< Höhe.
} VeRegion;

/** @brief Gleich große Quell- und Zielbereiche garantieren 1:1 ohne Skalierung. */
typedef struct {
    VeRegion source;      ///< Ausschnitt in lokalen Artboard-Pixeln.
    VeRegion destination; ///< Zentrierter Zielbereich in Bildschirm-Pixeln.
} VePreviewWindow;

/**
 * @brief Zentriert kleine Auswahlen; große folgen der Zeichenansicht innerhalb der Auswahlgrenzen.
 * @param selection Lokaler Auswahlbereich mit mindestens einem Pixel je Achse.
 * @param viewport Verfügbarer Vorschaubereich auf dem Bildschirm.
 * @param center_x Mittelpunkt der Zeichenansicht in lokalen Artboard-Pixeln.
 * @param center_y Vertikaler Mittelpunkt der Zeichenansicht in lokalen Artboard-Pixeln.
 * @param result Ausgabe; bleibt bei ungültigen Argumenten unverändert.
 * @return true für einen gültigen 1:1-Ausschnitt.
 */
bool ve_preview_window(VeRegion selection, VeRegion viewport, float center_x, float center_y,
                       VePreviewWindow *result);
#endif
