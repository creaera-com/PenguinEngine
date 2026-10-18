/** @file @brief Workspace tab creation and selection; document ownership stays in the model. */
#include "documents.h"
#include "canvas.h"
#include "editors/vector/artboard/tool.h"
#include <stdio.h>
#include <string.h>

_Static_assert(VECTOR_MAX_DOCUMENTS <= PUI_MAX_TABS, "document capacity exceeds workspace tabs");

VectorDocument *vector_document_tabs_active(VectorDocuments *documents, const PUIView *view) {
    const char *tab = pui_view_selected_tab(view, "vector_document_tabs");
    if (tab) {
        for (int i = 0; i < documents->count; i++) {
            char id[64];
            snprintf(id, sizeof id, "vector_document_%u", documents->items[i].id);
            if (!strcmp(tab, id)) {
                return vector_documents_find(documents, documents->items[i].id);
            }
        }
    }
    return NULL;
}

void vector_document_tabs_request(VectorDocumentTabs *tabs, const VectorDocuments *documents,
                                  PUIView *view) {
    if (documents->count + tabs->pending >= VECTOR_MAX_DOCUMENTS) {
        pui_view_set_text(view, "status", "Document tab limit reached");
        return;
    }
    tabs->pending++;
}

bool vector_document_tabs_commit(VectorDocumentTabs *tabs, VectorDocuments *documents,
                                 PUIView *view, PUIContext *input, char *error, size_t capacity) {
    /* A fresh or empty restored workspace gets one document. An explicit request
     * already provides that document, so do not queue a second one. */
    if (!documents->count && !tabs->pending) {
        tabs->pending = 1;
    }
    while (tabs->pending > 0) {
        VectorDocument document;
        vector_document_init(&document, (unsigned)documents->count + 1);
        char tab_id[64];
        snprintf(tab_id, sizeof tab_id, "vector_document_%u", document.id);
        PUILayout *canvas = vector_canvas_layout(error, capacity);
        bool added = canvas && pui_view_add_tab(view, input, "vector_document_tabs", tab_id,
                                                document.title, canvas, error, capacity);
        pui_layout_free(canvas);
        if (!added) {
            tabs->pending = 0;
            return false;
        }
        vector_documents_append(documents, &document);
        tabs->pending--;
        pui_view_set_text(view, "status", document.title);
    }
    return true;
}

void vector_workspace_cancel_inactive(VectorDocuments *documents, const VectorDocument *active) {
    for (int i = 0; i < documents->count; i++) {
        VectorDocument *document = &documents->items[i];
        if (document != active) {
            vector_artboard_tool_cancel(document);
            document->viewport.panning = false;
        }
    }
}

void vector_document_tabs_captions(VectorDocuments *documents, PUIView *view) {
    for (int i = 0; i < documents->count; i++) {
        VectorDocument *doc = &documents->items[i];
        const char *caption = doc->title;
        for (int j = 0; j < doc->artboard_count; j++) {
            if (doc->artboards[j].id == doc->active_artboard) {
                caption = doc->artboards[j].name;
            }
        }
        char tab[64];
        snprintf(tab, sizeof tab, "vector_document_%u", doc->id);
        pui_view_set_text(view, tab, caption);
    }
}
