#include "application/internal.h"

static bool request_dirty(Request r);
static bool request_save(void);

bool editor_save(void) {
    return pe_document_save(editor_active_document());
}

void editor_select_workspace(UiId id) {
    if (app.shell.rendering && id != app.workspace.loaded_view) {
        app.workspace.pending_view = id;
        app.shell.workspaces.selected = app.workspace.loaded_view;
        return;
    }
    if (editor_edit_pending() || (editor_playing() && id != UI_MAP_VIEW)) {
        app.shell.workspaces.selected = app.workspace.loaded_view;
        return;
    }
    if (!editor_switch(editor_view(id)->id)) {
        app.shell.workspaces.selected = app.workspace.loaded_view;
        ui_focus(&app.shell.ui, app.workspace.script ? UI_SCRIPT_DOCUMENT : 0);
        if (app.workspace.script) {
            se_focus(app.workspace.script, app.platform.window, true);
        }
        return;
    }
    app.shell.workspaces.selected = app.workspace.loaded_view;
    ui_focus(&app.shell.ui, editor_scripts_active() ? UI_SCRIPT_DOCUMENT : 0);
    if (app.workspace.script) {
        se_focus(app.workspace.script, app.platform.window, !editor_playing());
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
        return editor_map_dirty() || editor_script_dirty() || editor_vector_dirty();
    }

    if (r == REQUEST_VECTOR_OPEN || r == REQUEST_VECTOR_NEW) {
        return editor_vector_dirty();
    }
    return editor_map_dirty();
}

static bool request_save(void) {
    if (app.shell.request == REQUEST_QUIT) {
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
        if (editor_vector_dirty()) {
            editor_select_workspace(UI_VECTOR_VIEW);
            if (!editor_vector_active() || !vector_save()) {
                return false;
            }
        }
        return true;
    }

    return editor_save();
}

void editor_undo_active(bool redo) {
    pe_document_undo(editor_active_document(), redo);
}

void editor_request(Request type, const char *path) {
    if (app.shell.rendering) {
        app.shell.deferred_request = type;
        snprintf(app.shell.deferred_path, sizeof app.shell.deferred_path, "%s", path ? path : "");
        return;
    }
    if (type == REQUEST_IMPORT) {
        editor_import_png(path);
        return;
    }
    if (type == REQUEST_VECTOR_OPEN || type == REQUEST_VECTOR_NEW) {
        editor_select_workspace(UI_VECTOR_VIEW);
        if (!editor_vector_active()) {
            return;
        }
    } else if (type == REQUEST_SCRIPT_OPEN) {
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
    if (editor_playing()) {
        return;
    }
    if (path) {
        if (strlen(path) >= sizeof app.shell.open_path) {
            editor_message("Path too long");
            return;
        }
        snprintf(app.shell.open_path, sizeof app.shell.open_path, "%s", path);
    }
    if (request_dirty(type)) {
        app.shell.request = type;
        return;
    }
    if (type == REQUEST_QUIT) {
        app.runtime.running = false;
    } else if (type == REQUEST_VECTOR_OPEN) {
        vector_open(app.shell.open_path);
    } else if (type == REQUEST_VECTOR_NEW) {
        vector_new();
    } else if (type == REQUEST_SCRIPT_OPEN) {
        if (se_open(&(*app.workspace.script), app.shell.open_path)) {
            editor_tab_select(true);
        } else {
            editor_message((*app.workspace.script).status);
        }
    } else {
        editor_load_scene(app.shell.open_path);
    }
}

void editor_request_finish(void) {
    Request r = app.shell.request;
    app.shell.request = REQUEST_NONE;
    if (r == REQUEST_QUIT) {
        app.runtime.running = false;
    } else if (r == REQUEST_VECTOR_OPEN) {
        vector_open(app.shell.open_path);
    } else if (r == REQUEST_VECTOR_NEW) {
        vector_new();
    } else if (r == REQUEST_OPEN) {
        editor_load_scene(app.shell.open_path);
    } else if (r == REQUEST_SCRIPT_OPEN) {
        if (se_open(&(*app.workspace.script), app.shell.open_path)) {
            editor_tab_select(true);
        } else {
            editor_message((*app.workspace.script).status);
        }
    }
}

void editor_process_deferred_ui(void) {
    app.shell.wheel_delta = 0;
    app.shell.rendering = false;
    if (app.shell.deferred_save) {
        app.shell.deferred_save = false;
        if (request_save()) {
            editor_request_finish();
        }
    }
    if (app.shell.deferred_request != REQUEST_NONE) {
        Request r = app.shell.deferred_request;
        app.shell.deferred_request = REQUEST_NONE;
        editor_request(r, *app.shell.deferred_path ? app.shell.deferred_path : NULL);
    }
    if (app.workspace.pending_view) {
        UiId id = app.workspace.pending_view;
        app.workspace.pending_view = 0;
        editor_select_workspace(id);
    }
}
