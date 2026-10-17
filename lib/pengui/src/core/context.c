/** @file @brief Shared pointer capture, focus, clipping and drag services; no SDL. */
#include "core/internal.h"
#include <string.h>

UiId ui_id(const char *s) {
    uint32_t hash = 2166136261u;
    while (*s) {
        hash ^= (unsigned char)*s++;
        hash *= 16777619u;
    }

    return hash ? hash : 1;
}

void ui_init(UiContext *c) {
    memset(c, 0, sizeof *c);
}

bool ui_contains(UiRect r, float x, float y) {
    return r.w > 0 && r.h > 0 && x >= r.x && y >= r.y && x < r.x + r.w && y < r.y + r.h;
}

bool ui_context_visible(const UiContext *c, float x, float y) {
    return !c->clip_depth || ui_contains(c->clips[c->clip_depth - 1], x, y);
}

bool ui_hovered(const UiContext *c, UiRect r) {
    return !c->blocked && ui_context_visible(c, c->x, c->y) && ui_contains(r, c->x, c->y);
}

void ui_pointer(UiContext *c, float x, float y, bool down) {
    if (!isfinite(x) || !isfinite(y)) {
        return;
    }
    c->x = x;
    c->y = y;
    if (down && !c->down) {
        c->pressed = true;
        c->press_x = x;
        c->press_y = y;
    }
    if (!down && c->down) {
        c->released = true;
        c->release_x = x;
        c->release_y = y;
    }
    c->down = down;
}

void ui_cancel(UiContext *c) {
    c->active = 0;
    c->down = c->pressed = c->released = c->activate = c->dragging = false;
}

void ui_focus(UiContext *c, UiId id) {
    c->focus = id;
}

void ui_activate(UiContext *c) {
    c->activate = true;
}

void ui_end_frame(UiContext *c) {
    if (c->active && !c->active_seen) {
        c->active = 0;
    }
    c->pressed = c->released = c->activate = c->active_seen = false;
    c->clip_depth = 0;
}

bool ui_clip_push(UiContext *c, UiRect r) {
    if (c->clip_depth == UI_MAX_CLIPS) {
        return false;
    }
    if (c->clip_depth) {
        UiRect p = c->clips[c->clip_depth - 1];
        float x = fmaxf(r.x, p.x), y = fmaxf(r.y, p.y);
        r = (UiRect){x, y, fmaxf(0, fminf(r.x + r.w, p.x + p.w) - x),
                     fmaxf(0, fminf(r.y + r.h, p.y + p.h) - y)};
    }
    c->clips[c->clip_depth++] = r;
    return true;
}

void ui_clip_pop(UiContext *c) {
    if (c->clip_depth) {
        c->clip_depth--;
    }
}

bool ui_context_acquire(UiContext *c, UiId id, UiRect r) {
    if (c->blocked || !id) {
        return false;
    }
    if (c->active == id) {
        c->active_seen = true;
    }
    if (c->pressed && !c->active && ui_context_visible(c, c->press_x, c->press_y) &&
        ui_contains(r, c->press_x, c->press_y)) {
        c->active = id;
        c->pressed = false;
        c->active_seen = true;
        c->dragging = false;
        return true;
    }

    return false;
}

bool ui_drag_value(UiContext *c, UiId id, UiRect r, float *value, float low, float high,
                   UiAxis axis, float scale) {
    if (high < low) {
        high = low;
    }
    *value = ui_clamp(*value, low, high);
    if (ui_context_acquire(c, id, r)) {
        c->drag_origin = axis == UI_AXIS_X ? c->press_x : c->press_y;
        c->drag_value = *value;
    }
    if (c->blocked || c->active != id) {
        return false;
    }

    float old = *value, position = axis == UI_AXIS_X ? c->x : c->y;
    *value = ui_clamp(c->drag_value + (position - c->drag_origin) * scale, low, high);
    if (c->released) {
        c->active = 0;
    }

    return old != *value;
}
