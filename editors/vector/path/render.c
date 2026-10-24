/** @file @brief Editor adapter for independent pengDraw painting. */
#include "render.h"
#include "editors/vector/rendering/drawing.h"

void vector_path_draw(SDL_Renderer *r, const VectorShape *s, PUIRect rect, float angle,
                      SDL_FRect board, float zoom, uint32_t fill, bool selected) {
    PDrawShape drawing = vector_shape_drawing(s);
    pdraw_path_draw(r, &drawing, vector_draw_rect(rect), angle, board, zoom, fill,
                    selected ? PUI_HIGHLIGHT : 0);
}
