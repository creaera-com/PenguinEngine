/** @file @brief The private stb decoder accepts only PNG/JPEG and enforces allocation bounds. */
#include "editor_host/images.h"
#define STB_IMAGE_STATIC
#define STB_IMAGE_IMPLEMENTATION
#define STBI_MAX_DIMENSIONS 8192
#define STBI_ONLY_PNG
#define STBI_NO_GIF
#define STBI_ONLY_JPEG
#define STBI_NO_HDR
#define STBI_NO_LINEAR
#include <stb_image.h>

SDL_Surface *editor_image_decode(const char *path) {
    SDL_PathInfo info;
    if (!path || !SDL_GetPathInfo(path, &info) || info.type != SDL_PATHTYPE_FILE ||
        info.size > 32 * 1024 * 1024) {
        return NULL;
    }
    size_t length;
    unsigned char *data = SDL_LoadFile(path, &length);
    int w, h, channels;
    if (!data || length < 3 || length > 32 * 1024 * 1024 ||
        !((length >= 8 && !SDL_memcmp(data, "\x89PNG\r\n\x1a\n", 8)) ||
          (data[0] == 0xff && data[1] == 0xd8 && data[2] == 0xff)) ||
        !stbi_info_from_memory(data, (int)length, &w, &h, &channels) || w < 1 || h < 1 ||
        w > 8192 || h > 8192 || (unsigned long long)w * h > 16777216) {
        SDL_free(data);
        return NULL;
    }
    unsigned char *pixels = stbi_load_from_memory(data, (int)length, &w, &h, &channels, 4);
    SDL_free(data);
    if (!pixels) {
        return NULL;
    }
    SDL_Surface *surface = SDL_CreateSurface(w, h, SDL_PIXELFORMAT_RGBA32);
    if (surface) {
        for (int y = 0; y < h; y++) {
            SDL_memcpy((Uint8 *)surface->pixels + y * surface->pitch, pixels + (size_t)y * w * 4,
                       (size_t)w * 4);
        }
    }
    stbi_image_free(pixels);
    return surface;
}
