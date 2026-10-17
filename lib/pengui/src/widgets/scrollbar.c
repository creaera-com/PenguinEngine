/** @file @brief SDL-free scrollbar geometry and dragging. */
#include "core/internal.h"
#include <string.h>

float ui_scroll_max(const UiScroll *s) {
    return fmaxf(0, s->content - fmaxf(0, s->viewport));
}

void ui_scroll_clamp(UiScroll *s) {
    s->offset = ui_clamp(s->offset, 0, ui_scroll_max(s));
}

void ui_scroll_by(UiScroll *s, float delta) {
    if (isfinite(delta)) {
        s->offset += delta;
    }
    ui_scroll_clamp(s);
}

UiRect ui_scroll_thumb(const UiScroll *s, UiRect r, UiAxis axis) {
    float length = axis == UI_AXIS_X ? r.w : r.h;
    float size = s->content > 0 ? length * fminf(1, s->viewport / s->content) : length;
    size = ui_clamp(size, fminf(20, length), length);
    float max = ui_scroll_max(s),
          position = max > 0 ? ui_clamp(s->offset, 0, max) / max * (length - size) : 0;
    if (axis == UI_AXIS_X) {
        r.x += position;
        r.w = size;
    } else {
        r.y += position;
        r.h = size;
    }

    return r;
}

bool ui_scrollbar(UiContext *c, UiId id, UiRect track, UiScroll *s, UiAxis axis) {
    ui_scroll_clamp(s);
    if (ui_scroll_max(s) <= 0) {
        if (c->active == id) {
            c->active = 0;
        }
        return false;
    }

    UiRect thumb = ui_scroll_thumb(s, track, axis);
    float old = s->offset;
    if (ui_context_acquire(c, id, track)) {
        c->dragging = ui_contains(thumb, c->press_x, c->press_y);
        c->drag_origin = axis == UI_AXIS_X ? c->press_x : c->press_y;
        c->drag_value = s->offset;
        if (!c->dragging) {
            float p = axis == UI_AXIS_X ? c->press_x : c->press_y,
                  t = axis == UI_AXIS_X ? thumb.x : thumb.y;
            ui_scroll_by(s, p < t ? -s->viewport : s->viewport);
        }
    }
    if (c->active == id && !c->blocked) {
        if (c->dragging) {
            float length = axis == UI_AXIS_X ? track.w : track.h,
                  thumb_length = axis == UI_AXIS_X ? thumb.w : thumb.h,
                  p = axis == UI_AXIS_X ? c->x : c->y;
            if (length > thumb_length) {
                s->offset = ui_clamp(c->drag_value + (p - c->drag_origin) * ui_scroll_max(s) /
                                                         (length - thumb_length),
                                     0, ui_scroll_max(s));
            }
        }
        if (c->released) {
            c->active = 0;
        }
    }

    return old != s->offset;
}
