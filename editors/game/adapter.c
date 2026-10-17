/** @file @brief Adapter exposes only the common editor contract to the registry. */
#include "adapter.h"
#include "internal.h"

const EditorDefinition *game_editor_definition(void) {
    static const EditorDefinition definition = {.id = "game",
                                                .title = "Game Editor",
                                                .create = game_editor_create,
                                                .layout = game_editor_layout,
                                                .destroy = game_editor_destroy};
    return &definition;
}
