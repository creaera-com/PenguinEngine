/** @file @brief SDL-free button hit testing and activation. */
#include "core/internal.h"
#include <string.h>

bool pui_button(PUIContext *c, PUIId id, PUIRect r, bool enabled) {
    if (!enabled || c->blocked) {
        if (c->active == id) {
            c->active = 0;
        }
        return false;
    }
    if (pui_context_acquire(c, id, r)) {
        c->focus = id;
    }
    if (c->released && c->active == id) {
        bool hit = pui_context_visible(c, c->release_x, c->release_y) &&
                   pui_contains(r, c->release_x, c->release_y);
        c->active = 0;
        return hit;
    }

    bool in_clip = true;
    if (c->clip_depth) {
        PUIRect p = c->clips[c->clip_depth - 1];
        in_clip = r.x + r.w > p.x && r.y + r.h > p.y && r.x < p.x + p.w && r.y < p.y + p.h &&
                  p.w > 0 && p.h > 0;
    }
    if (c->activate && c->focus == id && in_clip) {
        c->activate = false;
        return true;
    }

    return false;
}
