/** @file @brief Renderer-local logical texture accounting; never synchronizes or reads GPU data. */
#include "resources.h"
#include <stdlib.h>
#define LEDGER_PROPERTY "pengdraw.resources"
#define TEXTURE_PROPERTY "pengdraw.resources.texture"

typedef struct {
    PDrawResourceStats stats;
    size_t references; /* Renderer plus each live tracked texture. */
} Ledger;

typedef struct {
    Ledger *ledger;
    uint64_t bytes;
    PDrawTextureClass kind;
    bool active;
} TextureRecord;

static void unref(Ledger *ledger) {
    if (!--ledger->references) {
        free(ledger);
    }
}

static void SDLCALL release_ledger(void *context, void *value) {
    (void)context;
    unref(value);
}

static Ledger *ledger(SDL_Renderer *renderer, bool create) {
    if (!renderer) {
        return NULL;
    }
    SDL_PropertiesID properties = SDL_GetRendererProperties(renderer);
    Ledger *value = SDL_GetPointerProperty(properties, LEDGER_PROPERTY, NULL);
    if (!value && create) {
        value = calloc(1, sizeof *value);
        if (value) {
            value->references = 1;
            value->stats.available = value->stats.complete = true;
            /* SDL invokes the cleanup on failure as well. */
            if (!SDL_SetPointerPropertyWithCleanup(properties, LEDGER_PROPERTY, value,
                                                   release_ledger, NULL)) {
                return NULL;
            }
        }
    }
    return value;
}

static uint64_t *category(PDrawResourceStats *stats, PDrawTextureClass kind) {
    switch (kind) {
    case PDRAW_TEXTURE_ARTWORK:
        return &stats->artwork_bytes;
    case PDRAW_TEXTURE_SHADOW:
        return &stats->shadow_bytes;
    case PDRAW_TEXTURE_MASK:
        return &stats->mask_bytes;
    case PDRAW_TEXTURE_GRADIENT:
        return &stats->gradient_bytes;
    default:
        return &stats->auxiliary_bytes;
    }
}

static void SDLCALL release_texture(void *context, void *value) {
    (void)context;
    TextureRecord *texture = value;
    Ledger *owner = texture->ledger;
    if (texture->active) {
        owner->stats.textures_released++;
        owner->stats.live_textures--;
        owner->stats.live_bytes -= texture->bytes;
        *category(&owner->stats, texture->kind) -= texture->bytes;
    }
    free(texture);
    unref(owner);
}

static SDL_Texture *track(Ledger *owner, SDL_Texture *texture, PDrawTextureClass kind) {
    if (!owner) {
        return texture;
    }
    if (!texture) {
        owner->stats.texture_failures++;
        return NULL;
    }
    SDL_PropertiesID props = SDL_GetTextureProperties(texture);
    SDL_PixelFormat format = (SDL_PixelFormat)SDL_GetNumberProperty(
        props, SDL_PROP_TEXTURE_FORMAT_NUMBER, SDL_PIXELFORMAT_UNKNOWN);
    uint64_t width = (uint64_t)SDL_GetNumberProperty(props, SDL_PROP_TEXTURE_WIDTH_NUMBER, 0);
    uint64_t height = (uint64_t)SDL_GetNumberProperty(props, SDL_PROP_TEXTURE_HEIGHT_NUMBER, 0);
    TextureRecord *record = calloc(1, sizeof *record);
    if (!record) {
        owner->stats.complete = false;
        return texture;
    }
    record->ledger = owner;
    record->kind = kind;
    record->bytes = width * height * SDL_BYTESPERPIXEL(format);
    owner->references++;
    if (!SDL_SetPointerPropertyWithCleanup(props, TEXTURE_PROPERTY, record, release_texture,
                                           NULL)) {
        owner->stats.complete = false;
        return texture;
    }
    record->active = true;
    owner->stats.textures_created++;
    owner->stats.live_textures++;
    owner->stats.live_bytes += record->bytes;
    *category(&owner->stats, kind) += record->bytes;
    if (owner->stats.live_bytes > owner->stats.peak_bytes) {
        owner->stats.peak_bytes = owner->stats.live_bytes;
    }
    return texture;
}

SDL_Texture *pdraw_create_texture(SDL_Renderer *renderer, SDL_PixelFormat format,
                                  SDL_TextureAccess access, int width, int height,
                                  PDrawTextureClass kind) {
    Ledger *owner = ledger(renderer, true);
    return track(owner, SDL_CreateTexture(renderer, format, access, width, height), kind);
}

SDL_Texture *pdraw_create_texture_from_surface(SDL_Renderer *renderer, SDL_Surface *surface,
                                               PDrawTextureClass kind) {
    Ledger *owner = ledger(renderer, true);
    return track(owner, SDL_CreateTextureFromSurface(renderer, surface), kind);
}

void pdraw_record_fallback(SDL_Renderer *renderer, bool shadow) {
    Ledger *owner = ledger(renderer, true);
    if (owner) {
        if (shadow) {
            owner->stats.shadow_fallback_attempts++;
        } else {
            owner->stats.gradient_fallback_attempts++;
        }
    }
}

PDrawResourceStats pdraw_renderer_resource_stats(SDL_Renderer *renderer) {
    Ledger *owner = ledger(renderer, false);
    return owner ? owner->stats : (PDrawResourceStats){0};
}
