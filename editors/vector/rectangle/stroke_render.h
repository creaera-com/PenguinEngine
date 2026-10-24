/** @file
 * @brief Render a solid, aligned rectangular stroke around a screen-space rectangle. */
#ifndef PE_VECTOR_STROKE_RENDER_H
#define PE_VECTOR_STROKE_RENDER_H
#include "rectangle.h"
#include <SDL3/SDL.h>
/** @brief Width scales with zoom; rotation uses the original rectangle center. Caller owns clip. */
void vector_stroke_draw(SDL_Renderer *renderer, PUIRect rect, float angle, float zoom,
                        VectorStroke stroke);
#endif
