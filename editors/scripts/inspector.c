/** @file @brief Scripts inspector module; future inspector bindings belong here. */
#include "internal.h"

PUILayout *scripts_inspector_load(const EditorServices *services, char *error, size_t capacity) {
    return editor_load_ui(services, "editors/scripts/ui/inspector.yaml", error, capacity);
}
