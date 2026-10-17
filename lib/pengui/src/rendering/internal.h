/** @file @brief Private rendering utilities, not part of the public API. */
#ifndef PUI_RENDERING_INTERNAL_H
#define PUI_RENDERING_INTERNAL_H
#include "pengui/pengui.h"

static inline SDL_FRect pui_frect(PUIRect r) {
    return (SDL_FRect){r.x, r.y, r.w, r.h};
}

static inline void pui_render_color(SDL_Renderer *r, Uint32 c) {
    SDL_SetRenderDrawColor(r, (Uint8)(c >> 24), (Uint8)(c >> 16), (Uint8)(c >> 8), (Uint8)c);
}

PUIIcon pui_icon_from_name(const char *name);
#endif
