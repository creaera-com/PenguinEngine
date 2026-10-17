/** @file @brief Domain-neutral access to editor UI resources. */
#include "editor.h"
#include <stdio.h>

PUILayout *editor_load_ui(const EditorServices *services, const char *relative, char *error,
                          size_t capacity) {
    char path[1600];
    if (!services || !services->resource_root || !relative ||
        snprintf(path, sizeof path, "%s/%s", services->resource_root, relative) >=
            (int)sizeof path) {
        if (error && capacity) {
            snprintf(error, capacity, "invalid or oversized UI resource path");
        }
        return NULL;
    }
    return pui_layout_load(path, error, capacity);
}
