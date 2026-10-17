/** @file @brief Basic surface drawing and render clipping. */
#include "rendering/internal.h"
#include <math.h>
#include <string.h>

void pui_panel(SDL_Renderer *r, SDL_FRect rect, Uint32 c) {
    pui_render_color(r, c);
    SDL_RenderFillRect(r, &rect);
}

void pui_apply_clip(SDL_Renderer *r, const PUIContext *c) {
    if (c->clip_depth) {
        PUIRect p = c->clips[c->clip_depth - 1];
        SDL_Rect clip = {(int)p.x, (int)p.y, (int)p.w, (int)p.h};
        SDL_SetRenderClipRect(r, &clip);
    } else {
        SDL_SetRenderClipRect(r, NULL);
    }
}
