#include "penguin/render.h"
#define STB_IMAGE_IMPLEMENTATION
#define STBI_ONLY_PNG
#define STBI_MAX_DIMENSIONS 8192
#if defined(__GNUC__)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-function"
#endif
#include "stb_image.h"
#if defined(__GNUC__)
#pragma GCC diagnostic pop
#endif
#include <stdio.h>
#include <string.h>

void pe_assets_free(PeAssets *a) {
    if (a->tileset) {
        SDL_DestroyTexture(a->tileset);
    }
    memset(a, 0, sizeof *a);
}

bool pe_assets_load(PeAssets *a, SDL_Renderer *r, const char *root, const PeScene *s, char *error,
                    size_t size) {
    if (a->tileset && !strcmp(a->path, s->tileset) && a->source_tile == s->tileset_tile_size) {
        return true;
    }

    char path[1024];
    if (snprintf(path, sizeof path, "%s/%s", root, s->tileset) >= (int)sizeof path) {
        snprintf(error, size, "Asset path too long");
        return false;
    }

    int w, h, n;
    unsigned char *pixels = stbi_load(path, &w, &h, &n, 4);
    if (!pixels) {
        pe_assets_free(a);
        snprintf(error, size, "PNG load failed: %.150s", path);
        return false;
    }

    int tile = s->tileset_tile_size;
    if (w % tile || h % tile || w / tile * (h / tile) > 65535) {
        stbi_image_free(pixels);
        pe_assets_free(a);
        snprintf(error, size, "PNG dimensions must match tile size (max 65535 tiles)");
        return false;
    }

    SDL_Surface *surface = SDL_CreateSurfaceFrom(w, h, SDL_PIXELFORMAT_RGBA32, pixels, w * 4);
    SDL_Texture *texture = surface ? SDL_CreateTextureFromSurface(r, surface) : NULL;
    SDL_DestroySurface(surface);
    stbi_image_free(pixels);
    if (!texture) {
        snprintf(error, size, "Texture error: %s", SDL_GetError());
        return false;
    }
    SDL_SetTextureScaleMode(texture, SDL_SCALEMODE_NEAREST);
    SDL_SetTextureBlendMode(texture, SDL_BLENDMODE_BLEND);
    pe_assets_free(a);
    a->tileset = texture;
    a->width = w;
    a->height = h;
    a->source_tile = tile;
    snprintf(a->path, sizeof a->path, "%s", s->tileset);
    return true;
}

static void color(SDL_Renderer *r, uint32_t c) {
    SDL_SetRenderDrawColor(r, (Uint8)(c >> 24), (Uint8)(c >> 16), (Uint8)(c >> 8), (Uint8)c);
}

void pe_render_scene(SDL_Renderer *r, const PeScene *s, const PeAssets *a, const PeCamera *c,
                     SDL_FRect v, bool grid, bool collision) {
    SDL_Rect clip = {(int)v.x, (int)v.y, (int)v.w, (int)v.h};
    SDL_SetRenderClipRect(r, &clip);
    float size = s->tile_size * c->zoom;
    for (int l = 0; l < s->layer_count; l++) {
        if (!s->layers[l].visible) {
            continue;
        }
        for (int y = 0; y < s->height; y++) {
            for (int x = 0; x < s->width; x++) {
                uint16_t tile = pe_tile_get(s, l, x, y);
                if (!tile) {
                    continue;
                }
                SDL_FRect dst = {v.x + (x * s->tile_size - c->x) * c->zoom,
                                 v.y + (y * s->tile_size - c->y) * c->zoom, size, size};
                if (dst.x + dst.w < v.x || dst.y + dst.h < v.y || dst.x > v.x + v.w ||
                    dst.y > v.y + v.h) {
                    continue;
                }
                if (a->tileset && a->width / a->source_tile > 0 &&
                    tile <= a->width / a->source_tile * (a->height / a->source_tile)) {
                    int columns = a->width / a->source_tile;
                    SDL_FRect src = {(float)((tile - 1) % columns * a->source_tile),
                                     (float)((tile - 1) / columns * a->source_tile),
                                     (float)a->source_tile, (float)a->source_tile};
                    SDL_RenderTexture(r, a->tileset, &src, &dst);
                } else {
                    color(r, 0xb84dccff);
                    SDL_RenderFillRect(r, &dst);
                }
            }
        }
    }
    for (int i = 0; i < PE_ENTITIES; i++) {
        const PeObject *o = &s->objects[i];
        if (!o->alive || (o->mask & (PE_TRANSFORM | PE_SPRITE)) != (PE_TRANSFORM | PE_SPRITE)) {
            continue;
        }
        SDL_FRect dst = {v.x + (o->x - c->x) * c->zoom, v.y + (o->y - c->y) * c->zoom,
                         o->w * c->zoom, o->h * c->zoom};
        color(r, o->color);
        SDL_RenderFillRect(r, &dst);
        color(r, 0x17202aff);
        SDL_RenderRect(r, &dst);
    }
    if (collision) {
        SDL_SetRenderDrawBlendMode(r, SDL_BLENDMODE_BLEND);
        color(r, 0xff566880);
        for (int y = 0; y < s->height; y++) {
            for (int x = 0; x < s->width; x++) {
                if (s->solid[y * PE_MAP_MAX + x]) {
                    SDL_FRect dst = {v.x + (x * s->tile_size - c->x) * c->zoom,
                                     v.y + (y * s->tile_size - c->y) * c->zoom, size, size};
                    SDL_RenderFillRect(r, &dst);
                }
            }
        }
        SDL_SetRenderDrawBlendMode(r, SDL_BLENDMODE_NONE);
    }
    if (grid && size >= 8) {
        color(r, 0x3c3c3cff);
        for (int x = 0; x <= s->width; x++) {
            float sx = v.x + (x * s->tile_size - c->x) * c->zoom;
            SDL_RenderLine(r, sx, v.y - c->y * c->zoom, sx,
                           v.y + (s->height * s->tile_size - c->y) * c->zoom);
        }
        for (int y = 0; y <= s->height; y++) {
            float sy = v.y + (y * s->tile_size - c->y) * c->zoom;
            SDL_RenderLine(r, v.x - c->x * c->zoom, sy,
                           v.x + (s->width * s->tile_size - c->x) * c->zoom, sy);
        }
    }
    SDL_SetRenderClipRect(r, NULL);
}
