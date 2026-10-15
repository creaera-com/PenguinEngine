#include "app/editor_internal.h"

static bool request_dirty(Request r);
static bool request_save(void);

bool editor_save(void) {
    if (editor_scripts_active()) {
        bool ok = se_save(&(*app.script));
        editor_message((*app.script).status);
        return ok;
    }

    return editor_save_scene();
}

void editor_select_workspace(UiId id) {
    if (app.rendering && id != app.loaded_view) {
        app.pending_view = id;
        app.workspaces.selected = app.loaded_view;
        return;
    }
    if (editor_edit_pending() || (app.playing && id != UI_MAP_VIEW)) {
        app.workspaces.selected = app.loaded_view;
        return;
    }
    if (!editor_switch(editor_view(id)->id)) {
        app.workspaces.selected = app.loaded_view;
        ui_focus(&app.ui, app.script ? UI_SCRIPT_DOCUMENT : 0);
        if (app.script) {
            se_focus(app.script, app.window, true);
        }
        return;
    }
    app.workspaces.selected = app.loaded_view;
    ui_focus(&app.ui, editor_scripts_active() ? UI_SCRIPT_DOCUMENT : 0);
    if (app.script) {
        se_focus(app.script, app.window, !app.playing);
    }
}

void editor_tab_select(bool scripts) {
    editor_select_workspace(scripts ? UI_SCRIPT_VIEW : UI_MAP_VIEW);
}

static bool request_dirty(Request r) {
    if (r == REQUEST_SCRIPT_OPEN) {
        return editor_script_dirty();
    }
    if (r == REQUEST_QUIT) {
        return editor_map_dirty() || editor_script_dirty();
    }

    return editor_map_dirty();
}

static bool request_save(void) {
    if (app.request == REQUEST_QUIT) {
        if (editor_map_dirty()) {
            editor_select_workspace(UI_MAP_VIEW);
            if (!editor_map_active() || !editor_save_scene()) {
                return false;
            }
        }
        if (editor_script_dirty()) {
            editor_select_workspace(UI_SCRIPT_VIEW);
            if (!editor_scripts_active() || !editor_save()) {
                return false;
            }
        }
        return true;
    }

    return editor_save();
}

void editor_undo_active(bool redo) {
    if (editor_scripts_active()) {
        bool ok =
            redo ? pe_text_redo(&(*app.script).document) : pe_text_undo(&(*app.script).document);
        if (ok) {
            se_changed(&(*app.script), app.viewport, false);
        }
    } else {
        if (redo) {
            pe_redo(&app.map->history, &app.map->scene);
        } else {
            pe_undo(&app.map->history, &app.map->scene);
        }
        editor_selection_sync();
    }
}

void editor_request(Request type, const char *path) {
    if (app.rendering) {
        app.deferred_request = type;
        snprintf(app.deferred_path, sizeof app.deferred_path, "%s", path ? path : "");
        return;
    }
    if (type == REQUEST_IMPORT) {
        editor_import_png(path);
        return;
    }
    if (type == REQUEST_SCRIPT_OPEN) {
        editor_select_workspace(UI_SCRIPT_VIEW);
        if (!editor_scripts_active()) {
            return;
        }
    } else if (type == REQUEST_OPEN) {
        editor_select_workspace(UI_MAP_VIEW);
        if (!editor_map_active()) {
            return;
        }
    }
    if (app.playing) {
        return;
    }
    if (path) {
        if (strlen(path) >= sizeof app.open_path) {
            editor_message("Path too long");
            return;
        }
        snprintf(app.open_path, sizeof app.open_path, "%s", path);
    }
    if (request_dirty(type)) {
        app.request = type;
        return;
    }
    if (type == REQUEST_QUIT) {
        app.running = false;
    } else if (type == REQUEST_SCRIPT_OPEN) {
        if (se_open(&(*app.script), app.open_path)) {
            editor_tab_select(true);
        } else {
            editor_message((*app.script).status);
        }
    } else {
        editor_load_scene(app.open_path);
    }
}

void editor_request_finish(void) {
    Request r = app.request;
    app.request = REQUEST_NONE;
    if (r == REQUEST_QUIT) {
        app.running = false;
    } else if (r == REQUEST_OPEN) {
        editor_load_scene(app.open_path);
    } else if (r == REQUEST_SCRIPT_OPEN) {
        if (se_open(&(*app.script), app.open_path)) {
            editor_tab_select(true);
        } else {
            editor_message((*app.script).status);
        }
    }
}

void editor_process_deferred_ui(void) {
    app.wheel_delta = 0;
    app.rendering = false;
    if (app.deferred_save) {
        app.deferred_save = false;
        if (request_save()) {
            editor_request_finish();
        }
    }
    if (app.deferred_request != REQUEST_NONE) {
        Request r = app.deferred_request;
        app.deferred_request = REQUEST_NONE;
        editor_request(r, *app.deferred_path ? app.deferred_path : NULL);
    }
    if (app.pending_view) {
        UiId id = app.pending_view;
        app.pending_view = 0;
        editor_select_workspace(id);
    }
}
