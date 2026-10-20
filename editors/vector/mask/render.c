/** @file @brief Render existing content once, then sample it only inside the mask's triangles. */
#include "render.h"

typedef struct {
    VectorMaskRenderer *context;
    SDL_FRect board;
    float zoom;
} Composite;

static void triangle(void *data, const VectorPoint points[3]) {
    Composite *c = data;
    SDL_Vertex vertices[3];
    for (int i = 0; i < 3; i++) {
        float x = c->board.x + points[i].x * c->zoom;
        float y = c->board.y + points[i].y * c->zoom;
        vertices[i] = (SDL_Vertex){.position = {x, y},
                                   .color = {1, 1, 1, 1},
                                   .tex_coord = {x / c->context->width, y / c->context->height}};
    }
    SDL_RenderGeometry(c->context->renderer, c->context->texture, vertices, 3, NULL, 0);
}

void vector_mask_render(VectorMaskRenderer *c, const VectorShape *mask, PUIRect rect, float angle,
                        SDL_FRect board, float zoom, void (*draw)(void *), void *data) {
    if (mask->flags & VECTOR_OBJECT_HIDDEN) {
        return;
    }
    SDL_Renderer *r = c->renderer;
    if (!c->texture) {
        if (!SDL_GetCurrentRenderOutputSize(r, &c->width, &c->height)) {
            return;
        }
        c->texture = SDL_CreateTexture(r, SDL_PIXELFORMAT_RGBA8888, SDL_TEXTUREACCESS_TARGET,
                                       c->width, c->height);
        c->clipped = SDL_CreateTexture(r, SDL_PIXELFORMAT_RGBA8888, SDL_TEXTUREACCESS_TARGET,
                                       c->width, c->height);
        if (!c->texture || !c->clipped ||
            !SDL_SetTextureBlendMode(c->texture, SDL_BLENDMODE_NONE) ||
            !SDL_SetTextureBlendMode(c->clipped, SDL_BLENDMODE_BLEND_PREMULTIPLIED)) {
            vector_mask_renderer_destroy(c);
            return;
        }
    }
    SDL_Texture *previous = SDL_GetRenderTarget(r);
    SDL_Rect clip;
    bool clipped = SDL_RenderClipEnabled(r);
    SDL_GetRenderClipRect(r, &clip);
    SDL_BlendMode blend;
    SDL_GetRenderDrawBlendMode(r, &blend);
    Uint8 red, green, blue, alpha;
    SDL_GetRenderDrawColor(r, &red, &green, &blue, &alpha);
    if (!SDL_SetRenderTarget(r, c->texture)) {
        return;
    }
    SDL_SetRenderClipRect(r, NULL);
    SDL_SetRenderDrawColor(r, 0, 0, 0, 0);
    SDL_RenderClear(r);
    SDL_SetRenderClipRect(r, clipped ? &clip : NULL);
    draw(data);
    if (!SDL_SetRenderTarget(r, c->clipped)) {
        SDL_SetRenderTarget(r, previous);
        SDL_SetRenderDrawColor(r, red, green, blue, alpha);
        SDL_SetRenderDrawBlendMode(r, blend);
        return;
    }
    SDL_SetRenderClipRect(r, NULL);
    SDL_SetRenderDrawColor(r, 0, 0, 0, 0);
    SDL_RenderClear(r);
    SDL_SetRenderClipRect(r, clipped ? &clip : NULL);
    VectorPath path = vector_shape_path(mask, rect, angle);
    Composite composite = {c, board, zoom};
    vector_path_triangles(&path, triangle, &composite);
    SDL_SetRenderTarget(r, previous);
    SDL_SetRenderClipRect(r, clipped ? &clip : NULL);
    /* Sampling uses NONE, then a single premultiplied blit. This also avoids the software
     * renderer's triangle blitter, which does not implement premultiplied blending. */
    SDL_RenderTexture(r, c->clipped, NULL, &(SDL_FRect){0, 0, c->width, c->height});
    SDL_SetRenderDrawColor(r, red, green, blue, alpha);
    SDL_SetRenderDrawBlendMode(r, blend);
}

void vector_mask_renderer_destroy(VectorMaskRenderer *c) {
    SDL_DestroyTexture(c->texture);
    SDL_DestroyTexture(c->clipped);
    c->clipped = NULL;
    c->texture = NULL;
}
