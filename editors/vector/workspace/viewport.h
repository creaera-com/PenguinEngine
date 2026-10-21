/** @file @brief Plain per-document viewport state; no renderer or event processing. */
#ifndef PE_VECTOR_VIEWPORT_H
#define PE_VECTOR_VIEWPORT_H
#include "pengui/pengui.h"

typedef struct {
    PUIRect bounds;
    float x, y, zoom;
    float mouse_x, mouse_y;
    bool panning;
    bool zooming; ///< Transient animation; not saved in documents or checkpoints.
    float zoom_from, zoom_target, zoom_anchor_x, zoom_anchor_y;
    double zoom_started_ms;
} VectorViewport;

/** @brief Advance the 140 ms zoom transition, keeping its screen anchor fixed. */
void vector_viewport_update(VectorViewport *viewport, double now_ms);
/** @brief Accumulate fractional wheel steps; x/y are relative to viewport bounds. */
void vector_viewport_zoom(VectorViewport *viewport, float steps, float x, float y, double now_ms);
#endif
