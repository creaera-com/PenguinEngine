/** @file @brief Vector inspector module; future inspector bindings belong here. */
#include "internal.h"

PUILayout *vector_inspector_load(const EditorServices *services, char *error, size_t capacity) {
    return editor_load_ui(services, "editors/vector/ui/inspector.yaml", error, capacity);
}
