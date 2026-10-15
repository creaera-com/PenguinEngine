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
    /* Parents move first even when their slot follows their children. */
    bool updated[PE_ENTITIES] = {false};
    for (int pass = 0; pass < PE_ENTITIES; pass++) {
        bool progress = false;
        for (int i = 0; i < PE_ENTITIES; i++) {
            PeObject *o = &s->objects[i];
            if (!o->alive || updated[i]) {
                continue;
            }
            if (o->parent_id) {
                PeEntity parent = pe_entity_find(s, o->parent_id);
                if (!pe_entity_valid(s, parent) || !updated[parent.index]) {
                    continue;
                }
            }
            updated[i] = true;
            progress = true;
            if ((o->mask & (PE_TRANSFORM | PE_BODY)) != (PE_TRANSFORM | PE_BODY)) {
                continue;
            }
            if (o->mask & PE_PLAYER) {
                o->vx = ix * 90;
                o->vy = iy * 90;
            }
            pe_move(s, o, o->vx * dt, o->vy * dt);
        }
        if (!progress) {
            break;
        }
    }
}
