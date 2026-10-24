/** @file @brief Shade tessellated gradient fills on the GPU instead of rasterizing CPU pixels. */
#include "internal.h"
#include <math.h>

typedef struct {
    float crop[4], origin[4], shape[4], axis[4], ends[4];
    float colors[PDRAW_GRADIENT_STOPS][4], positions[PDRAW_GRADIENT_STOPS][4];
} Uniforms;

SDL_Texture *pdraw_gpu_gradient_begin(SDL_Renderer *r, const PDrawGradient *g, PDrawRect rect,
                                      float angle, SDL_FRect board, float zoom, PDrawRect crop) {
    PDrawGPU *gpu = pdraw_gpu_context(r);
    if (!gpu) {
        return NULL;
    }
    Uniforms uniforms = {
        .crop = {crop.x, crop.y, crop.w, crop.h},
        .origin = {board.x, board.y, zoom, (float)g->kind},
        .shape = {rect.x, rect.y, rect.w, rect.h},
        .axis = {cosf(angle * .017453293f), sinf(angle * .017453293f), (float)g->count, 0},
        .ends = {g->x1, g->y1, g->x2, g->y2}};
    for (unsigned i = 0; i < g->count; i++) {
        uint32_t color = g->stops[i].color;
        uniforms.positions[i][0] = g->stops[i].position;
        uniforms.colors[i][0] = (color >> 24) / 255.f;
        uniforms.colors[i][1] = ((color >> 16) & 255) / 255.f;
        uniforms.colors[i][2] = ((color >> 8) & 255) / 255.f;
        uniforms.colors[i][3] = (color & 255) / 255.f;
    }
    if (!SDL_SetGPURenderStateFragmentUniforms(gpu->gradient_state, 0, &uniforms,
                                               sizeof uniforms) ||
        !SDL_SetGPURenderState(r, gpu->gradient_state)) {
        pdraw_gpu_gradient_end(r, false);
        return NULL;
    }
    return gpu->white;
}

void pdraw_gpu_gradient_end(SDL_Renderer *r, bool success) {
    SDL_SetGPURenderState(r, NULL);
    PDrawGPU *gpu = pdraw_gpu_context(r);
    if (success) {
        gpu->stats.gradient_draws++;
    } else {
        gpu->stats.gradient_failures++;
        if (gpu->stats.gradient_failures == 1) {
            SDL_LogWarn(SDL_LOG_CATEGORY_RENDER, "pengDraw: GPU gradient failed: %s",
                        SDL_GetError());
        }
    }
}
