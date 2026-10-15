#include "penguin/engine.h"
#include <math.h>
#include <stdlib.h>
#include <string.h>

bool pe_collides(const PeScene *s, float x, float y, float w, float h) {
    if (w <= 0 || h <= 0 || !isfinite(w) || !isfinite(h) || !isfinite(x) || !isfinite(y)) {
        return true;
    }
    if (x < 0 || y < 0 || x + w > s->width * s->tile_size || y + h > s->height * s->tile_size) {
        return true;
    }

    int x0 = (int)floorf(x / s->tile_size), y0 = (int)floorf(y / s->tile_size);
    int x1 = (int)ceilf((x + w) / s->tile_size) - 1, y1 = (int)ceilf((y + h) / s->tile_size) - 1;
    for (int py = y0; py <= y1; py++) {
        for (int px = x0; px <= x1; px++) {
            if (s->solid[py * PE_MAP_MAX + px]) {
                return true;
            }
        }
    }

    return false;
}

void pe_move(PeScene *s, PeObject *o, float dx, float dy) {
    /* Substeps prevent tunneling through tiles. Axis separation permits wall sliding. */
    if (!isfinite(dx) || !isfinite(dy)) {
        return;
    }

    float requested = ceilf(fmaxf(fabsf(dx), fabsf(dy)) / fmaxf(1, s->tile_size * .25f));
    if (requested > 4096) {
        return;
    }

    int steps = (int)requested;
    if (steps < 1) {
        steps = 1;
    }
    for (int i = 0; i < steps; i++) {
        if (!pe_collides(s, o->x + dx / steps, o->y, o->w, o->h)) {
            o->x += dx / steps;
        }
        if (!pe_collides(s, o->x, o->y + dy / steps, o->w, o->h)) {
            o->y += dy / steps;
        }
    }
}
