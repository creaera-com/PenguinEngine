/** @file @brief Vector editor lifecycle and routing to document, canvas and history modules. */
#include "artboard/tool.h"
#include "internal.h"
#include "ui/inspector.h"
#include "ui/toolbar.h"
#include "workspace/canvas.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void *vector_editor_create(const EditorServices *services, char *error, size_t capacity) {
    VectorEditor *editor = calloc(1, sizeof *editor);
    if (!editor) {
        snprintf(error, capacity, "could not allocate Vector editor");
        return NULL;
    }
    PUILayout *root = editor_load_ui(services, "editors/vector/ui/editor.yaml", error, capacity);
    if (!root) {
        free(editor);
        return NULL;
    }
    PUILayout *inspector = vector_inspector_load(services, error, capacity);
    if (inspector) {
        editor->layout = pui_layout_compose(root, "inspector_content", inspector, error, capacity);
    }
    pui_layout_free(inspector);
    pui_layout_free(root);
    vector_documents_init(&editor->documents);
    if (!editor->layout) {
        pui_layout_free(editor->inspector.artboard_layout);
        pui_layout_free(editor->layout);
        free(editor);
        return NULL;
    }
    editor->inspector.artboard_layout =
        editor_load_ui(services, "editors/vector/ui/artboard_inspector.yaml", error, capacity);
    if (!editor->inspector.artboard_layout) {
        vector_editor_destroy(editor);
        return NULL;
    }
    return editor;
}

const PUILayout *vector_editor_layout(const void *instance) {
    const VectorEditor *editor = instance;
    return editor->layout;
}

void vector_editor_destroy(void *instance) {
    VectorEditor *editor = instance;
    if (editor) {
        vector_workspace_cancel_inactive(&editor->documents, NULL);
        vector_documents_destroy(&editor->documents);
        pui_layout_free(editor->inspector.artboard_layout);
        pui_layout_free(editor->layout);
        free(editor);
    }
}

bool vector_editor_action(void *instance, PUIView *view, const char *name, const char *widget) {
    VectorEditor *editor = instance;
    if (vector_inspector_action(&editor->inspector, &editor->documents, view, name)) {
        return true;
    }
    if (!strcmp(name, "vector.add_document")) {
        vector_document_tabs_request(&editor->document_tabs, &editor->documents, view);
        return true;
    }
    VectorDocument *document = vector_document_tabs_active(&editor->documents, view);
    if (!strcmp(name, "vector.tool_selection")) {
        if (document) {
            vector_artboard_tool_cancel(document);
        }
        vector_toolbar_sync(view, document);
        return true;
    }
    if (vector_document_tree_action(document, view, name, widget)) {
        return true;
    }
    if (!strcmp(name, "vector.add_artboard") || !strcmp(name, "editor.undo") ||
        !strcmp(name, "editor.redo")) {
        if (!document) {
            vector_toolbar_sync(view, NULL);
            pui_view_set_text(view, "status", "Zuerst ein Dokument anlegen oder auswaehlen");
            return true;
        }
        if (!strcmp(name, "vector.add_artboard")) {
            if (document->artboard_tool.enabled) {
                vector_artboard_tool_cancel(document);
            } else {
                vector_artboard_tool_arm(document);
            }
            vector_toolbar_sync(view, document);
            pui_view_set_text(view, "status",
                              document->artboard_tool.enabled
                                  ? "Artboard aufziehen (linke Maustaste), Esc: abbrechen"
                                  : "Auswahlwerkzeug aktiv");
            return true;
        }
        vector_artboard_tool_cancel(document);
        vector_toolbar_sync(view, document);
        bool changed = !strcmp(name, "editor.undo")
                           ? vector_history_undo(&document->history, document)
                           : vector_history_redo(&document->history, document);
        if (changed) {
            vector_canvas_center(document);
        }
        pui_view_set_text(view, "status",
                          changed ? "Dokument aktualisiert" : "Keine Aenderung moeglich");
        return true;
    }
    return false;
}

void vector_editor_canvas(void *instance, SDL_Renderer *renderer, PUIRect area,
                          const char *widget) {
    VectorEditor *editor = instance;
    for (int i = 0; i < editor->documents.count; i++) {
        VectorDocument *document = &editor->documents.items[i];
        char id[96];
        snprintf(id, sizeof id, "vector_document_%u.canvas", document->id);
        if (!strcmp(id, widget)) {
            vector_workspace_cancel_inactive(&editor->documents, document);
            vector_canvas_draw(document, renderer, area);
            return;
        }
    }
}

bool vector_editor_event(void *instance, PUIView *view, const SDL_Event *event) {
    VectorEditor *editor = instance;
    VectorDocument *document = vector_document_tabs_active(&editor->documents, view);
    vector_workspace_cancel_inactive(&editor->documents, document);
    bool consumed = document && vector_canvas_event(document, view, event);
    vector_toolbar_sync(view, document);
    return consumed;
}

bool vector_editor_commit_ui(void *instance, PUIView *view, PUIContext *input, char *error,
                             size_t capacity) {
    VectorEditor *editor = instance;
    if (!vector_document_tabs_commit(&editor->document_tabs, &editor->documents, view, input, error,
                                     capacity)) {
        return false;
    }
    vector_document_tabs_captions(&editor->documents, view);
    VectorDocument *active = vector_document_tabs_active(&editor->documents, view);
    if (!vector_inspector_sync(&editor->inspector, active, view, input, error, capacity)) {
        return false;
    }
    vector_toolbar_sync(view, active);
    return vector_document_tree_sync(&editor->document_tree,
                                     vector_document_tabs_active(&editor->documents, view), view,
                                     error, capacity);
}
