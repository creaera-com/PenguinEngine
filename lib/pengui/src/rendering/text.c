/** @file @brief Font atlas, text drawing and text measurement. */
#include "rendering/internal.h"
#include <math.h>
#include <string.h>

#include "font_atlas.h"

static SDL_Texture *font_texture(SDL_Renderer *r) {
    SDL_PropertiesID props = SDL_GetRendererProperties(r);
    SDL_Texture *texture = SDL_GetPointerProperty(props, "pengui.font", NULL);
    if (texture) {
        return texture;
    }

    SDL_Surface *surface = SDL_CreateSurface(128, 256, SDL_PIXELFORMAT_RGBA32);
    if (!surface) {
        return NULL;
    }
    size_t pixel = 0;
    for (size_t i = 0; i < sizeof pui_font_runs; i += 2) {
        for (unsigned n = 0; n < pui_font_runs[i]; n++, pixel++) {
            Uint8 *p =
                (Uint8 *)surface->pixels + (pixel / 128) * surface->pitch + (pixel % 128) * 4;
            p[0] = p[1] = p[2] = 255;
            p[3] = pui_font_runs[i + 1];
        }
    }
    texture = SDL_CreateTextureFromSurface(r, surface);
    SDL_DestroySurface(surface);
    if (!texture) {
        return NULL;
    }
    SDL_SetTextureBlendMode(texture, SDL_BLENDMODE_BLEND);
    SDL_SetTextureScaleMode(texture, SDL_SCALEMODE_LINEAR);
    if (!SDL_SetPointerProperty(props, "pengui.font", texture)) {
        SDL_DestroyTexture(texture);
        return NULL;
    }

    return texture;
}

bool pui_text(SDL_Renderer *r, float x, float y, const char *text, Uint32 rgba) {
    return pui_text_scaled(r, x, y, text, rgba, 1);
}

bool pui_text_scaled(SDL_Renderer *r, float x, float y, const char *text, Uint32 rgba,
                     float scale) {
    if (!r || !text || !isfinite(scale) || scale <= 0) {
        return false;
    }

    SDL_Texture *texture = font_texture(r);
    if (!texture) {
        return false;
    }
    SDL_SetTextureColorMod(texture, (Uint8)(rgba >> 24), (Uint8)(rgba >> 16), (Uint8)(rgba >> 8));
    SDL_SetTextureAlphaMod(texture, (Uint8)rgba);
    const float origin = x;
    while (*text) {
        Uint32 code = SDL_StepUTF8(&text, NULL);
        if (code == '\n') {
            x = origin;
            y += PUI_FONT_HEIGHT * scale;
            continue;
        }
        if (code == '\r') {
            continue;
        }
        if (code == '\t') {
            x += PUI_FONT_WIDTH * 4 * scale;
            continue;
        }
        if (code < 32 || (code >= 127 && code < 160) || code > 255) {
            code = '?';
        }
        SDL_FRect src = {(float)(code % 16 * PUI_FONT_WIDTH), (float)(code / 16 * PUI_FONT_HEIGHT),
                         PUI_FONT_WIDTH, PUI_FONT_HEIGHT};
        SDL_FRect dst = {x, y, PUI_FONT_WIDTH * scale, PUI_FONT_HEIGHT * scale};
        if (!SDL_RenderTexture(r, texture, &src, &dst)) {
            return false;
        }
        x += PUI_FONT_WIDTH * scale;
    }

    return true;
}

float pui_text_width(const char *text) {
    float width = 0, longest = 0;
    if (!text) {
        return 0;
    }
    while (*text) {
        Uint32 code = SDL_StepUTF8(&text, NULL);
        if (code == '\n') {
            longest = fmaxf(longest, width);
            width = 0;
        } else if (code != '\r') {
            width += PUI_FONT_WIDTH * (code == '\t' ? 4 : 1);
        }
    }

    return fmaxf(longest, width);
}
