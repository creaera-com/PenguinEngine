/** @file @brief Shared pointer capture, focus, clipping and drag services; no SDL. */
#include "core/internal.h"
#include <string.h>

PUIId pui_id(const char *s) {
    uint32_t hash = 2166136261u;
    while (*s) {
        hash ^= (unsigned char)*s++;
        hash *= 16777619u;
    }

    return hash ? hash : 1;
}

void pui_init(PUIContext *c) {
    memset(c, 0, sizeof *c);
}

bool pui_contains(PUIRect r, float x, float y) {
    return r.w > 0 && r.h > 0 && x >= r.x && y >= r.y && x < r.x + r.w && y < r.y + r.h;
}

bool pui_context_visible(const PUIContext *c, float x, float y) {
    return !c->clip_depth || pui_contains(c->clips[c->clip_depth - 1], x, y);
}

bool pui_hovered(const PUIContext *c, PUIRect r) {
    return !c->blocked && pui_context_visible(c, c->x, c->y) && pui_contains(r, c->x, c->y);
}

void pui_pointer(PUIContext *c, float x, float y, bool down) {
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

void pui_cancel(PUIContext *c) {
    c->active = 0;
    c->down = c->pressed = c->released = c->activate = c->dragging = false;
}

void pui_focus(PUIContext *c, PUIId id) {
    c->focus = id;
}

void pui_activate(PUIContext *c) {
    c->activate = true;
}

void pui_end_frame(PUIContext *c) {
    if (c->active && !c->active_seen) {
        c->active = 0;
    }
    c->pressed = c->released = c->activate = c->active_seen = false;
    c->clip_depth = 0;
}

bool pui_clip_push(PUIContext *c, PUIRect r) {
    if (c->clip_depth == PUI_MAX_CLIPS) {
        return false;
    }
    if (c->clip_depth) {
        PUIRect p = c->clips[c->clip_depth - 1];
        float x = fmaxf(r.x, p.x), y = fmaxf(r.y, p.y);
        r = (PUIRect){x, y, fmaxf(0, fminf(r.x + r.w, p.x + p.w) - x),
                      fmaxf(0, fminf(r.y + r.h, p.y + p.h) - y)};
    }
    c->clips[c->clip_depth++] = r;
    return true;
}

void pui_clip_pop(PUIContext *c) {
    if (c->clip_depth) {
        c->clip_depth--;
    }
}

bool pui_context_acquire(PUIContext *c, PUIId id, PUIRect r) {
    if (c->blocked || !id) {
        return false;
    }
    if (c->active == id) {
        c->active_seen = true;
    }
    if (c->pressed && !c->active && pui_context_visible(c, c->press_x, c->press_y) &&
        pui_contains(r, c->press_x, c->press_y)) {
        c->active = id;
        c->pressed = false;
        c->active_seen = true;
        c->dragging = false;
        return true;
    }

    return false;
}

bool pui_drag_value(PUIContext *c, PUIId id, PUIRect r, float *value, float low, float high,
                    PUIAxis axis, float scale) {
    if (high < low) {
        high = low;
    }
    *value = pui_clamp(*value, low, high);
    if (pui_context_acquire(c, id, r)) {
        c->drag_origin = axis == PUI_AXIS_X ? c->press_x : c->press_y;
        c->drag_value = *value;
    }
    if (c->blocked || c->active != id) {
        return false;
    }

    float old = *value, position = axis == PUI_AXIS_X ? c->x : c->y;
    *value = pui_clamp(c->drag_value + (position - c->drag_origin) * scale, low, high);
    if (c->released) {
        c->active = 0;
    }

    return old != *value;
}
