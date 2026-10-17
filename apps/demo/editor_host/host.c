/** @file @brief Transactional lifetime management independent of concrete editor modules. */
#include "host.h"
#include "registry.h"
#include <stdio.h>

void editor_host_init(EditorHost *host, EditorServices services) {
    *host = (EditorHost){.services = services};
}

bool editor_host_request(EditorHost *host, const char *id, bool force) {
    const EditorDefinition *definition = editor_registry_find(id);
    if (!definition) {
        return false;
    }
    host->pending = !force && definition == host->active ? NULL : definition;
    return true;
}

bool editor_host_commit(EditorHost *host, PUIView *shell, PUIContext *input, char *error,
                        size_t capacity) {
    const EditorDefinition *next = host->pending;
    if (!next) {
        return !host->active || !host->active->commit_ui ||
               host->active->commit_ui(host->instance, shell, input, error, capacity);
    }
    host->pending = NULL;
    void *candidate = next->create(&host->services, error, capacity);
    if (!candidate) {
        return false;
    }
    const PUILayout *layout = next->layout(candidate);
    if (!layout) {
        next->destroy(candidate);
        if (error && capacity) {
            snprintf(error, capacity, "editor did not provide a UI layout");
        }
        return false;
    }
    if (!pui_view_replace_slot(shell, input, "editor_area", layout, error, capacity)) {
        next->destroy(candidate);
        return false;
    }
    if (host->active) {
        host->active->destroy(host->instance);
    }
    host->instance = candidate;
    host->active = next;
    pui_view_set_text(shell, "status", next->title);
    pui_view_set_checked(shell, next->id, true);
    return true;
}

bool editor_host_action(EditorHost *host, PUIView *view, const char *name, const char *widget) {
    return host->active && host->active->action &&
           host->active->action(host->instance, view, name, widget);
}

void editor_host_destroy(EditorHost *host) {
    if (host->active) {
        host->active->destroy(host->instance);
    }
    host->active = host->pending = NULL;
    host->instance = NULL;
}
