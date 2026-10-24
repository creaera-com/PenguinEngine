#include "resources.h"
/** @file @brief Bounded supersampling with caller-owned replayable drawing commands. */
#include "pengdraw/pengdraw.h"
#include <math.h>
#include <stdlib.h>
#include <string.h>

enum { TILE = 1024, SAMPLES = 2 };

enum { CACHE_TILES = 16 };

#define CACHE_BYTES (64u * 1024u * 1024u)

typedef struct {
    PDrawRenderer drawing;
    SDL_Rect crop;
    SDL_FRect bounds;
    SDL_Texture *destination;
    uint64_t revision, used;
} CachedTile;

typedef struct {
    CachedTile entries[CACHE_TILES];
    uint64_t clock;
    uint64_t hits, misses, repaints, evictions;
} TileCache;

typedef struct {
    SDL_Texture *target;
    SDL_Rect viewport, clip;
    bool viewport_set, clipped;
    float sx, sy;
    Uint8 red, green, blue, alpha;
    SDL_BlendMode blend;
} RenderState;

static RenderState capture(SDL_Renderer *r) {
    RenderState s = {.target = SDL_GetRenderTarget(r),
                     .viewport_set = SDL_RenderViewportSet(r),
                     .clipped = SDL_RenderClipEnabled(r)};
    SDL_GetRenderViewport(r, &s.viewport);
    SDL_GetRenderClipRect(r, &s.clip);
    SDL_GetRenderScale(r, &s.sx, &s.sy);
    SDL_GetRenderDrawColor(r, &s.red, &s.green, &s.blue, &s.alpha);
    SDL_GetRenderDrawBlendMode(r, &s.blend);
    return s;
}

static void restore(SDL_Renderer *r, const RenderState *s) {
    SDL_SetRenderTarget(r, s->target);
    SDL_SetRenderScale(r, s->sx, s->sy);
    SDL_SetRenderViewport(r, s->viewport_set ? &s->viewport : NULL);
    SDL_SetRenderClipRect(r, s->clipped ? &s->clip : NULL);
    SDL_SetRenderDrawColor(r, s->red, s->green, s->blue, s->alpha);
    SDL_SetRenderDrawBlendMode(r, s->blend);
}

void pdraw_renderer_destroy(PDrawRenderer *c) {
    if (!c) {
        return;
    }
    if (c->tiles) {
        TileCache *tiles = c->tiles;
        for (int i = 0; i < CACHE_TILES; i++) {
            pdraw_renderer_destroy(&tiles->entries[i].drawing);
        }
        free(tiles);
    }
    SDL_DestroyTexture(c->target);
    *c = (PDrawRenderer){0};
}

static bool prepare(PDrawRenderer *c, SDL_Renderer *r, int w, int h) {
    if (c->renderer && c->renderer != r) {
        pdraw_renderer_destroy(c);
    }
    c->renderer = r;
    w *= SAMPLES;
    h *= SAMPLES;
    if (c->target && c->width >= w && c->height >= h) {
        return true;
    }
    if (w < c->width) {
        w = c->width;
    }
    if (h < c->height) {
        h = c->height;
    }
    SDL_DestroyTexture(c->target);
    c->width = w;
    c->height = h;
    c->target = pdraw_create_texture(r, SDL_PIXELFORMAT_RGBA8888, SDL_TEXTUREACCESS_TARGET, w, h,
                                     PDRAW_TEXTURE_ARTWORK);
    if (!c->target || !SDL_SetTextureScaleMode(c->target, SDL_SCALEMODE_LINEAR) ||
        !SDL_SetTextureBlendMode(c->target, SDL_BLENDMODE_BLEND_PREMULTIPLIED)) {
        SDL_DestroyTexture(c->target);
        c->target = NULL;
        return false;
    }
    return true;
}

static void tile(PDrawRenderer *c, SDL_Renderer *r, SDL_Rect crop, const RenderState *previous,
                 PDrawPaint paint, void *context) {
    bool ok = SDL_SetRenderTarget(r, c->target);
    if (ok) {
        SDL_SetRenderScale(r, 1, 1);
        SDL_SetRenderViewport(r, NULL);
        SDL_SetRenderClipRect(r, NULL);
        SDL_SetRenderDrawColor(r, 0, 0, 0, 0);
        SDL_SetRenderDrawBlendMode(r, SDL_BLENDMODE_NONE);
        SDL_RenderFillRect(r, &(SDL_FRect){0, 0, crop.w * SAMPLES, crop.h * SAMPLES});
        SDL_Rect clip = {0, 0, crop.w * SAMPLES, crop.h * SAMPLES};
        SDL_SetRenderClipRect(r, &clip);
        paint(context, r, -(float)crop.x * SAMPLES, -(float)crop.y * SAMPLES, SAMPLES);
    }
    restore(r, previous);
    if (ok) {
        SDL_FRect src = {0, 0, crop.w * SAMPLES, crop.h * SAMPLES};
        SDL_FRect dst = {crop.x, crop.y, crop.w, crop.h};
        SDL_RenderTexture(r, c->target, &src, &dst);
    } else {
        SDL_SetRenderClipRect(r, &crop);
        paint(context, r, 0, 0, 1);
        restore(r, previous);
    }
}

void pdraw_render(PDrawRenderer *cache, SDL_Renderer *r, SDL_FRect bounds, PDrawPaint paint,
                  void *context) {
    if (!r || !paint) {
        return;
    }
    RenderState previous = capture(r);
    int width, height;
    if (!SDL_GetCurrentRenderOutputSize(r, &width, &height) || bounds.w <= 0 || bounds.h <= 0) {
        return;
    }
    /* Native-coordinate draws are the artwork contract. Leave unusual caller transforms intact. */
    if (previous.sx != 1 || previous.sy != 1 || previous.viewport_set) {
        paint(context, r, 0, 0, 1);
        restore(r, &previous);
        return;
    }
    float left = fmaxf(0, bounds.x), top = fmaxf(0, bounds.y);
    float right = fminf(width, bounds.x + bounds.w), bottom = fminf(height, bounds.y + bounds.h);
    if (previous.clipped) {
        left = fmaxf(left, previous.clip.x);
        top = fmaxf(top, previous.clip.y);
        right = fminf(right, previous.clip.x + previous.clip.w);
        bottom = fminf(bottom, previous.clip.y + previous.clip.h);
    }
    if (right <= left || bottom <= top) {
        return;
    }
    SDL_Rect crop = {(int)floorf(left), (int)floorf(top), (int)ceilf(right) - (int)floorf(left),
                     (int)ceilf(bottom) - (int)floorf(top)};
    PDrawRenderer temporary = {0};
    PDrawRenderer *c = cache ? cache : &temporary;
    int w = crop.w < TILE ? crop.w : TILE, h = crop.h < TILE ? crop.h : TILE;
    if (!prepare(c, r, w, h)) {
        paint(context, r, 0, 0, 1);
    } else {
        for (int y = crop.y; y < crop.y + crop.h; y += TILE) {
            for (int x = crop.x; x < crop.x + crop.w; x += TILE) {
                SDL_Rect part = {x, y, crop.x + crop.w - x, crop.y + crop.h - y};
                if (part.w > TILE) {
                    part.w = TILE;
                }
                if (part.h > TILE) {
                    part.h = TILE;
                }
                tile(c, r, part, &previous, paint, context);
            }
        }
    }
    restore(r, &previous);
    if (!cache) {
        pdraw_renderer_destroy(&temporary);
    }
}

static size_t tile_bytes(const CachedTile *entry) {
    return (size_t)entry->drawing.width * entry->drawing.height * 4;
}

static CachedTile *cached_tile(TileCache *cache, SDL_Renderer *r, SDL_Rect crop, SDL_FRect bounds,
                               SDL_Texture *destination, uint64_t revision, bool *repaint) {
    size_t bytes = 0;
    CachedTile *oldest = &cache->entries[0];
    for (int i = 0; i < CACHE_TILES; i++) {
        CachedTile *entry = &cache->entries[i];
        if (entry->drawing.target && entry->drawing.renderer == r &&
            entry->destination == destination && !memcmp(&entry->crop, &crop, sizeof crop) &&
            !memcmp(&entry->bounds, &bounds, sizeof bounds)) {
            /* Content revisions replace this tile in place. Keeping each drag
             * frame as another cached image wastes textures and churns GPU allocations. */
            *repaint = entry->revision != revision;
            if (*repaint) {
                cache->repaints++;
            } else {
                cache->hits++;
            }
            entry->revision = revision;
            entry->used = ++cache->clock;
            return entry;
        }
        bytes += tile_bytes(entry);
        if (entry->used < oldest->used) {
            oldest = entry;
        }
    }
    *repaint = true;
    cache->misses++;
    size_t needed = (size_t)crop.w * crop.h * SAMPLES * SAMPLES * 4;
    /* Evict least-recently-used tiles until both the slot and byte budgets fit. */
    while (bytes - tile_bytes(oldest) + needed > CACHE_BYTES) {
        if (oldest->drawing.target) {
            cache->evictions++;
        }
        bytes -= tile_bytes(oldest);
        pdraw_renderer_destroy(&oldest->drawing);
        *oldest = (CachedTile){0};
        oldest = NULL;
        for (int i = 0; i < CACHE_TILES; i++) {
            CachedTile *entry = &cache->entries[i];
            if (entry->drawing.target && (!oldest || entry->used < oldest->used)) {
                oldest = entry;
            }
        }
        if (!oldest) {
            oldest = &cache->entries[0];
        }
    }
    if (oldest->drawing.target) {
        cache->evictions++;
    }
    pdraw_renderer_destroy(&oldest->drawing);
    *oldest = (CachedTile){.crop = crop,
                           .bounds = bounds,
                           .destination = destination,
                           .revision = revision,
                           .used = ++cache->clock};
    return oldest;
}

void pdraw_render_cached(PDrawRenderer *cache, SDL_Renderer *r, SDL_FRect bounds, uint64_t revision,
                         PDrawPaint paint, void *context) {
    if (!cache || !r || !paint) {
        pdraw_render(cache, r, bounds, paint, context);
        return;
    }
    if (cache->renderer && cache->renderer != r) {
        pdraw_renderer_destroy(cache);
    }
    cache->renderer = r;
    if (!cache->tiles) {
        cache->tiles = calloc(1, sizeof(TileCache));
    }
    RenderState previous = capture(r);
    int width, height;
    if (!cache->tiles || previous.sx != 1 || previous.sy != 1 || previous.viewport_set ||
        !SDL_GetCurrentRenderOutputSize(r, &width, &height)) {
        pdraw_render(cache, r, bounds, paint, context);
        return;
    }
    float left = fmaxf(0, bounds.x), top = fmaxf(0, bounds.y);
    float right = fminf(width, bounds.x + bounds.w), bottom = fminf(height, bounds.y + bounds.h);
    if (previous.clipped) {
        left = fmaxf(left, previous.clip.x);
        top = fmaxf(top, previous.clip.y);
        right = fminf(right, previous.clip.x + previous.clip.w);
        bottom = fminf(bottom, previous.clip.y + previous.clip.h);
    }
    if (right <= left || bottom <= top) {
        return;
    }
    int end_x = (int)ceilf(right), end_y = (int)ceilf(bottom);
    for (int y = (int)floorf(top); y < end_y; y += TILE) {
        for (int x = (int)floorf(left); x < end_x; x += TILE) {
            SDL_Rect crop = {x, y, end_x - x, end_y - y};
            if (crop.w > TILE) {
                crop.w = TILE;
            }
            if (crop.h > TILE) {
                crop.h = TILE;
            }
            bool repaint;
            CachedTile *entry =
                cached_tile(cache->tiles, r, crop, bounds, previous.target, revision, &repaint);
            if (repaint || !entry->drawing.target) {
                pdraw_render(&entry->drawing, r, (SDL_FRect){x, y, crop.w, crop.h}, paint, context);
            } else {
                SDL_FRect source = {0, 0, crop.w * SAMPLES, crop.h * SAMPLES};
                SDL_FRect destination = {x, y, crop.w, crop.h};
                SDL_RenderTexture(r, entry->drawing.target, &source, &destination);
            }
        }
    }
    restore(r, &previous);
}

PDrawCacheStats pdraw_renderer_cache_stats(const PDrawRenderer *cache) {
    PDrawCacheStats stats = {0};
    if (!cache) {
        return stats;
    }
    if (cache->target) {
        stats.scratch_bytes = (uint64_t)cache->width * cache->height * 4;
    }
    const TileCache *tiles = cache->tiles;
    if (tiles) {
        stats.hits = tiles->hits;
        stats.misses = tiles->misses;
        stats.repaints = tiles->repaints;
        stats.evictions = tiles->evictions;
        for (int i = 0; i < CACHE_TILES; i++) {
            if (tiles->entries[i].drawing.target) {
                stats.tiles++;
                stats.tile_bytes += tile_bytes(&tiles->entries[i]);
            }
        }
    }
    return stats;
}
