/** @file @brief Smooth ellipse fills and solid inside/center/outside contour bands. */
#ifndef PE_VECTOR_ELLIPSE_RENDER_H
#define PE_VECTOR_ELLIPSE_RENDER_H
#include "ellipse.h"
/** @brief Draw a transformed ellipse with screen-space contour sampling, respecting current clip.
 */
void vector_ellipse_draw(SDL_Renderer *renderer, const VectorShape *shape, PUIRect rect,
                         float angle, SDL_FRect board, float zoom, uint32_t fill, bool selected);
#endif
