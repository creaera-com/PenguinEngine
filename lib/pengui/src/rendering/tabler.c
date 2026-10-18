/** @file @brief Embedded Tabler masks, name lookup and renderer-owned tinted texture cache. */
#include "internal.h"
#include "tabler_atlas.h"
#include <math.h>
#include <stdio.h>
#include <string.h>

PUIIcon pui_tabler_from_name(const char *name) {
    for (size_t i = 0; i < sizeof tabler_catalog / sizeof *tabler_catalog; i++) {
        if (!strcmp(name, tabler_catalog[i].name)) {
            return (PUIIcon)(ICON_TABLER_BASE + i);
        }
    }
    return ICON_NONE;
}

static SDL_Texture *icon_texture(SDL_Renderer *renderer, unsigned index) {
    char key[64];
    snprintf(key, sizeof key, "pengui.tabler.%u", index);
    SDL_PropertiesID properties = SDL_GetRendererProperties(renderer);
    SDL_Texture *texture = SDL_GetPointerProperty(properties, key, NULL);
    if (texture) {
        return texture;
    }
    SDL_Surface *surface = SDL_CreateSurface(64, 64, SDL_PIXELFORMAT_RGBA32);
    if (!surface) {
        return NULL;
    }
    unsigned pixel = 0, offset = tabler_catalog[index].offset;
    for (unsigned end = offset + tabler_catalog[index].length; offset < end; offset += 2) {
        for (unsigned run = 0; run < tabler_runs[offset]; run++, pixel++) {
            Uint8 *p = (Uint8 *)surface->pixels + (pixel / 64) * surface->pitch + (pixel % 64) * 4;
            p[0] = p[1] = p[2] = 255;
            p[3] = tabler_runs[offset + 1];
        }
    }
    texture = SDL_CreateTextureFromSurface(renderer, surface);
    SDL_DestroySurface(surface);
    if (!texture) {
        return NULL;
    }
    SDL_SetTextureBlendMode(texture, SDL_BLENDMODE_BLEND);
    SDL_SetTextureScaleMode(texture, SDL_SCALEMODE_LINEAR);
    if (!SDL_SetPointerProperty(properties, key, texture)) {
        SDL_DestroyTexture(texture);
        return NULL;
    }
    /* SDL destroys renderer-owned textures with their renderer, like the text atlas. */
    return texture;
}

void pui_tabler_draw(SDL_Renderer *renderer, PUIIcon icon, float x, float y, float size,
                     Uint32 rgba) {
    unsigned index = (unsigned)icon - ICON_TABLER_BASE;
    if (!renderer || index >= sizeof tabler_catalog / sizeof *tabler_catalog || !isfinite(size) ||
        size <= 0) {
        return;
    }
    SDL_Texture *texture = icon_texture(renderer, index);
    if (texture) {
        SDL_SetTextureColorMod(texture, (Uint8)(rgba >> 24), (Uint8)(rgba >> 16),
                               (Uint8)(rgba >> 8));
        SDL_SetTextureAlphaMod(texture, (Uint8)rgba);
        SDL_FRect rect = {x, y, size, size};
        SDL_RenderTexture(renderer, texture, NULL, &rect);
    }
}
