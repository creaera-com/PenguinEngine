/** @file @brief Vector editor root; only UI assembly, no document or tool functionality. */
#include "internal.h"
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
    if (!editor->layout || !vector_documents_init(&editor->documents, services, error, capacity)) {
        pui_layout_free(editor->layout);
        free(editor);
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
        vector_documents_destroy(&editor->documents);
        pui_layout_free(editor->layout);
        free(editor);
    }
}

bool vector_editor_action(void *instance, PUIView *view, const char *name, const char *widget) {
    (void)widget;
    VectorEditor *editor = instance;
    if (!strcmp(name, "vector.add_document")) {
        vector_documents_request(&editor->documents, view);
        return true;
    }
    return false;
}

bool vector_editor_commit_ui(void *instance, PUIView *view, PUIContext *input, char *error,
                             size_t capacity) {
    VectorEditor *editor = instance;
    return vector_documents_commit(&editor->documents, view, input, error, capacity);
}
