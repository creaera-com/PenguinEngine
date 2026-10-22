/** @file @brief Session-local color scheme designer and interactive hue wheel. */
#ifndef VECTOR_UI_PALETTE_H
#define VECTOR_UI_PALETTE_H
#include "editors/vector/palette/palette.h"

typedef struct {
    uint32_t base;
    VectorHarmony harmony;
    float spread;
    PUIRect area;
    bool dragging;
} VectorPaletteUI;

/** @brief Render hue wheel with harmony markers and clickable color samples. */
void vector_palette_ui_draw(VectorPaletteUI *state, SDL_Renderer *renderer, PUIRect area);
/** @brief Handle designer actions without modifying artwork or document history. */
bool vector_palette_ui_action(VectorPaletteUI *state, PUIView *view, const char *action);
/** @brief Route wheel dragging and swatch copying only while its canvas is visible. */
bool vector_palette_ui_event(VectorPaletteUI *state, PUIView *view, const SDL_Event *event);
#endif
