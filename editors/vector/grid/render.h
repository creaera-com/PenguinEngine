/** @file @brief Zoom-aware, artboard-local drawing guides. */
#ifndef PE_VECTOR_GRID_RENDER_H
#define PE_VECTOR_GRID_RENDER_H
#include "pengui/pengui.h"
#include "state.h"
/** @brief Overlay a clipped grid; skip subpixel cells to bound rendering work. */
void vector_grid_draw(const VectorGrid *grid, SDL_Renderer *renderer, SDL_FRect board, float zoom);
#endif
