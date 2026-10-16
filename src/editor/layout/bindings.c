#include "editor/host.h"

bool editor_yaml_boolean(const cJSON *value, bool fallback) {
    if (!value) {
        return fallback;
    }
    if (cJSON_IsBool(value)) {
        return cJSON_IsTrue(value);
    }

    const char *name = pe_ui_string(value, "binding", "");
    PeDocumentStatus status = pe_document_status(editor_active_document());
    bool available = status.loaded && !editor_playing() && !status.pending;
    if (!strcmp(name, "document.can_save")) {
        return available;
    }
    if (!strcmp(name, "document.can_undo")) {
        return available && status.can_undo;
    }
    if (!strcmp(name, "document.can_redo")) {
        return available && status.can_redo;
    }
    return app.workspace.backend && app.workspace.backend->boolean_binding &&
           app.workspace.backend->boolean_binding(name);
}

const char *editor_yaml_text(const cJSON *value, char *buffer, size_t size) {
    if (cJSON_IsString(value)) {
        return value->valuestring;
    }

    const char *b = pe_ui_string(value, "binding", "");
    if (!strcmp(b, "editor.mode")) {
        return editor_playing() ? "PLAY" : "EDIT";
    }
    if (!strcmp(b, "document.save_state")) {
        PeDocumentStatus status = pe_document_status(editor_active_document());
        return status.dirty ? "Modified" : !status.exists && status.loaded ? "Untitled" : "Saved";
    }
    if (!strcmp(b, "editor.message")) {
        return app.workspace.backend && app.workspace.backend->message
                   ? app.workspace.backend->message()
                   : app.shell.status;
    }
    if (!strcmp(b, "editor.details")) {
        if (pe_document_status(editor_active_document()).loaded) {
            pe_document_describe(editor_active_document(), buffer, size);
        } else {
            snprintf(buffer, size, "No editor loaded");
        }
        return buffer;
    }

    return "";
}

void editor_yaml_action(const char *name) {
    if (!name) {
        return;
    }
    if (editor_window_action(name)) {
        return;
    }
    if (!strcmp(name, "document.save")) {
        if (!editor_playing() && !editor_edit_pending() &&
            pe_document_status(editor_active_document()).loaded) {
            editor_save();
        }
        return;
    }
    if (!strcmp(name, "document.undo")) {
        if (pe_document_status(editor_active_document()).loaded && !editor_playing() &&
            !editor_edit_pending()) {
            editor_undo_active(false);
        }
        return;
    }
    if (!strcmp(name, "document.redo")) {
        if (pe_document_status(editor_active_document()).loaded && !editor_playing() &&
            !editor_edit_pending()) {
            editor_undo_active(true);
        }
        return;
    }
    if (app.workspace.backend && app.workspace.backend->action) {
        app.workspace.backend->action(name);
    }
}

bool editor_yaml_action_enabled(const char *action) {
    if (!action) {
        return true;
    }
    if (app.workspace.backend && app.workspace.backend->action_enabled) {
        return app.workspace.backend->action_enabled(action);
    }
    return true;
}

bool editor_yaml_action_active(const char *action) {
    return action && app.workspace.backend && app.workspace.backend->action_active &&
           app.workspace.backend->action_active(action);
}
