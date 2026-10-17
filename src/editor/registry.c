#include "editor/registry.h"
#include <string.h>

int pe_editor_registry_index(PeEditorRegistry registry, UiId id) {
    for (size_t i = 0; registry.views && i < registry.count; i++) {
        if (registry.views[i] && registry.views[i]->id == id) {
            return (int)i;
        }
    }
    return -1;
}

const EditorView *pe_editor_registry_find(PeEditorRegistry registry, UiId id) {
    int index = pe_editor_registry_index(registry, id);
    return index >= 0 ? registry.views[index] : NULL;
}

const EditorView *pe_editor_registry_module(PeEditorRegistry registry, const char *module) {
    if (!module) {
        return NULL;
    }
    for (size_t i = 0; registry.views && i < registry.count; i++) {
        const EditorView *view = registry.views[i];
        if (view && view->module && !strcmp(view->module, module)) {
            return view;
        }
    }
    return NULL;
}

const EditorView *pe_editor_registry_request(PeEditorRegistry registry, int command) {
    if (!command) {
        return NULL;
    }
    for (size_t i = 0; registry.views && i < registry.count; i++) {
        const EditorView *view = registry.views[i];
        if (view && (view->open_request == command || view->new_request == command ||
                     view->import_request == command)) {
            return view;
        }
    }
    return NULL;
}
