/** @file @brief Commit file operations between frames; candidate loads do not disturb open tabs. */
#include "files.h"
#include "canvas.h"
#include "documents.h"
#include "editors/vector/internal.h"
#include "editors/vector/persistence/document_file.h"
#include "editors/vector/text/tool.h"
#include <stdlib.h>
#include <string.h>

static bool same_document_path(const char *a, const char *b) {
#ifdef _WIN32
    /* SDL enumeration and native dialogs can use different separators. */
    while (*a && *b) {
        unsigned char x = (unsigned char)*a++, y = (unsigned char)*b++;
        if (x == '\\') {
            x = '/';
        }
        if (y == '\\') {
            y = '/';
        }
        if (SDL_tolower(x) != SDL_tolower(y)) {
            return false;
        }
    }
    return *a == *b;
#else
    return !strcmp(a, b);
#endif
}

bool vector_workspace_accepts_file(const char *path) {
    const char *ext = path ? strrchr(path, '.') : NULL;
    return ext && !SDL_strcasecmp(ext, ".pengvec");
}

bool vector_workspace_file_info(void *instance, const PUIView *view, EditorFileInfo *info) {
    VectorEditor *editor = instance;
    VectorDocument *d = vector_document_tabs_active(&editor->documents, view);
    if (!d) {
        return false;
    }
    *info = (EditorFileInfo){.artboard = d->active_artboard,
                             .document = d->id,
                             .modified = vector_document_modified(d),
                             .extension = "pengvec"};
    snprintf(info->filename, sizeof info->filename, "%s", d->filename);
    snprintf(info->title, sizeof info->title, "%s", d->title);
    return true;
}

bool vector_workspace_open_file(void *instance, PUIView *view, PUIContext *input, const char *path,
                                char *error, size_t capacity) {
    VectorEditor *editor = instance;
    if (!path || !*path) {
        snprintf(error, capacity, "Kein Dokumentpfad angegeben");
        return false;
    }
    /* Reuse the live document before checking capacity or touching disk:
     * unsaved edits, history, viewport and selection must remain intact. */
    for (int i = 0; i < editor->documents.count; i++) {
        const VectorDocument *open = &editor->documents.items[i];
        if (same_document_path(open->filename, path)) {
            char tab[64];
            snprintf(tab, sizeof tab, "vector_document_%u", open->id);
            return pui_view_select_tab(view, "vector_document_tabs", tab);
        }
    }
    if (editor->documents.count >= VECTOR_MAX_DOCUMENTS) {
        snprintf(error, capacity, "Dokument-Tab-Limit erreicht");
        return false;
    }
    VectorDocument *candidate = calloc(1, sizeof *candidate);
    if (!candidate) {
        return false;
    }
    vector_document_init(candidate, vector_documents_next_id(&editor->documents));
    bool ok = vector_document_file_load(candidate, path, error, capacity);
    if (ok) {
        char id[64];
        snprintf(id, sizeof id, "vector_document_%u", candidate->id);
        PUILayout *canvas = vector_canvas_layout(error, capacity);
        ok = canvas && pui_view_add_tab(view, input, "vector_document_tabs", id, candidate->title,
                                        canvas, error, capacity);
        pui_layout_free(canvas);
    }
    if (ok) {
        vector_documents_append(&editor->documents, candidate);
        vector_canvas_center(&editor->documents.items[editor->documents.count - 1]);
    } else {
        vector_history_destroy(&candidate->history);
    }
    free(candidate);
    return ok;
}

bool vector_workspace_save_file(void *instance, PUIView *view, unsigned id, const char *path,
                                char *error, size_t capacity) {
    VectorEditor *editor = instance;
    VectorDocument *d = vector_documents_find(&editor->documents, id);
    if (!d) {
        snprintf(error, capacity, "Dokument ist nicht mehr geoeffnet");
        return false;
    }
    vector_text_tool_end(d);
    if (!vector_document_file_save(d, path, error, capacity)) {
        return false;
    }
    char tab[64];
    snprintf(tab, sizeof tab, "vector_document_%u", id);
    pui_view_set_text(view, tab, d->title);
    return true;
}

bool vector_workspace_close_info(void *instance, const PUIView *view, const char *widget,
                                 EditorFileInfo *info) {
    (void)view;
    VectorEditor *editor = instance;
    for (int i = 0; i < editor->documents.count; i++) {
        VectorDocument *d = &editor->documents.items[i];
        char tab[64];
        snprintf(tab, sizeof tab, "vector_document_%u", d->id);
        if (!strcmp(widget, tab)) {
            vector_text_tool_end(d);
            *info = (EditorFileInfo){
                .document = d->id, .modified = vector_document_modified(d), .extension = "pengvec"};
            snprintf(info->filename, sizeof info->filename, "%s", d->filename);
            snprintf(info->title, sizeof info->title, "%s", d->title);
            return true;
        }
    }
    return false;
}

bool vector_workspace_close_file(void *instance, PUIView *view, PUIContext *input, unsigned id,
                                 char *error, size_t capacity) {
    VectorEditor *editor = instance;
    if (!vector_documents_find(&editor->documents, id)) {
        return false;
    }
    char tab[64];
    snprintf(tab, sizeof tab, "vector_document_%u", id);
    if (!pui_view_remove_tab(view, input, "vector_document_tabs", tab, error, capacity)) {
        return false;
    }
    return vector_documents_remove(&editor->documents, id);
}
