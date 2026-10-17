/** @file @brief Scrollbar track and thumb drawing. */
#include "view/internal.h"
#include <math.h>
#include <string.h>

void pui_draw_scrollbar(SDL_Renderer *r, const PUIContext *c, PUIId id, PUIRect track,
                        const PUIScroll *s, PUIAxis axis) {
    if (pui_scroll_max(s) <= 0) {
        return;
    }
    pui_panel(r, (SDL_FRect){track.x, track.y, track.w, track.h}, PUI_CANVAS);
    PUIRect thumb = pui_scroll_thumb(s, track, axis);
    pui_panel(r, (SDL_FRect){thumb.x, thumb.y, thumb.w, thumb.h},
              c->active == id         ? PUI_ACCENT
              : pui_hovered(c, thumb) ? PUI_HOVER
                                      : PUI_BORDER);
}
