/** @file @brief Immutable project assets are lazily decoded into an LRU capped at 64 MiB. */
#include "render.h"
#include "editor_host/editor.h"
#include <stdlib.h>
#include <string.h>
#define CACHE_COUNT 8
#define CACHE_BYTES (64u * 1024u * 1024u)

typedef struct {
    char source[4096];
    SDL_Texture *texture;
    SDL_Renderer *renderer;
    size_t bytes;
    unsigned long long age;
} Entry;

struct VectorImageRenderer {
    Entry entries[CACHE_COUNT];
    unsigned long long age;
    size_t bytes;
};

VectorImageRenderer *vector_image_renderer_create(void) {
    return calloc(1, sizeof(VectorImageRenderer));
}

void vector_image_renderer_destroy(VectorImageRenderer *c) {
    if (c) {
        for (int i = 0; i < CACHE_COUNT; i++) {
            SDL_DestroyTexture(c->entries[i].texture);
        }
        free(c);
    }
}

static int oldest(VectorImageRenderer *c) {
    int at = 0;
    for (int i = 1; i < CACHE_COUNT; i++) {
        if (c->entries[i].age < c->entries[at].age) {
            at = i;
        }
    }
    return at;
}

static void evict(VectorImageRenderer *c, int at) {
    c->bytes -= c->entries[at].bytes;
    SDL_DestroyTexture(c->entries[at].texture);
    c->entries[at] = (Entry){0};
}

static SDL_Texture *texture(VectorImageRenderer *c, SDL_Renderer *r, const char *path) {
    if (!c) {
        return NULL;
    }
    for (int i = 0; i < CACHE_COUNT; i++) {
        Entry *e = &c->entries[i];
        if (e->renderer == r && !strcmp(e->source, path)) {
            e->age = ++c->age;
            return e->texture;
        }
    }
    SDL_Surface *surface = editor_image_decode(path);
    size_t bytes = surface ? (size_t)surface->w * surface->h * 4 : 0;
    while (c->bytes + bytes > CACHE_BYTES) {
        int victim = -1;
        for (int i = 0; i < CACHE_COUNT; i++) {
            if (c->entries[i].bytes && (victim < 0 || c->entries[i].age < c->entries[victim].age)) {
                victim = i;
            }
        }
        if (victim < 0) {
            break;
        }
        evict(c, victim);
    }
    int at = oldest(c);
    evict(c, at);
    Entry *e = &c->entries[at];
    e->texture = surface ? SDL_CreateTextureFromSurface(r, surface) : NULL;
    SDL_DestroySurface(surface);
    snprintf(e->source, sizeof e->source, "%s", path);
    e->renderer = r;
    e->age = ++c->age;
    e->bytes = e->texture ? bytes : 0;
    c->bytes += e->bytes;
    if (e->texture) {
        SDL_SetTextureBlendMode(e->texture, SDL_BLENDMODE_BLEND);
    }
    return e->texture;
}

void vector_image_draw(VectorImageRenderer *c, SDL_Renderer *r, const VectorShape *s, PUIRect local,
                       float angle, SDL_FRect board, float zoom) {
    SDL_FRect rect = {board.x + local.x * zoom, board.y + local.y * zoom, local.w * zoom,
                      local.h * zoom};
    SDL_Texture *t = texture(c, r, s->image.source);
    if (t) {
        SDL_FlipMode flip = (SDL_FlipMode)((s->image.flip_x ? SDL_FLIP_HORIZONTAL : 0) |
                                           (s->image.flip_y ? SDL_FLIP_VERTICAL : 0));
        SDL_RenderTextureRotated(r, t, NULL, &rect, angle, NULL, flip);
    } else {
        SDL_SetRenderDrawColor(r, 160, 80, 70, 255);
        SDL_RenderRect(r, &rect);
        SDL_RenderLine(r, rect.x, rect.y, rect.x + rect.w, rect.y + rect.h);
        SDL_RenderLine(r, rect.x + rect.w, rect.y, rect.x, rect.y + rect.h);
    }
}
