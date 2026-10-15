#include "app/editor_internal.h"

bool editor_yaml_boolean(const cJSON *value, bool fallback) {
    if (!value)
        return fallback;
    if (cJSON_IsBool(value))
        return cJSON_IsTrue(value);
    const char *name = pe_ui_string(value, "binding", "");
    if (!strcmp(name, "document.can_save"))
        return (app.map || app.script) && !app.playing && !editor_edit_pending();
    if (!strcmp(name, "document.can_undo"))
        return !app.playing && !editor_edit_pending() &&
               (app.script ? app.script->document.position > 0
                           : app.map && app.map->history.cursor > 0);
    if (!strcmp(name, "document.can_redo"))
        return !app.playing && !editor_edit_pending() &&
               (app.script ? app.script->document.position < app.script->document.count
                           : app.map && app.map->history.cursor < app.map->history.count);
    if (!strcmp(name, "map.grid_visible"))
        return app.map && app.grid;
    if (!strcmp(name, "map.collision_visible"))
        return app.map && app.collision;
    return false;
}

const char *editor_yaml_text(const cJSON *value, char *buffer, size_t size) {
    if (cJSON_IsString(value))
        return value->valuestring;
    const char *b = pe_ui_string(value, "binding", "");
    if (!strcmp(b, "editor.mode"))
        return app.playing ? "PLAY" : "EDIT";
    if (!strcmp(b, "document.save_state"))
        return (editor_scripts_active() ? editor_script_dirty() : editor_map_dirty()) ? "Modified"
               : app.script && !app.script->exists                                    ? "Untitled"
                                                                                      : "Saved";
    if (!strcmp(b, "editor.message"))
        return app.script ? app.script->status : app.status;
    if (!strcmp(b, "editor.details")) {
        if (app.script) {
            int l, c;
            pe_text_location(&app.script->document, app.script->document.cursor, &l, &c);
            snprintf(buffer, size, "JS | Ln %d, Col %d | Undo %d / %d", l + 1, c + 1,
                     app.script->document.position, app.script->document.count);
        } else if (app.map)
            snprintf(buffer, size, "Zoom %.0f%% | Tile %d | Undo %d / %d", app.camera.zoom * 100,
                     app.tile, app.map->history.cursor, app.map->history.count);
        else
            snprintf(buffer, size, "No editor loaded");
        return buffer;
    }
    return "";
}

void editor_yaml_action(const char *name) {
    if (!name)
        return;
    if (!strcmp(name, "document.save")) {
        if (!app.playing && !editor_edit_pending() && (app.map || app.script))
            editor_save();
        return;
    }
    if (!strcmp(name, "document.undo")) {
        if ((app.map || app.script) && !app.playing && !editor_edit_pending())
            editor_undo_active(false);
        return;
    }
    if (!strcmp(name, "document.redo")) {
        if ((app.map || app.script) && !app.playing && !editor_edit_pending())
            editor_undo_active(true);
        return;
    }
    if (editor_script_action(name))
        return;
    editor_map_action(name);
}

bool editor_yaml_action_enabled(const char *action) {
    if (!action || !strcmp(action, "scene.toggle_play"))
        return !editor_edit_pending();
    if (!strncmp(action, "map.", 4))
        return app.map && !app.playing && !editor_edit_pending();
    return true;
}

bool editor_yaml_action_active(const char *action) {
    if (!action)
        return false;
    if (!strcmp(action, "scene.toggle_play"))
        return app.playing;
    const char *names[] = {"map.select_brush", "map.select_eraser", "map.select_rectangle",
                           "map.select_fill", "map.select_entity"};
    for (int i = 0; i < 5; i++)
        if (!strcmp(action, names[i]))
            return app.map && app.tool == i;
    return false;
}
