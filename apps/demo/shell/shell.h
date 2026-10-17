/** @file @brief Persistent shell UI; owns its root layout, view and input context. */
#ifndef PE_SHELL_H
#define PE_SHELL_H
#include "pengui/pengui.h"

typedef struct {
    PUILayout *layout;
    PUIView *view;
    PUIContext input;
} EditorShell;

/** @brief Load the permanent shell from a YAML file; false leaves an empty shell. */
bool editor_shell_load(EditorShell *shell, SDL_Renderer *renderer, const char *path, char *error,
                       size_t capacity);
/** @brief Release UI resources before destroying the renderer. */
void editor_shell_destroy(EditorShell *shell);
#endif
