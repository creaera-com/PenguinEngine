#include "editors/game/map/internal.h"

void editor_map_action(const char *name) {
    if (!pe_map_active_instance()) {
        return;
    }
    if (!strcmp(name, "scene.toggle_play")) {
        editor_play_toggle();
        return;
    }
    if (pe_map_active_instance()->view.playing || editor_edit_pending() ||
        !app.workspace.loaded_view) {
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
        pe_map_active_instance()->view.grid = !pe_map_active_instance()->view.grid;
    } else if (!strcmp(name, "map.toggle_collision")) {
        pe_map_active_instance()->view.collision = !pe_map_active_instance()->view.collision;
    } else {
        const char *names[] = {"map.select_brush", "map.select_eraser", "map.select_rectangle",
                               "map.select_fill", "map.select_entity"};
        for (int i = 0; i < 5; i++) {
            if (!strcmp(name, names[i])) {
                pe_map_active_instance()->view.tool = i;
            }
        }
    }
}
