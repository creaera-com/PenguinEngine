/** @file
 * @brief Filled gradients tessellated through the object's actual contours and holes. */
#ifndef PE_VECTOR_GRADIENT_RENDER_H
#define PE_VECTOR_GRADIENT_RENDER_H
#include "editors/vector/object/shape.h"
#include <SDL3/SDL.h>
/** @brief Draw only the closed fill, preserving renderer state; false for unavailable/failed fill.
 */
bool vector_gradient_draw(SDL_Renderer *renderer, const VectorShape *shape, PUIRect rect,
                          float angle, SDL_FRect board, float zoom);
#endif
