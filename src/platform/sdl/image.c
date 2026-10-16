#include "penguin/image.h"
#include "stb_image.h"
#include <stdio.h>

SDL_Texture *pe_image_load(SDL_Renderer *renderer, const char *path, char *error, size_t size) {
    int width, height, channels;
    SDL_Texture *texture = NULL;
    if (renderer && path && stbi_info(path, &width, &height, &channels) && width > 0 &&
        height > 0 && width <= 1024 && height <= 1024) {
        unsigned char *pixels = stbi_load(path, &width, &height, &channels, 4);
        if (pixels) {
            SDL_Surface *surface =
                SDL_CreateSurfaceFrom(width, height, SDL_PIXELFORMAT_RGBA32, pixels, width * 4);
            texture = surface ? SDL_CreateTextureFromSurface(renderer, surface) : NULL;
            SDL_DestroySurface(surface);
            stbi_image_free(pixels);
        }
    }
    if (texture) {
        SDL_SetTextureBlendMode(texture, SDL_BLENDMODE_BLEND);
        SDL_SetTextureScaleMode(texture, SDL_SCALEMODE_LINEAR);
    } else if (error && size) {
        snprintf(error, size, "Cannot load PNG icon (maximum 1024 x 1024): %s", path ? path : "");
    }
    return texture;
}
