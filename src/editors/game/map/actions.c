#include "editors/game/map/internal.h"

void editor_map_action(const char *name) {
    if (!app.workspace.map) {
        return;
    }
    if (!strcmp(name, "scene.toggle_play")) {
        editor_play_toggle();
        return;
    }
    if (app.workspace.map->view.playing || editor_edit_pending() || !app.workspace.loaded_view) {
        return;
    }
    if (!strcmp(name, "scene.add_root")) {
        editor_scene_add(0);
    } else if (!strcmp(name, "scene.add_sibling")) {
        editor_scene_add(1);
    } else if (!strcmp(name, "scene.add_child")) {
        editor_scene_add(2);
    } else if (!strcmp(name, "scene.delete")) {
        editor_scene_delete();
    } else if (!strcmp(name, "map.import_tileset")) {
        editor_field_start(FIELD_IMPORT, "");
    } else if (!strcmp(name, "map.toggle_grid")) {
        app.workspace.map->view.grid = !app.workspace.map->view.grid;
    } else if (!strcmp(name, "map.toggle_collision")) {
        app.workspace.map->view.collision = !app.workspace.map->view.collision;
    } else {
        const char *names[] = {"map.select_brush", "map.select_eraser", "map.select_rectangle",
                               "map.select_fill", "map.select_entity"};
        for (int i = 0; i < 5; i++) {
            if (!strcmp(name, names[i])) {
                app.workspace.map->view.tool = i;
            }
        }
    }
}
