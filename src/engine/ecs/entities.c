#include "penguin/engine.h"
#include <math.h>
#include <stdlib.h>
#include <string.h>

PeEntity pe_entity_create(PeScene *s, uint32_t mask) {
    for (uint32_t i = 0; i < PE_ENTITIES; i++) {
        if (!s->objects[i].alive) {
            PeObject *o = &s->objects[i];
            uint32_t generation = o->generation;
            memset(o, 0, sizeof *o);
            o->generation = generation;
            o->alive = true;
            o->stable_id = s->next_id++;
            o->mask = mask;
            o->w = 12;
            o->h = 12;
            o->color = 0xffcc55ff;
            return (PeEntity){i, generation};
        }
    }

    return (PeEntity){UINT32_MAX, 0};
}

bool pe_entity_valid(const PeScene *s, PeEntity e) {
    return e.index < PE_ENTITIES && s->objects[e.index].alive &&
           s->objects[e.index].generation == e.generation;
}

PeObject *pe_entity_get(PeScene *s, PeEntity e) {
    return pe_entity_valid(s, e) ? &s->objects[e.index] : NULL;
}

bool pe_entity_destroy(PeScene *s, PeEntity e) {
    if (!pe_entity_valid(s, e)) {
        return false;
    }

    bool removed[PE_ENTITIES] = {false};
    removed[e.index] = true;
    for (int pass = 0; pass < PE_ENTITIES; pass++) {
        bool changed = false;
        for (uint32_t i = 0; i < PE_ENTITIES; i++) {
            if (!s->objects[i].alive || removed[i] || !s->objects[i].parent_id) {
                continue;
            }
            PeEntity parent = pe_entity_find(s, s->objects[i].parent_id);
            if (pe_entity_valid(s, parent) && removed[parent.index]) {
                removed[i] = true;
                changed = true;
            }
        }
        if (!changed) {
            break;
        }
    }
    for (int i = 0; i < PE_ENTITIES; i++) {
        if (removed[i]) {
            PeObject *o = &s->objects[i];
            o->alive = false;
            o->generation++;
            if (!o->generation) {
                o->generation = 1;
            }
        }
    }

    return true;
}

PeEntity pe_entity_find(const PeScene *s, uint32_t id) {
    for (uint32_t i = 0; i < PE_ENTITIES; i++) {
        if (s->objects[i].alive && s->objects[i].stable_id == id) {
            return (PeEntity){i, s->objects[i].generation};
        }
    }

    return (PeEntity){UINT32_MAX, 0};
}
