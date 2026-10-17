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

bool editor_document_dirty(UiId id) {
    if (app.workspace.loaded_view == id) {
        return pe_document_status(editor_active_document()).dirty;
    }
    int index = editor_session_index(id);
    return index >= 0 && app.workspace.session_dirty[index];
}

bool editor_map_dirty(void) {
    return editor_document_dirty(UI_MAP_VIEW);
}

bool editor_script_dirty(void) {
    return editor_document_dirty(UI_SCRIPT_VIEW);
}

const EditorView *editor_view(UiId id) {
    const EditorView *view = pe_editor_registry_find(editor_registry, id);
    return view ? view : editor_views[0];
}

void editor_message(const char *s) {
    char copy[256] = {0};
    snprintf(copy, sizeof copy, "%s", s);
    memcpy(app.shell.status, copy, sizeof copy);
    if (app.workspace.backend && app.workspace.backend->notify) {
        app.workspace.backend->notify(copy);
    }
}

int editor_session_index(UiId id) {
    return pe_editor_registry_index(editor_registry, id);
}

bool editor_vector_dirty(void) {
    return editor_document_dirty(UI_VECTOR_VIEW);
}

bool editor_playing(void) {
    return app.workspace.backend && app.workspace.backend->playing &&
           app.workspace.backend->playing();
}
