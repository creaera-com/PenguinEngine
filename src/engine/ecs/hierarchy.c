#include "penguin/engine.h"
#include <math.h>

bool pe_entity_world_position(const PeScene *s, PeEntity entity, float *x, float *y) {
    float wx = 0, wy = 0;
    bool seen[PE_ENTITIES] = {false};
    while (pe_entity_valid(s, entity)) {
        const PeObject *o = &s->objects[entity.index];
        if (seen[entity.index] || !(o->mask & PE_TRANSFORM)) {
            return false;
        }
        seen[entity.index] = true;
        wx += o->x;
        wy += o->y;
        if (!isfinite(wx) || !isfinite(wy)) {
            return false;
        }
        if (!o->parent_id) {
            *x = wx;
            *y = wy;
            return true;
        }
        entity = pe_entity_find(s, o->parent_id);
    }
    return false;
}

bool pe_entity_set_world_position(PeScene *s, PeEntity entity, float x, float y) {
    float old_x, old_y, px = 0, py = 0;
    if (!isfinite(x) || !isfinite(y) || !pe_entity_world_position(s, entity, &old_x, &old_y)) {
        return false;
    }
    PeObject *o = pe_entity_get(s, entity);
    if (o->parent_id && !pe_entity_world_position(s, pe_entity_find(s, o->parent_id), &px, &py)) {
        return false;
    }
    if (!isfinite(x - px) || !isfinite(y - py)) {
        return false;
    }
    o->x = x - px;
    o->y = y - py;
    return true;
}

bool pe_entity_set_parent(PeScene *s, PeEntity entity, PeEntity parent, bool keep_world) {
    float x, y, px = 0, py = 0;
    if (!pe_entity_world_position(s, entity, &x, &y)) {
        return false;
    }
    uint32_t parent_id = 0;
    if (parent.index != UINT32_MAX || parent.generation != 0) {
        if (!pe_entity_world_position(s, parent, &px, &py)) {
            return false;
        }
        PeEntity ancestor = parent;
        for (int depth = 0; pe_entity_valid(s, ancestor) && depth < PE_ENTITIES; depth++) {
            if (ancestor.index == entity.index) {
                return false;
            }
            const PeObject *o = &s->objects[ancestor.index];
            ancestor = pe_entity_find(s, o->parent_id);
        }
        parent_id = s->objects[parent.index].stable_id;
    }
    if (keep_world && (!isfinite(x - px) || !isfinite(y - py))) {
        return false;
    }
    PeObject *o = pe_entity_get(s, entity);
    o->parent_id = parent_id;
    if (keep_world) {
        o->x = x - px;
        o->y = y - py;
    }
    return true;
}

bool pe_scene_hierarchy_valid(const PeScene *s) {
    for (uint32_t i = 0; i < PE_ENTITIES; i++) {
        const PeObject *o = &s->objects[i];
        if (!o->alive) {
            continue;
        }
        if (!o->stable_id || o->stable_id > 1000000 || !isfinite(o->x) || !isfinite(o->y)) {
            return false;
        }
        for (uint32_t j = 0; j < i; j++) {
            if (s->objects[j].alive && s->objects[j].stable_id == o->stable_id) {
                return false;
            }
        }
        if (o->parent_id || (o->mask & PE_TRANSFORM)) {
            float x, y;
            if (!pe_entity_world_position(s, (PeEntity){i, o->generation}, &x, &y)) {
                return false;
            }
        }
    }
    return true;
}
