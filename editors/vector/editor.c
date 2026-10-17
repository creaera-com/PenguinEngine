/** @file @brief Vector editor root; only UI assembly, no document or tool functionality. */
#include "internal.h"
#include <stdio.h>
#include <stdlib.h>

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
    if (!editor->layout) {
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
        pui_layout_free(editor->layout);
        free(editor);
    }
}
