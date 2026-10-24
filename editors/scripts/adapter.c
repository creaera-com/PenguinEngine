/** @file @brief Adapter exposes only the common editor contract to the registry. */
#include "adapter.h"
#include "internal.h"

const EditorDefinition *scripts_editor_definition(void) {
    static const EditorDefinition definition = {.id = "scripts",
                                                .title = "Scripts Editor",
                                                .create = scripts_editor_create,
                                                .layout = scripts_editor_layout,
                                                .destroy = scripts_editor_destroy};
    return &definition;
}
