/** @file @brief Renderer-owned GPU resources; private to the pengDraw SDL backend. */
#ifndef PDRAW_GPU_INTERNAL_H
#define PDRAW_GPU_INTERNAL_H
#include "pengdraw/pengdraw.h"
#define PDRAW_GPU_PROPERTY "pengdraw.gpu.effects"

typedef struct {
    SDL_GPUDevice *device; /* Borrowed from the SDL GPU renderer. */
    SDL_GPUShader *shader;
    SDL_GPURenderState *state;
    SDL_GPUShader *gradient_shader;
    SDL_GPURenderState *gradient_state;
    SDL_Texture *white;
    SDL_Texture *textures[3];
    int width, height;
    PDrawGPUStats stats;
} PDrawGPU;

typedef struct {
    int width, height, radius, spread;
    float left, top, scale, offset_x, offset_y;
} PDrawShadowRegion;

PDrawGPU *pdraw_gpu_context(SDL_Renderer *renderer);
SDL_Texture *pdraw_gpu_gradient_begin(SDL_Renderer *, const PDrawGradient *, PDrawRect, float angle,
                                      SDL_FRect board, float zoom, PDrawRect crop);
void pdraw_gpu_gradient_end(SDL_Renderer *, bool success);
bool pdraw_gpu_shadow(SDL_Renderer *, const PDrawShadow *, const PDrawShadowRegion *,
                      PDrawShadowPaint, void *);
#endif
