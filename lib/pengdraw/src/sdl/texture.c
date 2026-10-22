/** @file @brief Shared image and rasterized-text composition without asset or font ownership. */
#include "pengdraw/pengdraw.h"
#include <math.h>

bool pdraw_texture_draw(SDL_Renderer *r, SDL_Texture *t, const SDL_FRect *source,
                        const SDL_FRect *destination, double angle, SDL_FlipMode flip,
                        uint32_t tint) {
    if (!r || !t || !destination || destination->w <= 0 || destination->h <= 0 ||
        !isfinite(angle)) {
        return false;
    }
    Uint8 red, green, blue, alpha;
    SDL_GetTextureColorMod(t, &red, &green, &blue);
    SDL_GetTextureAlphaMod(t, &alpha);
    SDL_SetTextureColorMod(t, tint >> 24, tint >> 16, tint >> 8);
    SDL_SetTextureAlphaMod(t, tint);
    bool ok = SDL_RenderTextureRotated(r, t, source, destination, angle, NULL, flip);
    SDL_SetTextureColorMod(t, red, green, blue);
    SDL_SetTextureAlphaMod(t, alpha);
    return ok;
}
