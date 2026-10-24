/** @file @brief Bounded PNG loading and SDL texture creation. */
#include "pengui/pengui.h"
#include <limits.h>
#define STB_IMAGE_IMPLEMENTATION
#define STBI_MAX_DIMENSIONS 1024
#define STBI_ONLY_PNG
#define STBI_NO_HDR
#define STBI_NO_LINEAR
#include "stb_image.h"

static SDL_Texture *texture_from_pixels(SDL_Renderer *renderer, unsigned char *pixels, int width,
                                        int height) {
    if (!pixels) {
        return NULL;
    }
    SDL_Surface *surface =
        SDL_CreateSurfaceFrom(width, height, SDL_PIXELFORMAT_RGBA32, pixels, width * 4);
    SDL_Texture *texture = surface ? SDL_CreateTextureFromSurface(renderer, surface) : NULL;
    SDL_DestroySurface(surface);
    stbi_image_free(pixels);
    if (texture) {
        SDL_SetTextureBlendMode(texture, SDL_BLENDMODE_BLEND);
    }
    return texture;
}

static bool valid_dimensions(int width, int height) {
    return width > 0 && height > 0 && width <= 1024 && height <= 1024;
}

SDL_Texture *pui_image_load(SDL_Renderer *renderer, const char *path) {
    int width, height, channels;
    if (!renderer || !path || !stbi_info(path, &width, &height, &channels) ||
        !valid_dimensions(width, height)) {
        return NULL;
    }
    unsigned char *pixels = stbi_load(path, &width, &height, &channels, 4);
    return texture_from_pixels(renderer, pixels, width, height);
}

SDL_Texture *pui_image_load_memory(SDL_Renderer *renderer, const void *data, size_t length) {
    int width, height, channels;
    if (!renderer || !data || !length || length > INT_MAX ||
        !stbi_info_from_memory(data, (int)length, &width, &height, &channels) ||
        !valid_dimensions(width, height)) {
        return NULL;
    }
    unsigned char *pixels = stbi_load_from_memory(data, (int)length, &width, &height, &channels, 4);
    return texture_from_pixels(renderer, pixels, width, height);
}
