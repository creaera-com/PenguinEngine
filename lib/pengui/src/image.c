#include "pengui/view.h"
#define STB_IMAGE_IMPLEMENTATION
#define STBI_MAX_DIMENSIONS 1024
#define STBI_ONLY_PNG
#define STBI_NO_HDR
#define STBI_NO_LINEAR
#include "stb_image.h"

SDL_Texture *ui_image_load(SDL_Renderer *renderer, const char *path) {
    int width, height, channels;
    if (!renderer || !path || !stbi_info(path, &width, &height, &channels) || width < 1 ||
        height < 1 || width > 1024 || height > 1024) {
        return NULL;
    }
    unsigned char *pixels = stbi_load(path, &width, &height, &channels, 4);
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
