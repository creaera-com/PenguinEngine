/** @file @brief Document initialization, lookup and ownership without UI operations. */
#include "document.h"
#include <stdio.h>

void vector_documents_init(VectorDocuments *documents) {
    *documents = (VectorDocuments){0};
}

void vector_document_init(VectorDocument *document, unsigned id) {
    *document = (VectorDocument){
        .id = id,
        .next_artboard_id = 1,
        .next_rectangle_id = 1,
        .grid = {.snap_tolerance = 5, .width = 16, .height = 16, .color = 0x80808080},
        .viewport = {.x = 64, .y = 64, .zoom = 1}};
    snprintf(document->title, sizeof document->title, "Dokument %u", id);
}

VectorDocument *vector_documents_find(VectorDocuments *documents, unsigned id) {
    for (int i = 0; i < documents->count; i++) {
        if (documents->items[i].id == id) {
            return &documents->items[i];
        }
    }
    return NULL;
}

void vector_documents_append(VectorDocuments *documents, const VectorDocument *document) {
    documents->items[documents->count++] = *document;
}

void vector_documents_destroy(VectorDocuments *documents) {
    for (int i = 0; i < documents->count; i++) {
        vector_history_destroy(&documents->items[i].history);
    }
    *documents = (VectorDocuments){0};
}
