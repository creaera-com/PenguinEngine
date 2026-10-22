/** @file @brief Shadow composition adapter: layer visibility and silhouettes belong to editor. */
#include "render.h"
#include "editors/vector/rendering/drawing.h"

bool vector_shadow_draw(SDL_Renderer *r, const VectorShadow *s, float zoom, PUIRect bounds,
                        VectorShadowPaint paint, void *context) {
    return pdraw_shadow_draw(r, s, zoom, vector_draw_rect(bounds), paint, context);
}
