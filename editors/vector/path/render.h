/** @file @brief Shape path rendering inside the caller's artboard clip. */
#ifndef PE_VECTOR_PATH_RENDER_H
#define PE_VECTOR_PATH_RENDER_H
#include "editors/vector/object/shape.h"
#include <SDL3/SDL.h>
/** @brief Draw fill, aligned solid stroke and optional contour selection; reads values only. */
void vector_path_draw(SDL_Renderer *renderer, const VectorShape *shape, PUIRect rect, float angle,
                      SDL_FRect board, float zoom, uint32_t fill, bool selected);
#endif
