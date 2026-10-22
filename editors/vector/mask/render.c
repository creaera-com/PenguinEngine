/** @file @brief Layer visibility remains an editor decision; clipping is shared pengDraw code. */
#include "render.h"
#include "editors/vector/rendering/drawing.h"

void vector_mask_render(VectorMaskRenderer *c, const VectorShape *mask, PUIRect rect, float angle,
                        SDL_FRect board, float zoom, void (*paint)(void *), void *context) {
    if (mask->flags & VECTOR_OBJECT_HIDDEN) {
        return;
    }
    PDrawShape drawing = vector_shape_drawing(mask);
    pdraw_mask_render(c, &drawing, vector_draw_rect(rect), angle, board, zoom, paint, context);
}

void vector_mask_renderer_destroy(VectorMaskRenderer *c) {
    pdraw_mask_renderer_destroy(c);
}
