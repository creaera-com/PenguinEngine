/** @file @brief Private rendering utilities, not part of the public API. */
#ifndef PENGUI_RENDERING_INTERNAL_H
#define PENGUI_RENDERING_INTERNAL_H
#include "pengui/pengui.h"

static inline SDL_FRect ui_frect(UiRect r) {
    return (SDL_FRect){r.x, r.y, r.w, r.h};
}

static inline void ui_render_color(SDL_Renderer *r, Uint32 c) {
    SDL_SetRenderDrawColor(r, (Uint8)(c >> 24), (Uint8)(c >> 16), (Uint8)(c >> 8), (Uint8)c);
}

UiIcon ui_icon_from_name(const char *name);
#endif
