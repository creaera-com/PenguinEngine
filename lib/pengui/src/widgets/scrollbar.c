/** @file @brief SDL-free scrollbar geometry and dragging. */
#include "core/internal.h"
#include <string.h>

float pui_scroll_max(const PUIScroll *s) {
    return fmaxf(0, s->content - fmaxf(0, s->viewport));
}

void pui_scroll_clamp(PUIScroll *s) {
    s->offset = pui_clamp(s->offset, 0, pui_scroll_max(s));
}

void pui_scroll_by(PUIScroll *s, float delta) {
    if (isfinite(delta)) {
        s->offset += delta;
    }
    pui_scroll_clamp(s);
}

PUIRect pui_scroll_thumb(const PUIScroll *s, PUIRect r, PUIAxis axis) {
    float length = axis == PUI_AXIS_X ? r.w : r.h;
    float size = s->content > 0 ? length * fminf(1, s->viewport / s->content) : length;
    size = pui_clamp(size, fminf(20, length), length);
    float max = pui_scroll_max(s),
          position = max > 0 ? pui_clamp(s->offset, 0, max) / max * (length - size) : 0;
    if (axis == PUI_AXIS_X) {
        r.x += position;
        r.w = size;
    } else {
        r.y += position;
        r.h = size;
    }

    return r;
}

bool pui_scrollbar(PUIContext *c, PUIId id, PUIRect track, PUIScroll *s, PUIAxis axis) {
    pui_scroll_clamp(s);
    if (pui_scroll_max(s) <= 0) {
        if (c->active == id) {
            c->active = 0;
        }
        return false;
    }

    PUIRect thumb = pui_scroll_thumb(s, track, axis);
    float old = s->offset;
    if (pui_context_acquire(c, id, track)) {
        c->dragging = pui_contains(thumb, c->press_x, c->press_y);
        c->drag_origin = axis == PUI_AXIS_X ? c->press_x : c->press_y;
        c->drag_value = s->offset;
        if (!c->dragging) {
            float p = axis == PUI_AXIS_X ? c->press_x : c->press_y,
                  t = axis == PUI_AXIS_X ? thumb.x : thumb.y;
            pui_scroll_by(s, p < t ? -s->viewport : s->viewport);
        }
    }
    if (c->active == id && !c->blocked) {
        if (c->dragging) {
            float length = axis == PUI_AXIS_X ? track.w : track.h,
                  thumb_length = axis == PUI_AXIS_X ? thumb.w : thumb.h,
                  p = axis == PUI_AXIS_X ? c->x : c->y;
            if (length > thumb_length) {
                s->offset = pui_clamp(c->drag_value + (p - c->drag_origin) * pui_scroll_max(s) /
                                                          (length - thumb_length),
                                      0, pui_scroll_max(s));
            }
        }
        if (c->released) {
            c->active = 0;
        }
    }

    return old != s->offset;
}
