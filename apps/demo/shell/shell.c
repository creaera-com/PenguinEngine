/** @file @brief Shell construction uses only the public pengUI API. */
#include "shell.h"
#include <stdio.h>

bool editor_shell_load(EditorShell *shell, SDL_Renderer *renderer, const char *path, char *error,
                       size_t capacity) {
    *shell = (EditorShell){0};
    shell->layout = pui_layout_load(path, error, capacity);
    if (!shell->layout) {
        return false;
    }
    shell->view = pui_view_new(shell->layout, renderer);
    if (!shell->view) {
        if (error && capacity) {
            snprintf(error, capacity, "could not create shell view");
        }
        editor_shell_destroy(shell);
        return false;
    }
    pui_init(&shell->input);
    pui_view_apply_theme(shell->layout);
    return true;
}

void editor_shell_destroy(EditorShell *shell) {
    pui_view_free(shell->view);
    pui_layout_free(shell->layout);
    *shell = (EditorShell){0};
}
