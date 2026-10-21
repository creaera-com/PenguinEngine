/** @file @brief Document initialization, lookup and ownership without UI operations. */
#include "document.h"
#include <limits.h>
#include <stdio.h>
#include <string.h>

void vector_documents_init(VectorDocuments *documents) {
    *documents = (VectorDocuments){.next_id = 1};
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

bool vector_document_init_default(VectorDocument *document, unsigned id) {
    vector_document_init(document, id);
    return vector_artboard_add(document, (PUIRect){0, 0, 1920, 1080});
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
    documents->next_id = document->id + 1;
}

void vector_documents_destroy(VectorDocuments *documents) {
    for (int i = 0; i < documents->count; i++) {
        vector_history_destroy(&documents->items[i].history);
    }
    *documents = (VectorDocuments){.next_id = 1};
}

unsigned vector_documents_next_id(const VectorDocuments *documents) {
    return documents->next_id && documents->next_id < UINT_MAX ? documents->next_id : 0;
}

bool vector_documents_remove(VectorDocuments *documents, unsigned id) {
    for (int i = 0; i < documents->count; i++) {
        if (documents->items[i].id == id) {
            vector_history_destroy(&documents->items[i].history);
            memmove(&documents->items[i], &documents->items[i + 1],
                    (documents->count - i - 1) * sizeof documents->items[0]);
            documents->count--;
            memset(&documents->items[documents->count], 0, sizeof documents->items[0]);
            return true;
        }
    }
    return false;
}

static uint64_t artwork_hash(const VectorDocument *document) {
    uint64_t hash = UINT64_C(14695981039346656037);
    const void *parts[] = {&document->artboard_count, document->artboards,
                           &document->rectangle_count, document->rectangles};
    size_t sizes[] = {sizeof document->artboard_count,
                      document->artboard_count * sizeof document->artboards[0],
                      sizeof document->rectangle_count,
                      document->rectangle_count * sizeof document->rectangles[0]};
    for (int i = 0; i < 4; i++) {
        const unsigned char *bytes = parts[i];
        for (size_t j = 0; j < sizes[i]; j++) {
            hash = (hash ^ bytes[j]) * UINT64_C(1099511628211);
        }
    }
    return hash;
}

void vector_document_mark_saved(VectorDocument *document) {
    document->saved_content = artwork_hash(document);
    document->has_saved_content = true;
}

bool vector_document_modified(const VectorDocument *document) {
    return document->has_saved_content ? document->saved_content != artwork_hash(document)
                                       : document->artboard_count || document->rectangle_count;
}
