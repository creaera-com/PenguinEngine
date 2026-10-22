/** @file @brief Compatibility adapter for rectangle contours. */
#include "stroke_render.h"
#include "editors/vector/rendering/drawing.h"

void vector_stroke_draw(SDL_Renderer *r, PUIRect rect, float angle, float zoom,
                        VectorStroke stroke) {
    pdraw_stroke_draw(
        r, vector_draw_rect(rect), angle, zoom,
        (PDrawStroke){stroke.color, stroke.width, (PDrawStrokeAlignment)stroke.alignment});
}
