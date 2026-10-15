#include "penguin/engine.h"
#include <string.h>

bool pe_entity_name_valid(const char *name) {
    if (!name) {
        return false;
    }
    size_t n = 0;
    bool visible = false;
    while (n < PE_ENTITY_NAME && name[n]) {
        unsigned char c = (unsigned char)name[n++];
        if (c < 32 || c == 127) {
            return false;
        }
        visible = visible || c != ' ';
    }
    if (!visible || n >= PE_ENTITY_NAME) {
        return false;
    }
    for (size_t i = 0; i < n;) {
        unsigned c = (unsigned char)name[i++];
        if (c < 128) {
            continue;
        }
        int count = c >= 0xc2 && c <= 0xdf   ? 1
                    : c >= 0xe0 && c <= 0xef ? 2
                    : c >= 0xf0 && c <= 0xf4 ? 3
                                             : 0;
        if (!count || (size_t)count > n - i) {
            return false;
        }
        unsigned code = c & ((1u << (6 - count)) - 1);
        for (int j = 0; j < count; j++) {
            c = (unsigned char)name[i++];
            if ((c & 0xc0) != 0x80) {
                return false;
            }
            code = (code << 6) | (c & 63);
        }
        if ((count == 2 && code < 0x800) || (count == 3 && code < 0x10000) ||
            (code >= 0xd800 && code <= 0xdfff) || code > 0x10ffff) {
            return false;
        }
    }
    return true;
}

bool pe_entity_set_name(PeScene *scene, PeEntity entity, const char *name) {
    PeObject *o = pe_entity_get(scene, entity);
    if (!o || !pe_entity_name_valid(name) || !strcmp(o->name, name)) {
        return false;
    }
    char copy[PE_ENTITY_NAME] = {0};
    memcpy(copy, name, strlen(name));
    memcpy(o->name, copy, sizeof copy);
    return true;
}
