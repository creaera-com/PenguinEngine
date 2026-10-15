#include "penguin/engine.h"
#include <math.h>
#include <stdlib.h>
#include <string.h>

void pe_update(PeScene *s, float ix, float iy, float dt) {
    if (!isfinite(ix) || !isfinite(iy) || !isfinite(dt) || dt <= 0 || dt > .1f) {
        return;
    }
    ix = fmaxf(-1, fminf(1, ix));
    iy = fmaxf(-1, fminf(1, iy));
    float length = sqrtf(ix * ix + iy * iy);
    if (length > 1) {
        ix /= length;
        iy /= length;
    }
    for (int i = 0; i < PE_ENTITIES; i++) {
        PeObject *o = &s->objects[i];
        if (!o->alive || (o->mask & (PE_TRANSFORM | PE_BODY)) != (PE_TRANSFORM | PE_BODY)) {
            continue;
        }
        if (o->mask & PE_PLAYER) {
            o->vx = ix * 90;
            o->vy = iy * 90;
        }
        pe_move(s, o, o->vx * dt, o->vy * dt);
    }
}
