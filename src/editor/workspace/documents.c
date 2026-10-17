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
    bool switched = editor_switch(editor_view(id)->id);
    app.shell.workspaces.selected = app.workspace.loaded_view;
    ui_focus(&app.shell.ui, 0);
    if (app.workspace.backend && app.workspace.backend->focus) {
        app.workspace.backend->focus();
    }
    if (!switched) {
        return;
    }
}

void editor_tab_select(bool scripts) {
    editor_select_workspace(scripts ? UI_SCRIPT_VIEW : UI_MAP_VIEW);
}

static const EditorView *request_owner(Request command) {
    if (command == REQUEST_QUIT) {
        return NULL;
    }
    return pe_editor_registry_request(editor_registry, (int)command);
}

static bool request_dirty(Request r) {
    if (r == REQUEST_QUIT) {
        for (int i = 0; i < editor_view_count; i++) {
            if (editor_document_dirty(editor_views[i]->id)) {
                return true;
            }
        }
        return false;
    }
    const EditorView *owner = request_owner(r);
    return owner && editor_document_dirty(owner->id);
}

static bool request_save(void) {
    if (app.shell.request == REQUEST_QUIT) {
        for (int i = 0; i < editor_view_count; i++) {
            UiId id = editor_views[i]->id;
            if (editor_document_dirty(id)) {
                editor_select_workspace(id);
                if (app.workspace.loaded_view != id || !editor_save()) {
                    return false;
                }
            }
        }
        return true;
    }
    return editor_save();
}

static void perform_request(Request command) {
    if (command == REQUEST_QUIT) {
        app.runtime.running = false;
        return;
    }
    const EditorView *owner = request_owner(command);
    if (owner && owner->id == app.workspace.loaded_view && owner->backend->request) {
        owner->backend->request(command, app.shell.open_path);
    }
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
    const EditorView *owner = request_owner(type);
    if (owner && (int)type == owner->import_request) {
        if (owner->id == app.workspace.loaded_view && owner->backend->request) {
            owner->backend->request(type, path);
        }
        return;
    }
    if (owner) {
        editor_select_workspace(owner->id);
        if (app.workspace.loaded_view != owner->id) {
            return;
        }
    } else if (type != REQUEST_QUIT) {
        return;
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
    perform_request(type);
}

void editor_request_finish(void) {
    Request command = app.shell.request;
    app.shell.request = REQUEST_NONE;
    perform_request(command);
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
