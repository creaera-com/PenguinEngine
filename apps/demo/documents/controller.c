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
        snprintf(r->error, sizeof r->error, "Dateidialog fehlgeschlagen: %.200s", SDL_GetError());
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
    *c = (DocumentController){.event = SDL_RegisterEvents(1),
                              .confirm = SDL_ShowMessageBox,
                              .save_dialog = SDL_ShowSaveFileDialog};
}

void document_controller_destroy(DocumentController *c) {
    SDL_Event e;
    while (c->event && SDL_PeepEvents(&e, 1, SDL_GETEVENT, c->event, c->event) == 1) {
        free(e.user.data1);
    }
}

static bool request_save(DocumentController *c, const EditorFileInfo *info,
                         ProjectController *projects, SDL_Window *window, PUIView *view,
                         bool save_as) {
    c->document = info->document;
    if (*info->filename && !save_as) {
        snprintf(c->pending_save, sizeof c->pending_save, "%s", info->filename);
        return true;
    }
    if (!document_save_default_path(c->default_path, sizeof c->default_path, info,
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
    c->save_dialog(chosen, result, window, filter, 1, c->default_path);
    return true;
}

bool document_controller_action(DocumentController *c, EditorHost *host,
                                ProjectController *projects, SDL_Window *window, PUIView *view,
                                const char *action, const char *widget) {
    if (!strcmp(action, "editor.close_document")) {
        if (c->dialog_pending || *c->pending_save || *c->pending_import || *c->pending_open ||
            *c->close_request || c->pending_close || !host->active || !host->active->close_info ||
            !host->active->close_file || !widget || strlen(widget) >= sizeof c->close_request) {
            return true;
        }
        c->owner = host->active;
        c->close_window = window;
        snprintf(c->close_request, sizeof c->close_request, "%s", widget);
        return true;
    }
    if ((*c->close_request || c->pending_close) &&
        (!strcmp(action, "editor.save") || !strcmp(action, "editor.save_as") ||
         !strcmp(action, "editor.import_image") || !strcmp(action, "project.entry"))) {
        return true;
    }
    if (!strcmp(action, "editor.import_image")) {
        if (c->dialog_pending || *c->pending_import || *c->pending_save || *c->pending_open) {
            return true;
        }
        EditorFileInfo info = {0};
        if (!*projects->project.root || !host->active || !host->active->import_image ||
            !host->active->file_info || !host->active->file_info(host->instance, view, &info) ||
            !info.artboard) {
            pui_view_set_text(view, "status",
                              "Zuerst ein Projekt oeffnen und ein Artboard auswaehlen");
            return true;
        }
        Result *result = calloc(1, sizeof *result);
        if (!result || !c->event) {
            free(result);
            return true;
        }
        c->owner = host->active;
        c->document = info.document;
        c->artboard = info.artboard;
        snprintf(c->import_root, sizeof c->import_root, "%s", projects->project.root);
        c->importing = c->dialog_pending = true;
        result->event = c->event;
        static const SDL_DialogFileFilter filters[] = {{"PNG / JPEG", "png;jpg;jpeg"}};
        SDL_ShowOpenFileDialog(chosen, result, window, filters, 1, c->import_root, false);
        return true;
    }
    if (!strcmp(action, "project.entry")) {
        const char *path = project_explorer_file(&projects->explorer, widget);
        const EditorDefinition *owner = path ? editor_registry_for_file(path) : NULL;
        if (!owner) {
            return false;
        }
        if (c->dialog_pending || *c->pending_import || *c->pending_save) {
            return true;
        }
        snprintf(c->pending_open, sizeof c->pending_open, "%s", path);
        editor_host_request(host, owner->id, false);
        return true;
    }
    if (strcmp(action, "editor.save") && strcmp(action, "editor.save_as")) {
        return false;
    }
    if (c->dialog_pending || *c->pending_save || *c->pending_import) {
        return true;
    }
    EditorFileInfo info = {0};
    if (!host->active || !host->active->file_info || !host->active->save_file ||
        !host->active->file_info(host->instance, view, &info)) {
        pui_view_set_text(view, "status", "Kein speicherbares Dokument ausgewaehlt");
        return true;
    }
    c->owner = host->active;
    return request_save(c, &info, projects, window, view, !strcmp(action, "editor.save_as"));
}

bool document_controller_event(DocumentController *c, PUIView *view, const SDL_Event *event) {
    if (event->type != c->event) {
        return false;
    }
    Result *r = event->user.data1;
    c->dialog_pending = false;
    if (*r->error) {
        pui_view_set_text(view, "status", r->error);
    } else if (*r->path && c->importing) {
        snprintf(c->pending_import, sizeof c->pending_import, "%s", r->path);
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
    if (!*c->pending_save) {
        c->close_after_save = false;
    }
    c->importing = false;
    free(r);
    return true;
}

/* Confirmation happens between frames, after pending form edits have been submitted. */
static void request_close(DocumentController *c, EditorHost *host, ProjectController *projects,
                          PUIView *view) {
    EditorFileInfo info = {0};
    bool valid = host->active == c->owner && c->owner && c->owner->close_info &&
                 c->owner->close_info(host->instance, view, c->close_request, &info);
    c->close_request[0] = 0;
    if (!valid) {
        return;
    }
    if (!info.modified) {
        c->pending_close = info.document;
        return;
    }
    const SDL_MessageBoxButtonData buttons[] = {
        {SDL_MESSAGEBOX_BUTTON_ESCAPEKEY_DEFAULT, 0, "Abbrechen"},
        {0, 1, "Verwerfen"},
        {SDL_MESSAGEBOX_BUTTON_RETURNKEY_DEFAULT, 2, "Speichern"}};
    char message[256];
    snprintf(message, sizeof message, "Aenderungen an \"%s\" vor dem Schliessen speichern?",
             info.title);
    SDL_MessageBoxData box = {.flags = SDL_MESSAGEBOX_WARNING,
                              .window = c->close_window,
                              .title = "Dokument schliessen",
                              .message = message,
                              .numbuttons = 3,
                              .buttons = buttons};
    int choice = 0;
    if (!c->confirm(&box, &choice)) {
        pui_view_set_text(view, "status",
                          "Bestaetigung konnte nicht geoeffnet werden; Dokument bleibt offen");
        return;
    }
    if (choice == 1) {
        c->pending_close = info.document;
    } else if (choice == 2) {
        c->close_after_save = true;
        request_save(c, &info, projects, c->close_window, view, false);
        if (!c->dialog_pending && !*c->pending_save) {
            c->close_after_save = false;
        }
    }
}

void document_controller_commit(DocumentController *c, EditorHost *host,
                                ProjectController *projects, PUIView *view, PUIContext *input) {
    char error[256] = {0};
    if (*c->close_request) {
        request_close(c, host, projects, view);
    }
    if (*c->pending_import) {
        bool ok = host->active == c->owner && c->owner && c->owner->import_image &&
                  !strcmp(c->import_root, projects->project.root) &&
                  c->owner->import_image(host->instance, view, c->document, c->artboard,
                                         c->pending_import, c->import_root, error, sizeof error);
        pui_view_set_text(
            view, "status",
            ok ? "Bild ins Projekt importiert"
               : (*error ? error : "Editor oder Projekt gewechselt: bitte erneut importieren"));
        if (ok) {
            project_explorer_action(&projects->explorer, view, "project.refresh", "");
        }
        c->pending_import[0] = 0;
    }
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
        if (ok && c->close_after_save) {
            c->pending_close = c->document;
        }
        c->close_after_save = false;
        c->pending_save[0] = 0;
    }
    if (c->pending_close) {
        bool ok = host->active == c->owner && c->owner && c->owner->close_file &&
                  c->owner->close_file(host->instance, view, input, c->pending_close, error,
                                       sizeof error);
        pui_view_set_text(view, "status",
                          ok ? "Dokument geschlossen"
                             : (*error ? error : "Dokument konnte nicht geschlossen werden"));
        c->pending_close = 0;
    }
}
