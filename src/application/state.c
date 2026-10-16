#include "application/internal.h"
Editor app;

bool editor_map_active(void) {
    return app.workspace.loaded_view == UI_MAP_VIEW;
}

bool editor_scripts_active(void) {
    return app.workspace.loaded_view == UI_SCRIPT_VIEW;
}

PeDocument editor_active_document(void) {
    return app.workspace.backend ? app.workspace.backend->document() : (PeDocument){0};
}

bool editor_edit_pending(void) {
    return pe_document_status(editor_active_document()).pending;
}

bool editor_map_dirty(void) {
    return app.workspace.map ? pe_history_dirty(&app.workspace.map->history)
                             : app.workspace.session_dirty[0];
}

bool editor_script_dirty(void) {
    return app.workspace.script ? se_dirty(app.workspace.script) : app.workspace.session_dirty[1];
}

const EditorView *editor_view(UiId id) {
    for (int i = 0; i < editor_view_count; i++) {
        if (editor_views[i].id == id) {
            return &editor_views[i];
        }
    }

    return &editor_views[0];
}

void editor_message(const char *s) {
    char copy[256] = {0};
    snprintf(copy, sizeof copy, "%s", s);
    memcpy(app.shell.status, copy, sizeof copy);
    if (app.workspace.script) {
        memcpy(app.workspace.script->status, copy, sizeof copy);
    }
}

int editor_session_index(UiId id) {
    return id == UI_MAP_VIEW ? 0 : id == UI_SCRIPT_VIEW ? 1 : 2;
}

bool editor_vector_dirty(void) {
    return app.workspace.vector ? ve_dirty(&app.workspace.vector->history)
                                : app.workspace.session_dirty[2];
}

bool editor_playing(void) {
    return app.workspace.backend && app.workspace.backend->playing &&
           app.workspace.backend->playing();
}
