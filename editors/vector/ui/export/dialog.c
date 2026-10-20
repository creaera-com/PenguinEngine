/** @file @brief Native dialog callbacks never touch editor data or widgets. */
#include "dialog.h"
#include <errno.h>
#include <stdlib.h>
#include <string.h>

struct VectorExportMailbox {
    SDL_Mutex *mutex;
    SDL_AtomicInt references;
    bool alive, pending, completed;
    char path[4096], error[192];
};

static void release(VectorExportMailbox *m) {
    if (SDL_AtomicDecRef(&m->references)) {
        SDL_DestroyMutex(m->mutex);
        free(m);
    }
}

static void SDLCALL folder_chosen(void *context, const char *const *files, int filter) {
    (void)filter;
    VectorExportMailbox *m = context;
    SDL_LockMutex(m->mutex);
    if (m->alive) {
        m->completed = true;
        m->path[0] = m->error[0] = 0;
        if (!files) {
            snprintf(m->error, sizeof m->error, "Ordnerdialog fehlgeschlagen: %.140s",
                     SDL_GetError());
        } else if (files[0]) {
            if (strlen(files[0]) >= sizeof m->path) {
                snprintf(m->error, sizeof m->error, "Ordnerpfad zu lang");
            } else {
                snprintf(m->path, sizeof m->path, "%s", files[0]);
            }
        }
    }
    SDL_UnlockMutex(m->mutex);
    release(m);
}

static void status(PUIView *view, const char *message) {
    pui_view_set_text(view, "status", message);
    pui_view_set_text(view, "vector_export_status", message);
}

PUILayout *vector_export_ui_layout(const EditorServices *s, PUILayout *base, char *error,
                                   size_t capacity) {
    PUILayout *popup = editor_load_ui(s, "editors/vector/ui/export/dialog.yaml", error, capacity);
    PUILayout *result =
        base && popup ? pui_layout_compose(base, "vector_export_toolbar", popup, error, capacity)
                      : NULL;
    pui_layout_free(base);
    pui_layout_free(popup);
    return result;
}

void vector_export_ui_destroy(VectorExportUI *ui) {
    free(ui->request);
    ui->request = NULL;
    if (ui->mailbox) {
        SDL_LockMutex(ui->mailbox->mutex);
        ui->mailbox->alive = false;
        SDL_UnlockMutex(ui->mailbox->mutex);
        release(ui->mailbox);
        ui->mailbox = NULL;
    }
}

void vector_export_ui_sync(VectorExportUI *ui, const VectorDocument *d, PUIView *view) {
    ui->view = view;
    if (!ui->initialized) {
        const char *root = SDL_GetUserFolder(SDL_FOLDER_DOCUMENTS);
        char *cwd = NULL;
        if (!root) {
            cwd = SDL_GetCurrentDirectory();
            root = cwd ? cwd : ".";
        }
        snprintf(ui->directory, sizeof ui->directory, "%s", root);
        SDL_free(cwd);
        if (d && *d->filename) {
            snprintf(ui->directory, sizeof ui->directory, "%s", d->filename);
            char *last = strrchr(ui->directory, '/'), *back = strrchr(ui->directory, '\\');
            if (back && (!last || back > last)) {
                last = back;
            }
            if (last) {
                last[1] = 0;
            }
        }
        pui_view_set_text(view, "vector_export_directory", ui->directory);
        ui->initialized = true;
    }
    VectorExportMailbox *m = ui->mailbox;
    if (m) {
        SDL_LockMutex(m->mutex);
        if (m->completed) {
            if (*m->path) {
                if (!pui_view_set_text(view, "vector_export_directory", m->path)) {
                    status(view,
                           "Ordnerpfad passt nicht ins Eingabefeld; bitte kuerzeren Pfad waehlen");
                }
            } else {
                status(view, *m->error ? m->error : "Ordnerauswahl abgebrochen");
            }
            m->pending = m->completed = false;
        }
        SDL_UnlockMutex(m->mutex);
    }
}

static void choose_folder(VectorExportUI *ui, PUIView *view) {
    if (!ui->renderer) {
        return;
    }
    if (!ui->mailbox) {
        VectorExportMailbox *m = calloc(1, sizeof *m);
        if (!m) {
            return;
        }
        m->mutex = SDL_CreateMutex();
        if (!m->mutex) {
            free(m);
            return;
        }
        m->alive = true;
        SDL_SetAtomicInt(&m->references, 1);
        ui->mailbox = m;
    }
    VectorExportMailbox *m = ui->mailbox;
    SDL_LockMutex(m->mutex);
    bool pending = m->pending;
    if (!pending) {
        m->pending = true;
        SDL_AtomicIncRef(&m->references);
    }
    SDL_UnlockMutex(m->mutex);
    if (!pending) {
        SDL_ShowOpenFolderDialog(folder_chosen, m, SDL_GetRenderWindow(ui->renderer),
                                 pui_view_text(view, "vector_export_directory"), false);
    }
}

bool vector_export_ui_action(VectorExportUI *ui, const VectorDocument *d, PUIView *view,
                             const char *action) {
    if (strncmp(action, "vector.export.", 14)) {
        return false;
    }
    vector_export_ui_sync(ui, d, view);
    if (!strcmp(action, "vector.export.folder")) {
        choose_folder(ui, view);
        return true;
    }
    if (!strcmp(action, "vector.export.format")) {
        const char *value = pui_view_value(view, "vector_export_format");
        pui_view_set_text(view, "vector_export_hint",
                          value && !strcmp(value, "jpg") ? "JPEG: weisser Hintergrund"
                                                         : "PNG: Transparenz optional");
        return true;
    }
    if (strcmp(action, "vector.export.write")) {
        return false;
    }
    if (ui->request) {
        return true;
    }
    const char *scope = pui_view_value(view, "vector_export_scope");
    VectorExportScope selected = scope && !strcmp(scope, "all")         ? VECTOR_EXPORT_ALL
                                 : scope && !strcmp(scope, "selection") ? VECTOR_EXPORT_SELECTION
                                                                        : VECTOR_EXPORT_ARTBOARD;
    char error[256];
    if (!vector_export_plan(d, selected, &ui->plan, error, sizeof error)) {
        status(view, error);
        return true;
    }
    const char *directory = pui_view_text(view, "vector_export_directory"),
               *stem = pui_view_text(view, "vector_export_name");
    const char *format = pui_view_value(view, "vector_export_format");
    ui->jpeg = format && !strcmp(format, "jpg");
    const char *quality = pui_view_text(view, "vector_export_quality");
    char *end = NULL;
    errno = 0;
    long q = ui->jpeg && quality ? strtol(quality, &end, 10) : 90;
    if (!directory || !*directory || strlen(directory) >= sizeof ui->directory || !stem || !*stem ||
        strlen(stem) >= sizeof ui->stem ||
        (ui->jpeg && (!quality || end == quality || *end || errno || q < 1 || q > 100))) {
        status(view, "Ordner, Dateiname und JPEG-Qualitaet (1-100) pruefen");
        return true;
    }
    snprintf(ui->directory, sizeof ui->directory, "%s", directory);
    snprintf(ui->stem, sizeof ui->stem, "%s", stem);
    ui->quality = (int)q;
    ui->transparent = pui_view_checked(view, "vector_export_transparent") && !ui->jpeg;
    ui->request = malloc(sizeof *ui->request);
    if (!ui->request) {
        status(view, "Nicht genug Speicher fuer den Export");
        return true;
    }
    *ui->request = *d;
    ui->request->history = (VectorHistory){0};
    status(view, "Export wird erstellt...");
    return true;
}

void vector_export_ui_render(VectorExportUI *ui, SDL_Renderer *r, VectorTextRenderer *text,
                             VectorImageRenderer *images) {
    PUIView *view = ui->view;
    ui->renderer = r;
    if (!ui->request) {
        return;
    }
    char message[256];
    vector_export_write(ui->request, &ui->plan, r, text, images, ui->directory, ui->stem, ui->jpeg,
                        ui->transparent, ui->quality, message, sizeof message);
    free(ui->request);
    ui->request = NULL;
    status(view, message);
}
