#include "app/editor_internal.h"

void editor_map_action(const char *name) {
    if (!app.map) {
        return;
    }
    if (!strcmp(name, "scene.toggle_play")) {
        editor_play_toggle();
        return;
    }
    if (app.playing || editor_edit_pending() || !app.loaded_view) {
        return;
    }
    if (!strcmp(name, "map.import_tileset")) {
        editor_field_start(FIELD_IMPORT, "");
    } else if (!strcmp(name, "map.toggle_grid")) {
        app.grid = !app.grid;
    } else if (!strcmp(name, "map.toggle_collision")) {
        app.collision = !app.collision;
    } else {
        const char *names[] = {"map.select_brush", "map.select_eraser", "map.select_rectangle",
                               "map.select_fill", "map.select_entity"};
        for (int i = 0; i < 5; i++) {
            if (!strcmp(name, names[i])) {
                app.tool = i;
            }
        }
    }
}
