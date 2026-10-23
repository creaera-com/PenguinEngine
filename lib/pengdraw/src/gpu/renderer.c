/** @file @brief GPU renderer selection, shader loading and explicit resource lifetime. */
#include "internal.h"
#include "shaders/gradient_bytes.inc"
#include "shaders/shadow_bytes.inc"
#include <stdlib.h>
#include <string.h>

PDrawGPU *pdraw_gpu_context(SDL_Renderer *r) {
    return r ? SDL_GetPointerProperty(SDL_GetRendererProperties(r), PDRAW_GPU_PROPERTY, NULL)
             : NULL;
}

static void release(PDrawGPU *gpu) {
    if (!gpu) {
        return;
    }
    for (int i = 0; i < 3; i++) {
        SDL_DestroyTexture(gpu->textures[i]);
    }
    SDL_DestroyTexture(gpu->white);
    SDL_DestroyGPURenderState(gpu->gradient_state);
    if (gpu->gradient_shader) {
        SDL_ReleaseGPUShader(gpu->device, gpu->gradient_shader);
    }
    SDL_DestroyGPURenderState(gpu->state);
    if (gpu->shader) {
        SDL_ReleaseGPUShader(gpu->device, gpu->shader);
    }
    free(gpu);
}

static bool initialize(SDL_Renderer *r) {
    PDrawGPU *gpu = calloc(1, sizeof *gpu);
    if (!gpu) {
        return false;
    }
    gpu->device = SDL_GetGPURendererDevice(r);
    SDL_GPUShaderFormat formats = SDL_GetGPUShaderFormats(gpu->device);
    SDL_GPUShaderCreateInfo info = {.stage = SDL_GPU_SHADERSTAGE_FRAGMENT,
                                    .num_samplers = 1,
                                    .num_uniform_buffers = 1,
                                    .entrypoint = "main"};
    if (formats & SDL_GPU_SHADERFORMAT_SPIRV) {
        info.format = SDL_GPU_SHADERFORMAT_SPIRV;
        info.code = shadow_spirv;
        info.code_size = sizeof shadow_spirv;
    } else if (formats & SDL_GPU_SHADERFORMAT_DXIL) {
        info.format = SDL_GPU_SHADERFORMAT_DXIL;
        info.code = shadow_dxil;
        info.code_size = sizeof shadow_dxil;
    } else if (formats & SDL_GPU_SHADERFORMAT_MSL) {
        info.format = SDL_GPU_SHADERFORMAT_MSL;
        info.code = shadow_msl;
        info.code_size = sizeof shadow_msl;
        info.entrypoint = "main0";
    }
    gpu->shader = SDL_CreateGPUShader(gpu->device, &info);
    if (gpu->shader) {
        SDL_GPURenderStateCreateInfo state = {.fragment_shader = gpu->shader};
        gpu->state = SDL_CreateGPURenderState(r, &state);
    }
    if (info.format == SDL_GPU_SHADERFORMAT_SPIRV) {
        info.code = gradient_spirv;
        info.code_size = sizeof gradient_spirv;
    } else if (info.format == SDL_GPU_SHADERFORMAT_DXIL) {
        info.code = gradient_dxil;
        info.code_size = sizeof gradient_dxil;
    } else {
        info.code = gradient_msl;
        info.code_size = sizeof gradient_msl;
    }
    gpu->gradient_shader = SDL_CreateGPUShader(gpu->device, &info);
    if (gpu->gradient_shader) {
        SDL_GPURenderStateCreateInfo state = {.fragment_shader = gpu->gradient_shader};
        gpu->gradient_state = SDL_CreateGPURenderState(r, &state);
    }
    gpu->white = SDL_CreateTexture(r, SDL_PIXELFORMAT_RGBA32, SDL_TEXTUREACCESS_STATIC, 1, 1);
    Uint32 white = 0xffffffff;
    bool ready = gpu->state && gpu->gradient_state && gpu->white &&
                 SDL_UpdateTexture(gpu->white, NULL, &white, 4) &&
                 SDL_SetTextureBlendMode(gpu->white, SDL_BLENDMODE_BLEND);
    if (!ready || !SDL_SetPointerProperty(SDL_GetRendererProperties(r), PDRAW_GPU_PROPERTY, gpu)) {
        release(gpu);
        return false;
    }
    return true;
}

SDL_Renderer *pdraw_create_renderer(SDL_Window *window) {
    const char *driver = SDL_GetHint(SDL_HINT_RENDER_DRIVER);
    if (driver && *driver && strcmp(driver, "gpu")) {
        SDL_Renderer *r = SDL_CreateRenderer(window, driver);
        if (r) {
            SDL_Log("pengDraw: explicit compatibility renderer (%s), GPU effects disabled",
                    SDL_GetRendererName(r));
        }
        return r;
    }
    SDL_Renderer *r = SDL_CreateGPURenderer(NULL, window);
    if (r && initialize(r)) {
        SDL_Log("pengDraw: GPU renderer (%s), GPU shadow and gradient shaders enabled",
                SDL_GetGPUDeviceDriver(SDL_GetGPURendererDevice(r)));
        return r;
    }
    char reason[512];
    SDL_strlcpy(reason, SDL_GetError(), sizeof reason);
    SDL_DestroyRenderer(r);
    if (driver && !strcmp(driver, "gpu")) {
        SDL_SetError("pengDraw GPU initialization failed: %s", reason);
        return NULL;
    }
    SDL_LogWarn(SDL_LOG_CATEGORY_RENDER,
                "pengDraw: GPU effects unavailable (%s); using compatibility renderer", reason);
    return SDL_CreateRenderer(window, NULL);
}

bool pdraw_renderer_gpu_effects(SDL_Renderer *renderer) {
    return pdraw_gpu_context(renderer) != NULL;
}

PDrawGPUStats pdraw_renderer_gpu_stats(SDL_Renderer *r) {
    PDrawGPU *gpu = pdraw_gpu_context(r);
    return gpu ? gpu->stats : (PDrawGPUStats){0};
}

void pdraw_destroy_renderer(SDL_Renderer *r) {
    if (!r) {
        return;
    }
    PDrawGPU *gpu = pdraw_gpu_context(r);
    if (gpu) {
        SDL_SetGPURenderState(r, NULL);
        SDL_FlushRenderer(r);
        SDL_ClearProperty(SDL_GetRendererProperties(r), PDRAW_GPU_PROPERTY);
        release(gpu);
    }
    SDL_DestroyRenderer(r);
}
