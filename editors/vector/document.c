/** @file @brief Empty document creation; no SVG, artboard, save or undo behavior. */
#include "document.h"
#include <stdio.h>

bool vector_documents_init(VectorDocuments *documents, const EditorServices *services, char *error,
                           size_t capacity) {
    *documents = (VectorDocuments){0};
    documents->ui = editor_load_ui(services, "editors/vector/ui/document.yaml", error, capacity);
    return documents->ui != NULL;
}

void vector_documents_request(VectorDocuments *documents, PUIView *view) {
    if (documents->count + documents->pending >= VECTOR_MAX_DOCUMENTS) {
        pui_view_set_text(view, "status", "Document tab limit reached");
        return;
    }
    documents->pending++;
}

bool vector_documents_commit(VectorDocuments *documents, PUIView *view, PUIContext *input,
                             char *error, size_t capacity) {
    while (documents->pending > 0) {
        VectorDocument document = {.id = (unsigned)documents->count + 1};
        snprintf(document.title, sizeof document.title, "Dokument %u", document.id);
        char tab_id[64];
        snprintf(tab_id, sizeof tab_id, "vector_document_%u", document.id);
        if (!pui_view_add_tab(view, input, "vector_document_tabs", tab_id, document.title,
                              documents->ui, error, capacity)) {
            documents->pending = 0;
            return false;
        }
        documents->items[documents->count++] = document;
        documents->pending--;
        char title_id[64];
        snprintf(title_id, sizeof title_id, "vector_document_%u.document_title", document.id);
        pui_view_set_text(view, title_id, document.title);
        pui_view_set_text(view, "status", document.title);
    }
    return true;
}

void vector_documents_destroy(VectorDocuments *documents) {
    pui_layout_free(documents->ui);
    *documents = (VectorDocuments){0};
}
