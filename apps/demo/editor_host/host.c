/** @file @brief Transactional lifetime management independent of concrete editor modules. */
#include "host.h"
#include "checkpoint.h"
#include "monitor/monitor.h"
#include "registry.h"
#include <stdio.h>
#include <string.h>

static double seconds(void) {
    return (double)SDL_GetTicksNS() / 1000000000.;
}

static EditorDiagnostics diagnostics(const EditorHost *host, const PUIView *view) {
    return host->active && host->active->diagnostics
               ? host->active->diagnostics(host->instance, view)
               : (EditorDiagnostics){0};
}

static const char *editor_id(const EditorHost *host) {
    return host->active ? host->active->id : "";
}

static void observe(EditorHost *host, PUIView *view) {
    if (host->log.stream) {
        monitor_log_observe(&host->log, &host->monitor, editor_id(host), diagnostics(host, view),
                            seconds());
    }
}

void editor_host_project(EditorHost *host, const char *root) {
    monitor_log_project(&host->log, root, &host->monitor, seconds());
}

void editor_host_log_action(EditorHost *host, PUIView *view, const char *name, const char *widget) {
    if (host->log.stream) {
        monitor_log_action(&host->log, &host->monitor, editor_id(host), diagnostics(host, view),
                           name, widget, seconds());
    }
}

static void draw_canvas(void *context, SDL_Renderer *renderer, PUIRect area, const char *widget) {
    EditorHost *host = context;
    if (!process_monitor_draw(&host->monitor, renderer, area, widget) && host->active &&
        host->active->canvas) {
        host->active->canvas(host->instance, renderer, area, widget);
    }
    /* Capture after painting so newly created/released resources are visible this frame. */
    process_monitor_graphics(&host->monitor, renderer);
}

bool editor_host_event(EditorHost *host, PUIView *view, const SDL_Event *event) {
    bool consumed =
        host->active && host->active->event && host->active->event(host->instance, view, event);
    if (consumed && event->type == SDL_EVENT_KEY_DOWN && !event->key.repeat &&
        ((event->key.mod & (SDL_KMOD_CTRL | SDL_KMOD_GUI)) || event->key.key == SDLK_DELETE ||
         event->key.key == SDLK_ESCAPE)) {
        char shortcut[96];
        snprintf(shortcut, sizeof shortcut, "%s%s%s%.64s",
                 event->key.mod & (SDL_KMOD_CTRL | SDL_KMOD_GUI) ? "Ctrl/Cmd+" : "",
                 event->key.mod & SDL_KMOD_SHIFT ? "Shift+" : "",
                 event->key.mod & SDL_KMOD_ALT ? "Alt+" : "", SDL_GetKeyName(event->key.key));
        editor_host_log_action(host, view, "keyboard.shortcut", shortcut);
    }
    observe(host, view);
    return consumed;
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

static bool commit_editor(EditorHost *host, PUIView *shell, PUIContext *input, char *error,
                          size_t capacity) {
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

bool editor_host_commit(EditorHost *host, PUIView *shell, PUIContext *input, char *error,
                        size_t capacity) {
    process_monitor_tick(&host->monitor, shell, seconds());
    bool ok = commit_editor(host, shell, input, error, capacity);
    observe(host, shell);
    process_monitor_recording_sync(&host->log, shell);
    return ok;
}

bool editor_host_action(EditorHost *host, PUIView *view, const char *name, const char *widget) {
    if (!strcmp(name, "monitor.log")) {
        return process_monitor_recording_action(&host->log, &host->monitor, view, editor_id(host),
                                                diagnostics(host, view), seconds());
    }
    return host->active && host->active->action &&
           host->active->action(host->instance, view, name, widget);
}

void editor_host_destroy(EditorHost *host) {
    monitor_log_stop(&host->log, &host->monitor, "application.shutdown", seconds());
    if (host->active) {
        host->active->destroy(host->instance);
    }
    host->active = host->pending = NULL;
    host->instance = NULL;
    editor_checkpoint_destroy(host->checkpoints);
    host->checkpoints = NULL;
}
