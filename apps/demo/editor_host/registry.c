/** @file @brief The only place that knows all concrete editor adapters. */
#include "registry.h"
#include "editors/game/adapter.h"
#include "editors/scripts/adapter.h"
#include "editors/vector/adapter.h"
#include <string.h>

const EditorDefinition *editor_registry_find(const char *id) {
    if (!id) {
        return NULL;
    }
    const EditorDefinition *definitions[] = {vector_editor_definition(),
                                             scripts_editor_definition(), game_editor_definition()};
    for (size_t i = 0; i < sizeof definitions / sizeof definitions[0]; i++) {
        if (!strcmp(definitions[i]->id, id)) {
            return definitions[i];
        }
    }
    return NULL;
}
