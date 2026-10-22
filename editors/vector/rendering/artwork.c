/** @file @brief Resolve editor gestures once, then replay values through pengDraw supersampling. */
#include "artwork.h"
#include "editors/vector/curve/tool.h"
#include "editors/vector/gradient/tool.h"
#include "editors/vector/workspace/selection.h"
#include <stdlib.h>

typedef struct {
    VectorDocument *scratch;
    const VectorArtboard *board;
    SDL_FRect bounds;
    VectorTextRenderer *text;
    VectorImageRenderer *images;
    float zoom;
} Artwork;

static void paint(void *context, SDL_Renderer *r, float ox, float oy, float scale) {
    Artwork *a = context;
    a->scratch->viewport.zoom = a->zoom * scale;
    SDL_FRect high = {ox + a->bounds.x * scale, oy + a->bounds.y * scale, a->bounds.w * scale,
                      a->bounds.h * scale};
    vector_objects_draw(a->scratch, a->board, r, high, a->text, a->images);
}

void vector_artwork_renderer_destroy(VectorArtworkRenderer *c) {
    pdraw_renderer_destroy(&c->drawing);
    free(c->scratch);
    *c = (VectorArtworkRenderer){0};
}

void vector_artwork_draw(VectorArtworkRenderer *cache, VectorDocument *document,
                         const VectorArtboard *board, SDL_Renderer *r, SDL_FRect bounds,
                         VectorTextRenderer *text, VectorImageRenderer *images) {
    VectorArtworkRenderer temporary = {0};
    VectorArtworkRenderer *c = cache ? cache : &temporary;
    if (!c->scratch) {
        c->scratch = malloc(sizeof *c->scratch);
    }
    if (!c->scratch) {
        vector_objects_draw(document, board, r, bounds, text, images);
        return;
    }
    *c->scratch = *document;
    for (int i = 0; i < document->rectangle_count; i++) {
        VectorShape resolved = vector_curve_display_shape(document, &document->rectangles[i]);
        c->scratch->rectangles[i] = resolved;
        c->scratch->rectangles[i].rect = vector_rectangle_display_rect(document, &resolved);
        c->scratch->rectangles[i].angle = vector_rectangle_display_angle(document, &resolved);
        c->scratch->rectangles[i].gradient = *vector_gradient_display(document, &resolved);
    }
    c->scratch->selection_tool = (VectorSelectionTool){0};
    c->scratch->curve_tool = (VectorCurveTool){0};
    c->scratch->gradient_tool = (VectorGradientTool){0};
    Artwork a = {c->scratch, board, bounds, text, images, document->viewport.zoom};
    pdraw_render(&c->drawing, r, bounds, paint, &a);
    if (!cache) {
        vector_artwork_renderer_destroy(&temporary);
    }
}
