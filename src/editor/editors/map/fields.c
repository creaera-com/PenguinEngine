#include "app/editor_internal.h"

void editor_field_apply(void) {
    if (app.field == FIELD_IMPORT) {
        if (editor_import_png(app.input)) {
            editor_field_stop();
        }
        return;
    }
    if (app.field == FIELD_LAYER) {
        if (!*app.input || strlen(app.input) >= 32) {
            editor_message("Layer name must contain 1-31 bytes");
            return;
        }
        if (editor_begin()) {
            memcpy(app.map->scene.layers[app.layer].name, app.input, strlen(app.input) + 1);
            editor_commit();
            editor_field_stop();
        }
        return;
    }

    if (app.field == FIELD_COLOR) {
        PeObject *o = pe_entity_get(&app.map->scene, pe_entity_find(&app.map->scene, app.selected));
        if (!o || !(o->mask & PE_SPRITE)) {
            editor_field_stop();
            return;
        }
        const char *hex = app.input + (app.input[0] == '#');
        if (strlen(hex) != 8 || strspn(hex, "0123456789abcdefABCDEF") != 8) {
            editor_message("Enter 8 hexadecimal RGBA digits, e.g. C28E5CFF");
            return;
        }
        uint32_t color = (uint32_t)strtoul(hex, NULL, 16);
        if (editor_begin()) {
            o->color = color;
            editor_commit();
            editor_field_stop();
        }
        return;
    }

    char *end;
    errno = 0;
    float value = strtof(app.input, &end);
    if (errno || end == app.input || *end || !isfinite(value)) {
        editor_message("Enter a valid finite number");
        return;
    }
    if (app.field == FIELD_TILESIZE) {
        if (value < 1 || value > 128 || floorf(value) != value) {
            editor_message("Source tile size must be an integer from 1 to 128");
            return;
        }
        if (editor_set_tileset(app.map->scene.tileset, (int)value)) {
            editor_field_stop();
        }
        return;
    }

    PeObject *o = pe_entity_get(&app.map->scene, pe_entity_find(&app.map->scene, app.selected));
    uint32_t required = (app.field == FIELD_VX || app.field == FIELD_VY) ? PE_BODY : PE_TRANSFORM;
    if (!o || !(o->mask & required)) {
        editor_field_stop();
        return;
    }
    if (fabsf(value) > 8192 || ((app.field == FIELD_W || app.field == FIELD_H) && value <= 0)) {
        editor_message("Position/velocity: -8192..8192; size: >0..8192");
        return;
    }
    if (editor_begin()) {
        switch (app.field) {
        case FIELD_X:
            o->x = value;
            break;
        case FIELD_Y:
            o->y = value;
            break;
        case FIELD_W:
            o->w = value;
            break;
        case FIELD_H:
            o->h = value;
            break;
        case FIELD_VX:
            o->vx = value;
            break;
        case FIELD_VY:
            o->vy = value;
            break;
        default:
            break;
        }
        editor_commit();
        if (required == PE_TRANSFORM) {
            app.map->reveal_selected = true;
        }
        editor_field_stop();
    }
}
