#include "penguin/engine.h"

static bool known(uint32_t component) {
    return component == PE_TRANSFORM || component == PE_SPRITE || component == PE_BODY ||
           component == PE_PLAYER;
}

bool pe_entity_add_component(PeScene *s, PeEntity entity, uint32_t component) {
    PeObject *o = pe_entity_get(s, entity);
    if (!o || !known(component) || (o->mask & component)) {
        return false;
    }
    uint32_t added = component;
    if (component != PE_TRANSFORM) {
        added |= PE_TRANSFORM;
    }
    if (component == PE_PLAYER) {
        added |= PE_BODY;
    }
    added &= ~o->mask;
    if (added & PE_TRANSFORM) {
        o->x = o->y = 0;
        o->w = o->h = 12;
    }
    if (added & PE_SPRITE) {
        o->color = 0xffcc55ff;
    }
    if (added & PE_BODY) {
        o->vx = o->vy = 0;
    }
    o->mask |= added;
    return true;
}

bool pe_entity_component_removable(const PeScene *s, PeEntity entity, uint32_t component) {
    if (!known(component) || !pe_entity_valid(s, entity)) {
        return false;
    }
    const PeObject *o = &s->objects[entity.index];
    if (!(o->mask & component)) {
        return false;
    }
    if (component == PE_BODY && (o->mask & PE_PLAYER)) {
        return false;
    }
    if (component == PE_TRANSFORM) {
        if (o->parent_id || (o->mask & (PE_SPRITE | PE_BODY | PE_PLAYER))) {
            return false;
        }
        for (int i = 0; i < PE_ENTITIES; i++) {
            if (s->objects[i].alive && s->objects[i].parent_id == o->stable_id) {
                return false;
            }
        }
    }
    return true;
}

bool pe_entity_remove_component(PeScene *s, PeEntity entity, uint32_t component) {
    if (!pe_entity_component_removable(s, entity, component)) {
        return false;
    }
    s->objects[entity.index].mask &= ~component;
    return true;
}
