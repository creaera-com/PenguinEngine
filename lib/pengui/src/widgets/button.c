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

static uint32_t interpolate(uint32_t a, uint32_t b, float t) {
    uint32_t out = 0;
    for (int shift = 0; shift <= 24; shift += 8) {
        float x = (a >> shift) & 255, y = (b >> shift) & 255;
        out |= (uint32_t)(x + (y - x) * t + .5f) << shift;
    }
    return out;
}

uint32_t pui_color_transition(PUIColorTransition *s, uint32_t target, double now, float duration) {
    if (!s->initialized || duration <= 0) {
        *s = (PUIColorTransition){.from = target,
                                  .target = target,
                                  .current = target,
                                  .started_ms = now,
                                  .initialized = true};
        return target;
    }
    float t = (float)((now - s->started_ms) / duration);
    t = t < 0 ? 0 : t > 1 ? 1 : t;
    t = t * t * (3 - 2 * t);
    s->current = interpolate(s->from, s->target, t);
    if (s->target != target) {
        s->from = s->current;
        s->target = target;
        s->started_ms = now;
    }
    return s->current;
}
