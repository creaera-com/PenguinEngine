/** @file @brief Game inspector module; future inspector bindings belong here. */
#include "internal.h"

PUILayout *game_inspector_load(const EditorServices *services, char *error, size_t capacity) {
    return editor_load_ui(services, "editors/game/ui/inspector.yaml", error, capacity);
}
