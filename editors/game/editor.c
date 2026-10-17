/** @file @brief Game editor root; only UI assembly, no document or tool functionality. */
#include "internal.h"
#include <stdio.h>
#include <stdlib.h>

void *game_editor_create(const EditorServices *services, char *error, size_t capacity) {
    GameEditor *editor = calloc(1, sizeof *editor);
    if (!editor) {
        snprintf(error, capacity, "could not allocate Game editor");
        return NULL;
    }
    PUILayout *root = editor_load_ui(services, "editors/game/ui/editor.yaml", error, capacity);
    if (!root) {
        free(editor);
        return NULL;
    }
    PUILayout *inspector = game_inspector_load(services, error, capacity);
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

const PUILayout *game_editor_layout(const void *instance) {
    const GameEditor *editor = instance;
    return editor->layout;
}

void game_editor_destroy(void *instance) {
    GameEditor *editor = instance;
    if (editor) {
        pui_layout_free(editor->layout);
        free(editor);
    }
}
