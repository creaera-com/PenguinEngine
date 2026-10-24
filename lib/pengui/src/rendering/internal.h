/** @file @brief Private rendering utilities, not part of the public API. */
#ifndef PUI_RENDERING_INTERNAL_H
#define PUI_RENDERING_INTERNAL_H
#include "pengui/pengui.h"

static inline SDL_FRect pui_frect(PUIRect r) {
    return (SDL_FRect){r.x, r.y, r.w, r.h};
}

PUIIcon pui_tabler_from_name(const char *name);
void pui_tabler_draw(SDL_Renderer *renderer, PUIIcon icon, float x, float y, float size,
                     Uint32 rgba);
/** @brief Filled rounded surface; upper_only keeps bottom corners square. */
void pui_rounded_panel(SDL_Renderer *renderer, SDL_FRect rect, Uint32 color, float radius,
                       bool upper_only);
#endif
