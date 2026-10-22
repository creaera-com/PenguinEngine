/** @file @brief Four coverage samples per output pixel, with premultiplied-alpha compositing. */
#include "artwork.h"
#include "editors/vector/curve/tool.h"
#include "editors/vector/gradient/tool.h"
#include "editors/vector/workspace/selection.h"
#include <math.h>
#include <stdlib.h>

/* At most one 2048x2048 RGBA scratch texture (16 MiB), even for large exports. */
enum { TILE = 1024, SAMPLES = 2 };

typedef struct {
    SDL_Texture *target;
    SDL_Rect viewport, clip;
    bool viewport_set, clipped;
    float sx, sy;
    Uint8 red, green, blue, alpha;
    SDL_BlendMode blend;
} RenderState;

static RenderState capture(SDL_Renderer *r) {
    RenderState s = {.target = SDL_GetRenderTarget(r),
                     .viewport_set = SDL_RenderViewportSet(r),
                     .clipped = SDL_RenderClipEnabled(r)};
    SDL_GetRenderViewport(r, &s.viewport);
    SDL_GetRenderClipRect(r, &s.clip);
    SDL_GetRenderScale(r, &s.sx, &s.sy);
    SDL_GetRenderDrawColor(r, &s.red, &s.green, &s.blue, &s.alpha);
    SDL_GetRenderDrawBlendMode(r, &s.blend);
    return s;
}

static void restore(SDL_Renderer *r, const RenderState *s) {
    SDL_SetRenderTarget(r, s->target);
    SDL_SetRenderScale(r, s->sx, s->sy);
    SDL_SetRenderViewport(r, s->viewport_set ? &s->viewport : NULL);
    SDL_SetRenderClipRect(r, s->clipped ? &s->clip : NULL);
    SDL_SetRenderDrawColor(r, s->red, s->green, s->blue, s->alpha);
    SDL_SetRenderDrawBlendMode(r, s->blend);
}

void vector_artwork_renderer_destroy(VectorArtworkRenderer *c) {
    SDL_DestroyTexture(c->target);
    free(c->scratch);
    *c = (VectorArtworkRenderer){0};
}

static bool prepare(VectorArtworkRenderer *c, SDL_Renderer *r, int w, int h) {
    if (c->renderer && c->renderer != r) {
        vector_artwork_renderer_destroy(c);
    }
    c->renderer = r;
    if (!c->scratch) {
        c->scratch = malloc(sizeof *c->scratch);
    }
    if (!c->scratch) {
        return false;
    }
    w *= SAMPLES;
    h *= SAMPLES;
    if (c->target && c->width >= w && c->height >= h) {
        return true;
    }
    if (w < c->width) {
        w = c->width;
    }
    if (h < c->height) {
        h = c->height;
    }
    SDL_DestroyTexture(c->target);
    c->width = w;
    c->height = h;
    c->target = SDL_CreateTexture(r, SDL_PIXELFORMAT_RGBA8888, SDL_TEXTUREACCESS_TARGET, w, h);
    if (!c->target || !SDL_SetTextureScaleMode(c->target, SDL_SCALEMODE_LINEAR) ||
        !SDL_SetTextureBlendMode(c->target, SDL_BLENDMODE_BLEND_PREMULTIPLIED)) {
        SDL_DestroyTexture(c->target);
        c->target = NULL;
        return false;
    }
    return true;
}

static void tile(VectorArtworkRenderer *c, VectorDocument *document, const VectorArtboard *board,
                 SDL_Renderer *r, SDL_FRect bounds, SDL_Rect crop, const RenderState *previous,
                 VectorTextRenderer *text, VectorImageRenderer *images) {
    bool ok = SDL_SetRenderTarget(r, c->target);
    if (ok) {
        SDL_SetRenderScale(r, 1, 1);
        SDL_SetRenderViewport(r, NULL);
        SDL_SetRenderClipRect(r, NULL);
        SDL_SetRenderDrawColor(r, 0, 0, 0, 0);
        SDL_SetRenderDrawBlendMode(r, SDL_BLENDMODE_NONE);
        SDL_RenderFillRect(r, &(SDL_FRect){0, 0, crop.w * SAMPLES, crop.h * SAMPLES});
        SDL_Rect clip = {0, 0, crop.w * SAMPLES, crop.h * SAMPLES};
        SDL_SetRenderClipRect(r, &clip);
        SDL_FRect high = {(bounds.x - crop.x) * SAMPLES, (bounds.y - crop.y) * SAMPLES,
                          bounds.w * SAMPLES, bounds.h * SAMPLES};
        vector_rectangles_draw(c->scratch, board, r, high, text, images);
    }
    restore(r, previous);
    if (ok) {
        SDL_FRect src = {0, 0, crop.w * SAMPLES, crop.h * SAMPLES};
        SDL_FRect dst = {crop.x, crop.y, crop.w, crop.h};
        SDL_RenderTexture(r, c->target, &src, &dst);
    } else {
        SDL_SetRenderClipRect(r, &crop);
        vector_rectangles_draw(document, board, r, bounds, text, images);
        restore(r, previous);
    }
}

void vector_artwork_draw(VectorArtworkRenderer *cache, VectorDocument *document,
                         const VectorArtboard *board, SDL_Renderer *r, SDL_FRect bounds,
                         VectorTextRenderer *text, VectorImageRenderer *images) {
    RenderState previous = capture(r);
    int width, height;
    if (!SDL_GetCurrentRenderOutputSize(r, &width, &height) || bounds.w <= 0 || bounds.h <= 0) {
        return;
    }
    /* Native-coordinate draws are the artwork contract. Leave unusual caller transforms intact. */
    if (previous.sx != 1 || previous.sy != 1 || previous.viewport_set) {
        vector_rectangles_draw(document, board, r, bounds, text, images);
        restore(r, &previous);
        return;
    }
    float left = fmaxf(0, bounds.x), top = fmaxf(0, bounds.y);
    float right = fminf(width, bounds.x + bounds.w), bottom = fminf(height, bounds.y + bounds.h);
    if (previous.clipped) {
        left = fmaxf(left, previous.clip.x);
        top = fmaxf(top, previous.clip.y);
        right = fminf(right, previous.clip.x + previous.clip.w);
        bottom = fminf(bottom, previous.clip.y + previous.clip.h);
    }
    if (right <= left || bottom <= top) {
        return;
    }
    SDL_Rect crop = {(int)floorf(left), (int)floorf(top), (int)ceilf(right) - (int)floorf(left),
                     (int)ceilf(bottom) - (int)floorf(top)};
    VectorArtworkRenderer temporary = {0};
    VectorArtworkRenderer *c = cache ? cache : &temporary;
    int w = crop.w < TILE ? crop.w : TILE, h = crop.h < TILE ? crop.h : TILE;
    if (!prepare(c, r, w, h)) {
        vector_rectangles_draw(document, board, r, bounds, text, images);
    } else {
        *c->scratch =
            *document; /* Values and borrowed history payloads only; never destroy as a document. */
        /* Resolve live transforms at the real interaction zoom: supersampling must
         * never change pixel-based snap tolerances or apply a transform twice. */
        for (int i = 0; i < document->rectangle_count; i++) {
            VectorShape resolved = vector_curve_display_shape(document, &document->rectangles[i]);
            const VectorShape *shown = &resolved;
            c->scratch->rectangles[i] = *shown;
            c->scratch->rectangles[i].rect = vector_rectangle_display_rect(document, shown);
            c->scratch->rectangles[i].angle = vector_rectangle_display_angle(document, shown);
            c->scratch->rectangles[i].gradient = *vector_gradient_display(document, shown);
        }
        c->scratch->selection_tool = (VectorSelectionTool){0};
        c->scratch->curve_tool = (VectorCurveTool){0};
        c->scratch->gradient_tool = (VectorGradientTool){0};
        c->scratch->viewport.zoom *= SAMPLES;
        for (int y = crop.y; y < crop.y + crop.h; y += TILE) {
            for (int x = crop.x; x < crop.x + crop.w; x += TILE) {
                SDL_Rect part = {x, y, crop.x + crop.w - x, crop.y + crop.h - y};
                if (part.w > TILE) {
                    part.w = TILE;
                }
                if (part.h > TILE) {
                    part.h = TILE;
                }
                tile(c, document, board, r, bounds, part, &previous, text, images);
            }
        }
    }
    restore(r, &previous);
    if (!cache) {
        vector_artwork_renderer_destroy(&temporary);
    }
}
