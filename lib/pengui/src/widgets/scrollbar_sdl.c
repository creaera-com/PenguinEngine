/** @file @brief Scrollbar track and thumb drawing. */
#include "view/internal.h"
#include <math.h>
#include <string.h>

void pui_draw_scrollbar(SDL_Renderer *r, const PUIContext *c, PUIId id, PUIRect track,
                        const PUIScroll *s, PUIAxis axis) {
    if (pui_scroll_max(s) <= 0) {
        return;
    }
    float radius = fminf(3, fminf(track.w, track.h) / 2);
    pui_rounded_panel(r, pui_frect(track), PUI_CANVAS, radius, false);
    PUIRect thumb = pui_scroll_thumb(s, track, axis);
    pui_rounded_panel(r, pui_frect(thumb),
                      c->active == id         ? PUI_ACCENT
                      : pui_hovered(c, thumb) ? PUI_HOVER
                                              : PUI_BORDER,
                      radius, false);
}
