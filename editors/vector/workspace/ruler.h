/** @file
 * @brief Compact artboard-local pixel rulers with zoom-adaptive subdivisions. */
#ifndef PE_VECTOR_RULER_H
#define PE_VECTOR_RULER_H
#include "pengui/pengui.h"
/** @brief Draw top/left rulers around a screen-space artboard, respecting the current clip. */
void vector_ruler_draw(SDL_Renderer *renderer, SDL_FRect board, float zoom);
/** @brief Mark the pointer's X/Y positions in black on the top/left rulers.
 * Both marks are hidden outside the screen-space artboard. The existing clip is preserved. */
void vector_ruler_draw_pointer(SDL_Renderer *renderer, SDL_FRect board, float x, float y);
#endif
