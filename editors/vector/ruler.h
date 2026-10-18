/** @file @brief Compact artboard-local pixel rulers with zoom-adaptive subdivisions. */
#ifndef PE_VECTOR_RULER_H
#define PE_VECTOR_RULER_H
#include "pengui/pengui.h"
/** @brief Draw top/left rulers around a screen-space artboard, respecting the current clip. */
void vector_ruler_draw(SDL_Renderer *renderer, SDL_FRect board, float zoom);
#endif
