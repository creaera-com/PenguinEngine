/** @file @brief Native dialogs carry only owned values; editors are reached through callbacks. */
#include "controller.h"
#include "editor_host/registry.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct {
    Uint32 event;
    char path[4096], error[256];
} Result;

static void SDLCALL chosen(void *context, const char *const *files, int filter) {
    (void)filter;
    Result *r = context;
    if (!files) {
        snprintf(r->error, sizeof r->error, "Speicherdialog fehlgeschlagen: %.200s",
                 SDL_GetError());
    } else if (files[0]) {
        if (strlen(files[0]) >= sizeof r->path) {
            snprintf(r->error, sizeof r->error, "Dateipfad ist zu lang");
        } else {
            snprintf(r->path, sizeof r->path, "%s", files[0]);
        }
    }
    SDL_Event event = {.type = r->event};
    event.user.data1 = r;
    if (!SDL_PushEvent(&event)) {
        free(r);
    }
}

bool document_save_default_path(char *path, size_t capacity, const EditorFileInfo *info,
                                const char *project_root) {
    if (*info->filename) {
        int n = snprintf(path, capacity, "%s", info->filename);
        return n >= 0 && (size_t)n < capacity;
    }
    const char *root =
        project_root && *project_root ? project_root : SDL_GetUserFolder(SDL_FOLDER_DOCUMENTS);
    char *cwd = NULL;
    if (!root) {
        cwd = SDL_GetCurrentDirectory();
        root = cwd ? cwd : ".";
    }
    int n = snprintf(path, capacity, "%s/%s.%s", root, *info->title ? info->title : "Dokument",
                     info->extension);
    SDL_free(cwd);
    return n >= 0 && (size_t)n < capacity;
}

void document_controller_init(DocumentController *c) {
    *c = (DocumentController){.event = SDL_RegisterEvents(1)};
}

void document_controller_destroy(DocumentController *c) {
    SDL_Event e;
    while (c->event && SDL_PeepEvents(&e, 1, SDL_GETEVENT, c->event, c->event) == 1) {
        free(e.user.data1);
    }
}

bool document_controller_action(DocumentController *c, EditorHost *host,
                                ProjectController *projects, SDL_Window *window, PUIView *view,
                                const char *action, const char *widget) {
    if (!strcmp(action, "project.entry")) {
        const char *path = project_explorer_file(&projects->explorer, widget);
        const EditorDefinition *owner = path ? editor_registry_for_file(path) : NULL;
        if (!owner) {
            return false;
        }
        if (c->dialog_pending) {
            return true;
        }
        snprintf(c->pending_open, sizeof c->pending_open, "%s", path);
        editor_host_request(host, owner->id, false);
        return true;
    }
    if (strcmp(action, "editor.save") && strcmp(action, "editor.save_as")) {
        return false;
    }
    if (c->dialog_pending || *c->pending_save) {
        return true;
    }
    EditorFileInfo info = {0};
    if (!host->active || !host->active->file_info || !host->active->save_file ||
        !host->active->file_info(host->instance, view, &info)) {
        pui_view_set_text(view, "status", "Kein speicherbares Dokument ausgewaehlt");
        return true;
    }
    c->owner = host->active;
    c->document = info.document;
    if (*info.filename && !strcmp(action, "editor.save")) {
        snprintf(c->pending_save, sizeof c->pending_save, "%s", info.filename);
        return true;
    }
    if (!document_save_default_path(c->default_path, sizeof c->default_path, &info,
                                    projects->project.root)) {
        pui_view_set_text(view, "status", "Projektpfad ist zu lang");
        return true;
    }
    Result *result = calloc(1, sizeof *result);
    if (!result || !c->event) {
        free(result);
        return true;
    }
    result->event = c->event;
    c->dialog_pending = true;
    static const SDL_DialogFileFilter filter[] = {{"Penguengine Vektordokument", "pengvec"}};
    SDL_ShowSaveFileDialog(chosen, result, window, filter, 1, c->default_path);
    return true;
}

bool document_controller_event(DocumentController *c, PUIView *view, const SDL_Event *event) {
    if (event->type != c->event) {
        return false;
    }
    Result *r = event->user.data1;
    c->dialog_pending = false;
    if (*r->error) {
        pui_view_set_text(view, "status", r->error);
    } else if (*r->path) {
        const char *name = strrchr(r->path, '/'), *back = strrchr(r->path, '\\');
        if (back && (!name || back > name)) {
            name = back;
        }
        const char *ext = strrchr(name ? name + 1 : r->path, '.');
        bool extension = ext && !SDL_strcasecmp(ext, ".pengvec");
        int n = snprintf(c->pending_save, sizeof c->pending_save, "%s%s", r->path,
                         extension ? "" : ".pengvec");
        SDL_PathInfo info;
        if (!extension && SDL_GetPathInfo(c->pending_save, &info)) {
            c->pending_save[0] = 0;
            pui_view_set_text(view, "status",
                              "Zieldatei existiert: bitte mit .pengvec-Endung erneut waehlen");
        }
        if (n >= (int)sizeof c->pending_save) {
            c->pending_save[0] = 0;
            pui_view_set_text(view, "status", "Dateipfad ist zu lang");
        }
    }
    free(r);
    return true;
}

void document_controller_commit(DocumentController *c, EditorHost *host,
                                ProjectController *projects, PUIView *view, PUIContext *input) {
    char error[256] = {0};
    if (*c->pending_open) {
        const EditorDefinition *owner = editor_registry_for_file(c->pending_open);
        bool ok =
            owner && host->active == owner && owner->open_file &&
            owner->open_file(host->instance, view, input, c->pending_open, error, sizeof error);
        pui_view_set_text(view, "status",
                          ok ? "Dokument geladen"
                             : (*error ? error : "Dokumenteditor konnte nicht aktiviert werden"));
        c->pending_open[0] = 0;
    }
    if (*c->pending_save) {
        bool ok = host->active == c->owner && c->owner &&
                  c->owner->save_file(host->instance, view, c->document, c->pending_save, error,
                                      sizeof error);
        pui_view_set_text(view, "status",
                          ok ? "Dokument gespeichert"
                             : (*error ? error : "Editor gewechselt: bitte erneut speichern"));
        if (ok) {
            project_explorer_action(&projects->explorer, view, "project.refresh", "");
        }
        c->pending_save[0] = 0;
    }
}
