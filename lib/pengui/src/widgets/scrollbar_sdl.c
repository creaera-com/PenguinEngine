/** @file @brief Scrollbar track and thumb drawing. */
#include "view/internal.h"
#include <math.h>
#include <string.h>

void ui_draw_scrollbar(SDL_Renderer *r, const UiContext *c, UiId id, UiRect track,
                       const UiScroll *s, UiAxis axis) {
    if (ui_scroll_max(s) <= 0) {
        return;
    }
    ui_panel(r, (SDL_FRect){track.x, track.y, track.w, track.h}, UI_CANVAS);
    UiRect thumb = ui_scroll_thumb(s, track, axis);
    ui_panel(r, (SDL_FRect){thumb.x, thumb.y, thumb.w, thumb.h},
             c->active == id        ? UI_ACCENT
             : ui_hovered(c, thumb) ? UI_HOVER
                                    : UI_BORDER);
}
