/** @file @brief Editor adapter for independent pengDraw painting. */
#include "render.h"
#include "editors/vector/rendering/drawing.h"

bool vector_gradient_draw(SDL_Renderer *r, const VectorShape *s, PUIRect rect, float angle,
                          SDL_FRect board, float zoom) {
    if (*s->image.source || s->text.enabled) {
        return false;
    }
    PDrawShape drawing = vector_shape_drawing(s);
    return pdraw_gradient_draw(r, &drawing, vector_draw_rect(rect), angle, board, zoom);
}
