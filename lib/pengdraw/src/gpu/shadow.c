/** @file @brief GPU alpha dilation, separable blur and tint without CPU pixel readback. */
#include "internal.h"

typedef struct {
    SDL_Texture *target;
    SDL_Rect viewport, clip;
    bool viewport_set, clipped;
    float xscale, yscale;
    Uint8 red, green, blue, alpha;
    SDL_BlendMode blend;
} State;

typedef struct {
    float sampling[4], tint[4];
} Uniforms;

static State capture(SDL_Renderer *r) {
    State s = {.target = SDL_GetRenderTarget(r),
               .viewport_set = SDL_RenderViewportSet(r),
               .clipped = SDL_RenderClipEnabled(r)};
    SDL_GetRenderViewport(r, &s.viewport);
    SDL_GetRenderClipRect(r, &s.clip);
    SDL_GetRenderScale(r, &s.xscale, &s.yscale);
    SDL_GetRenderDrawColor(r, &s.red, &s.green, &s.blue, &s.alpha);
    SDL_GetRenderDrawBlendMode(r, &s.blend);
    return s;
}

static void restore(SDL_Renderer *r, const State *s) {
    SDL_SetGPURenderState(r, NULL);
    SDL_SetRenderTarget(r, s->target);
    SDL_SetRenderScale(r, s->xscale, s->yscale);
    SDL_SetRenderViewport(r, s->viewport_set ? &s->viewport : NULL);
    SDL_SetRenderClipRect(r, s->clipped ? &s->clip : NULL);
    SDL_SetRenderDrawColor(r, s->red, s->green, s->blue, s->alpha);
    SDL_SetRenderDrawBlendMode(r, s->blend);
}

static bool prepare(PDrawGPU *gpu, SDL_Renderer *r, int w, int h) {
    if (gpu->width >= w && gpu->height >= h && gpu->textures[2]) {
        return true;
    }
    /* Keep high-water capacity so objects of different sizes do not allocate
     * three new render targets on every draw. The planner caps each axis at 4096. */
    if (w < gpu->width) {
        w = gpu->width;
    }
    if (h < gpu->height) {
        h = gpu->height;
    }
    for (int i = 0; i < 3; i++) {
        SDL_DestroyTexture(gpu->textures[i]);
        gpu->textures[i] = NULL;
    }
    gpu->width = gpu->height = 0;
    gpu->stats.texture_bytes = 0;
    for (int i = 0; i < 3; i++) {
        gpu->textures[i] =
            SDL_CreateTexture(r, SDL_PIXELFORMAT_RGBA8888, SDL_TEXTUREACCESS_TARGET, w, h);
        if (gpu->textures[i]) {
            gpu->stats.texture_bytes += (size_t)w * h * 4;
        }
        if (!gpu->textures[i] || !SDL_SetTextureBlendMode(gpu->textures[i], SDL_BLENDMODE_NONE) ||
            !SDL_SetTextureScaleMode(gpu->textures[i], SDL_SCALEMODE_NEAREST)) {
            return false;
        }
    }
    gpu->width = w;
    gpu->height = h;
    return true;
}

static bool pass(SDL_Renderer *r, PDrawGPU *gpu, SDL_Texture *source, SDL_Texture *target,
                 bool vertical, int radius, bool maximum) {
    Uniforms uniforms = {.sampling = {vertical ? 0 : 1.f / gpu->width,
                                      vertical ? 1.f / gpu->height : 0, (float)radius,
                                      maximum ? 1.f : 0.f}};
    return SDL_SetRenderTarget(r, target) && SDL_SetRenderScale(r, 1, 1) &&
           SDL_SetRenderViewport(r, NULL) && SDL_SetRenderClipRect(r, NULL) &&
           SDL_SetGPURenderStateFragmentUniforms(gpu->state, 0, &uniforms, sizeof uniforms) &&
           SDL_SetGPURenderState(r, gpu->state) && SDL_RenderTexture(r, source, NULL, NULL);
}

bool pdraw_gpu_shadow(SDL_Renderer *r, const PDrawShadow *shadow, const PDrawShadowRegion *region,
                      PDrawShadowPaint paint, void *context) {
    PDrawGPU *gpu = pdraw_gpu_context(r);
    if (!gpu) {
        return false;
    }
    State previous = capture(r);
    bool ok = prepare(gpu, r, region->width, region->height) && SDL_SetGPURenderState(r, NULL) &&
              SDL_SetRenderTarget(r, gpu->textures[0]) && SDL_SetRenderViewport(r, NULL) &&
              SDL_SetRenderClipRect(r, NULL) &&
              SDL_SetRenderScale(r, region->scale, region->scale) &&
              SDL_SetRenderDrawColor(r, 0, 0, 0, 0) && SDL_RenderClear(r);
    if (ok) {
        paint(context, -region->left, -region->top);
    }
    SDL_Texture *source = gpu->textures[0];
    if (ok && region->spread) {
        ok = pass(r, gpu, source, gpu->textures[1], false, region->spread, true) &&
             pass(r, gpu, gpu->textures[1], gpu->textures[2], true, region->spread, true);
        source = gpu->textures[2];
    }
    for (int i = 0; ok && region->radius && i < 3; i++) {
        ok = pass(r, gpu, source, gpu->textures[1], false, region->radius, false) &&
             pass(r, gpu, gpu->textures[1], gpu->textures[2], true, region->radius, false);
        source = gpu->textures[2];
    }
    restore(r, &previous);
    if (ok) {
        Uniforms uniforms = {
            .sampling = {0, 0, 0, 2},
            .tint = {(shadow->color >> 24) / 255.f, ((shadow->color >> 16) & 255) / 255.f,
                     ((shadow->color >> 8) & 255) / 255.f, (shadow->color & 255) / 255.f}};
        SDL_FRect destination = {region->left + region->offset_x, region->top + region->offset_y,
                                 region->width / region->scale, region->height / region->scale};
        SDL_FRect source_rect = {0, 0, region->width, region->height};
        ok = SDL_SetTextureBlendMode(source, SDL_BLENDMODE_BLEND) &&
             SDL_SetTextureScaleMode(source, SDL_SCALEMODE_LINEAR) &&
             SDL_SetGPURenderStateFragmentUniforms(gpu->state, 0, &uniforms, sizeof uniforms) &&
             SDL_SetGPURenderState(r, gpu->state) &&
             SDL_RenderTexture(r, source, &source_rect, &destination);
        SDL_SetGPURenderState(r, NULL);
        SDL_SetTextureBlendMode(source, SDL_BLENDMODE_NONE);
        SDL_SetTextureScaleMode(source, SDL_SCALEMODE_NEAREST);
    }
    restore(r, &previous);
    if (ok) {
        gpu->stats.shadow_draws++;
    } else {
        gpu->stats.shadow_failures++;
        if (gpu->stats.shadow_failures == 1) {
            SDL_LogWarn(SDL_LOG_CATEGORY_RENDER, "pengDraw: GPU shadow failed, CPU fallback: %s",
                        SDL_GetError());
        }
    }
    return ok;
}
