/** @file @brief Transactional lifetime management independent of concrete editor modules. */
#include "host.h"
#include "checkpoint.h"
#include "monitor/monitor.h"
#include "registry.h"
#include <stdio.h>

static void draw_canvas(void *context, SDL_Renderer *renderer, PUIRect area, const char *widget) {
    EditorHost *host = context;
    process_monitor_graphics(&host->monitor, renderer);
    if (process_monitor_draw(&host->monitor, renderer, area, widget)) {
        return;
    }
    if (host->active && host->active->canvas) {
        host->active->canvas(host->instance, renderer, area, widget);
    }
}

bool editor_host_event(EditorHost *host, PUIView *view, const SDL_Event *event) {
    return host->active && host->active->event && host->active->event(host->instance, view, event);
}

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
    process_monitor_tick(&host->monitor, shell, (double)SDL_GetTicksNS() / 1000000000.);
    const EditorDefinition *next = host->pending;
    if (!next) {
        return !host->active || !host->active->commit_ui ||
               host->active->commit_ui(host->instance, shell, input, error, capacity);
    }
    host->pending = NULL;
    EditorCheckpoint *staged = NULL;
    if (host->active &&
        ((host->active->commit_ui &&
          !host->active->commit_ui(host->instance, shell, input, error, capacity)) ||
         !editor_checkpoint_capture(host->active, host->instance, shell, &staged, error,
                                    capacity))) {
        return false;
    }
    void *candidate = next->create(&host->services, error, capacity);
    if (!candidate) {
        editor_checkpoint_destroy(staged);
        return false;
    }
    FILE *saved = next == host->active && staged ? staged->file
                                                 : editor_checkpoint_find(host->checkpoints, next);
    if (!editor_checkpoint_restore(next, candidate, saved, error, capacity)) {
        next->destroy(candidate);
        editor_checkpoint_destroy(staged);
        return false;
    }
    const PUILayout *layout = next->layout(candidate);
    if (!layout) {
        next->destroy(candidate);
        editor_checkpoint_destroy(staged);
        if (error && capacity) {
            snprintf(error, capacity, "editor did not provide a UI layout");
        }
        return false;
    }
    bool monitor_requested;
    PUILayout *monitored =
        process_monitor_layout(&host->services, layout, &monitor_requested, error, capacity);
    bool mounted = (!monitor_requested || monitored) &&
                   pui_view_replace_slot(shell, input, "editor_area",
                                         monitored ? monitored : layout, error, capacity);
    pui_layout_free(monitored);
    if (!mounted) {
        next->destroy(candidate);
        editor_checkpoint_destroy(staged);
        return false;
    }
    editor_checkpoint_publish(&host->checkpoints, staged);
    if (host->active) {
        host->active->destroy(host->instance);
    }
    host->instance = candidate;
    host->active = next;
    pui_view_set_canvas_renderer(shell, draw_canvas, host);
    pui_view_set_text(shell, "status", next->title);
    pui_view_set_text(shell, "viewport_status", "");
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
    editor_checkpoint_destroy(host->checkpoints);
    host->checkpoints = NULL;
}
