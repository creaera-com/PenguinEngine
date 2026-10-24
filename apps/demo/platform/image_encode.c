/** @file @brief Private PNG/JPEG encoder; SDL streams retain Unicode path support. */
#include "editor_host/images.h"
#define STB_IMAGE_WRITE_IMPLEMENTATION
#define STBI_WRITE_NO_STDIO
#include <stb_image_write.h>

typedef struct {
    SDL_IOStream *io;
    bool ok;
} Output;

static void write_bytes(void *context, void *data, int length) {
    Output *out = context;
    if (out->ok && length > 0) {
        out->ok = SDL_WriteIO(out->io, data, (size_t)length) == (size_t)length;
    }
}

bool editor_image_write(SDL_Surface *surface, const char *path, bool jpeg, int quality) {
    if (!surface || !path || !*path || quality < 1 || quality > 100) {
        return false;
    }
    SDL_Surface *rgba = SDL_ConvertSurface(surface, SDL_PIXELFORMAT_RGBA32);
    if (!rgba) {
        return false;
    }
    Output out = {SDL_IOFromFile(path, "wb"), true};
    out.ok = out.io != NULL;
    int encoded = 0;
    if (out.ok && jpeg) {
        /* stb JPEG expects tightly packed rows, whereas SDL surfaces may include padding. */
        unsigned char *packed = SDL_malloc((size_t)rgba->w * rgba->h * 4);
        if (packed) {
            for (int y = 0; y < rgba->h; y++) {
                SDL_memcpy(packed + (size_t)y * rgba->w * 4,
                           (unsigned char *)rgba->pixels + y * rgba->pitch, (size_t)rgba->w * 4);
            }
            encoded =
                stbi_write_jpg_to_func(write_bytes, &out, rgba->w, rgba->h, 4, packed, quality);
            SDL_free(packed);
        }
    } else if (out.ok) {
        encoded = stbi_write_png_to_func(write_bytes, &out, rgba->w, rgba->h, 4, rgba->pixels,
                                         rgba->pitch);
    }
    bool ok = out.ok && encoded;
    if (out.io) {
        ok = SDL_CloseIO(out.io) && ok;
    }
    SDL_DestroySurface(rgba);
    return ok;
}
