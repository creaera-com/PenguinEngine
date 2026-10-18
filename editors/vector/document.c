/** @file @brief Document ownership and deferred canvas-tab creation. */
#include "document.h"
#include <stdio.h>
#include <string.h>

void vector_documents_init(VectorDocuments *documents) {
    *documents = (VectorDocuments){0};
}

VectorDocument *vector_documents_active(VectorDocuments *documents, const PUIView *view) {
    const char *tab = pui_view_selected_tab(view, "vector_document_tabs");
    if (tab) {
        for (int i = 0; i < documents->count; i++) {
            char id[64];
            snprintf(id, sizeof id, "vector_document_%u", documents->items[i].id);
            if (!strcmp(tab, id)) {
                return &documents->items[i];
            }
        }
    }
    return NULL;
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
        VectorDocument document = {.id = (unsigned)documents->count + 1,
                                   .next_artboard_id = 1,
                                   .viewport = {.x = 64, .y = 64, .zoom = 1}};
        snprintf(document.title, sizeof document.title, "Dokument %u", document.id);
        char tab_id[64];
        snprintf(tab_id, sizeof tab_id, "vector_document_%u", document.id);
        PUILayout *canvas = vector_canvas_layout(error, capacity);
        bool added = canvas && pui_view_add_tab(view, input, "vector_document_tabs", tab_id,
                                                document.title, canvas, error, capacity);
        pui_layout_free(canvas);
        if (!added) {
            documents->pending = 0;
            return false;
        }
        documents->items[documents->count++] = document;
        documents->pending--;
        pui_view_set_text(view, "status", document.title);
    }
    return true;
}

void vector_documents_destroy(VectorDocuments *documents) {
    for (int i = 0; i < documents->count; i++) {
        vector_artboard_tool_cancel(&documents->items[i]);
        vector_history_destroy(&documents->items[i].history);
    }
    *documents = (VectorDocuments){0};
}
