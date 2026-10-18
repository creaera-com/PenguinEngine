/** @file @brief Adapter exposes only the common editor contract to the registry. */
#include "adapter.h"
#include "editors/vector/persistence/checkpoint.h"
#include "internal.h"

const EditorDefinition *vector_editor_definition(void) {
    static const EditorDefinition definition = {.id = "vector",
                                                .title = "Vector Editor",
                                                .create = vector_editor_create,
                                                .layout = vector_editor_layout,
                                                .destroy = vector_editor_destroy,
                                                .action = vector_editor_action,
                                                .commit_ui = vector_editor_commit_ui,
                                                .canvas = vector_editor_canvas,
                                                .event = vector_editor_event,
                                                .checkpoint = vector_editor_checkpoint,
                                                .restore = vector_editor_restore};
    return &definition;
}
