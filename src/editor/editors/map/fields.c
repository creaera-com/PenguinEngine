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
    if (!o) {
        editor_field_stop();
        return;
    }
    if (fabsf(value) > 8192 || ((app.field == FIELD_W || app.field == FIELD_H) && value <= 0)) {
        editor_message("Position: -8192..8192; size: >0..8192");
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
        default:
            break;
        }
        editor_commit();
        editor_field_stop();
    }
}
