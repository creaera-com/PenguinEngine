/** @file @brief Basic surface drawing and render clipping. */
#include "rendering/internal.h"
#include <math.h>
#include <string.h>

void ui_panel(SDL_Renderer *r, SDL_FRect rect, Uint32 c) {
    ui_render_color(r, c);
    SDL_RenderFillRect(r, &rect);
}

void ui_apply_clip(SDL_Renderer *r, const UiContext *c) {
    if (c->clip_depth) {
        UiRect p = c->clips[c->clip_depth - 1];
        SDL_Rect clip = {(int)p.x, (int)p.y, (int)p.w, (int)p.h};
        SDL_SetRenderClipRect(r, &clip);
    } else {
        SDL_SetRenderClipRect(r, NULL);
    }
}
