/** @file @brief Asynchronous native dialog callbacks transfer owned results through SDL events. */
#include "controller.h"
#include "state.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct {
    Uint32 event;
    bool create;
    char path[PROJECT_PATH_CAPACITY], error[256];
} DialogResult;

static void SDLCALL chosen(void *context, const char *const *files, int filter) {
    (void)filter;
    DialogResult *result = context;
    if (!files) {
        snprintf(result->error, sizeof result->error, "Dateidialog fehlgeschlagen: %.210s",
                 SDL_GetError());
    } else if (files[0]) {
        if (strlen(files[0]) >= sizeof result->path) {
            snprintf(result->error, sizeof result->error, "Ausgewaehlter Pfad ist zu lang");
        } else {
            snprintf(result->path, sizeof result->path, "%s", files[0]);
        }
    }
    SDL_Event event = {.type = result->event};
    event.user.data1 = result;
    if (!SDL_PushEvent(&event)) {
        free(result);
    }
}

bool project_controller_open(ProjectController *c, const char *path, bool create, PUIView *view) {
    char error[256];
    AppProject next = {0};
    bool ok = create ? app_project_create(&next, path, error, sizeof error)
                     : app_project_open(&next, path, error, sizeof error);
    if (!ok) {
        pui_view_set_text(view, "status", error);
        return false;
    }
    c->project = next;
    project_explorer_set(&c->explorer, &next);
    bool remembered = !*c->state_file || app_project_state_save(c->state_file, next.file);
    char status[512];
    snprintf(status, sizeof status, "Projekt: %.250s%s", next.name,
             remembered ? "" : " (letzter Pfad konnte nicht gespeichert werden)");
    pui_view_set_text(view, "status", status);
    return true;
}

bool project_controller_init(ProjectController *c, const char *resources, bool remember,
                             char *error, size_t capacity) {
    *c = (ProjectController){0};
    c->dialog_event = SDL_RegisterEvents(1);
    if (!c->dialog_event || !project_explorer_init(&c->explorer, resources, error, capacity)) {
        return false;
    }
    if (remember && app_project_state_path(c->state_file, sizeof c->state_file)) {
        char path[PROJECT_PATH_CAPACITY];
        if (app_project_state_read(c->state_file, path, sizeof path)) {
            if (app_project_open(&c->project, path, error, capacity)) {
                project_explorer_set(&c->explorer, &c->project);
            } else {
                return true;
            } // A missing previous project must never prevent starting the app.
        }
    }
    return true;
}

void project_controller_destroy(ProjectController *c) {
    SDL_Event event;
    while (c->dialog_event &&
           SDL_PeepEvents(&event, 1, SDL_GETEVENT, c->dialog_event, c->dialog_event) == 1) {
        free(event.user.data1);
    }
    // A pending dialog owns its callback context independently of this controller.
    project_explorer_destroy(&c->explorer);
}

bool project_controller_action(ProjectController *c, SDL_Window *window, PUIView *view,
                               const char *action, const char *widget) {
    bool create = !strcmp(action, "project.new"), open = !strcmp(action, "project.open");
    if (!create && !open) {
        return project_explorer_action(&c->explorer, view, action, widget);
    }
    if (c->dialog_pending) {
        return true;
    }
    DialogResult *result = calloc(1, sizeof *result);
    if (!result) {
        pui_view_set_text(view, "status", "Dateidialog konnte nicht gestartet werden");
        return true;
    }
    result->event = c->dialog_event;
    result->create = create;
    c->dialog_pending = true;
    if (create) {
        SDL_ShowOpenFolderDialog(chosen, result, window, *c->project.root ? c->project.root : NULL,
                                 false);
    } else {
        static const SDL_DialogFileFilter filters[] = {{"Penguengine Projekt", "yaml;yml"}};
        SDL_ShowOpenFileDialog(chosen, result, window, filters, 1,
                               *c->project.root ? c->project.root : NULL, false);
    }
    return true;
}

bool project_controller_event(ProjectController *c, PUIView *view, const SDL_Event *event) {
    if (event->type != c->dialog_event) {
        return false;
    }
    DialogResult *result = event->user.data1;
    c->dialog_pending = false;
    if (*result->error) {
        pui_view_set_text(view, "status", result->error);
    } else if (*result->path) {
        project_controller_open(c, result->path, result->create, view);
    }
    free(result);
    return true;
}
