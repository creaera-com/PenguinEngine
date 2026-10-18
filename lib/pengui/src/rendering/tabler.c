/** @file @brief Complete embedded Tabler catalog and lazy, tinted PNG page cache. */
#include "internal.h"
#include "tabler_atlas.h"
#include <math.h>
#include <stdio.h>
#include <string.h>

extern const unsigned char pui_tabler_png_data[];

PUIIcon pui_tabler_from_name(const char *name) {
    if (!strncmp(name, "outline/", 8)) {
        name += 8;
    }
    size_t first = 0, end = sizeof tabler_catalog / sizeof *tabler_catalog;
    while (first < end) {
        size_t middle = first + (end - first) / 2;
        int comparison = strcmp(name, tabler_catalog[middle].name);
        if (!comparison) {
            return (PUIIcon)(ICON_TABLER_BASE + middle);
        }
        if (comparison < 0) {
            end = middle;
        } else {
            first = middle + 1;
        }
    }
    return ICON_NONE;
}

static SDL_Texture *page_texture(SDL_Renderer *renderer, unsigned page) {
    char key[64];
    snprintf(key, sizeof key, "pengui.tabler.page.%u", page);
    SDL_PropertiesID properties = SDL_GetRendererProperties(renderer);
    SDL_Texture *texture = SDL_GetPointerProperty(properties, key, NULL);
    if (texture) {
        return texture;
    }
    texture = pui_image_load_memory(renderer, pui_tabler_png_data + tabler_pages[page].offset,
                                    tabler_pages[page].length);
    if (!texture) {
        return NULL;
    }
    SDL_SetTextureScaleMode(texture, SDL_SCALEMODE_LINEAR);
    if (!SDL_SetPointerProperty(properties, key, texture)) {
        SDL_DestroyTexture(texture);
        return NULL;
    }
    /* SDL releases renderer-owned textures with their renderer, like the text atlas. */
    return texture;
}

void pui_tabler_draw(SDL_Renderer *renderer, PUIIcon icon, float x, float y, float size,
                     Uint32 rgba) {
    unsigned index = (unsigned)icon - ICON_TABLER_BASE;
    if (!renderer || index >= sizeof tabler_catalog / sizeof *tabler_catalog || !isfinite(size) ||
        size <= 0) {
        return;
    }
    SDL_Texture *texture = page_texture(renderer, tabler_catalog[index].page);
    if (texture) {
        SDL_SetTextureColorMod(texture, (Uint8)(rgba >> 24), (Uint8)(rgba >> 16),
                               (Uint8)(rgba >> 8));
        SDL_SetTextureAlphaMod(texture, (Uint8)rgba);
        unsigned slot = tabler_catalog[index].slot;
        SDL_FRect source = {(slot % 4) * 64.f, (slot / 4) * 64.f, 64, 64};
        SDL_FRect target = {x, y, size, size};
        SDL_RenderTexture(renderer, texture, &source, &target);
    }
}
