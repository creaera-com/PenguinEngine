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

/* Hash only values that affect artwork, not history pointers or viewport timers.
 * Resolved shapes include live transform/curve/gradient/numeric previews. */
static uint64_t artwork_revision(const VectorDocument *d, const VectorArtboard *board,
                                 SDL_FRect bounds) {
    uint64_t hash = UINT64_C(14695981039346656037);
    const void *values[] = {d->rectangles,     board,         &bounds,
                            &d->viewport.zoom, &d->selection, &d->active_rectangle,
                            &d->fill_preview};
    size_t sizes[] = {d->rectangle_count * sizeof *d->rectangles,
                      sizeof *board,
                      sizeof bounds,
                      sizeof d->viewport.zoom,
                      sizeof d->selection,
                      sizeof d->active_rectangle,
                      sizeof d->fill_preview};
    for (unsigned i = 0; i < sizeof sizes / sizeof *sizes; i++) {
        const unsigned char *bytes = values[i];
        for (size_t n = 0; n < sizes[i]; n++) {
            hash = (hash ^ bytes[n]) * UINT64_C(1099511628211);
        }
    }
    return hash;
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
    if (document->text_tool.editing_id || document->rectangle_tool.dragging) {
        /* Text cursors and unfinished creation gestures are transient output. */
        pdraw_render(&c->drawing, r, bounds, paint, &a);
    } else {
        pdraw_render_cached(&c->drawing, r, bounds, artwork_revision(c->scratch, board, bounds),
                            paint, &a);
    }
    if (!cache) {
        vector_artwork_renderer_destroy(&temporary);
    }
}
