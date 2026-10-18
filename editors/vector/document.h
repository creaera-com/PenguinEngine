/** @file @brief Vector document metadata and queued document-tab creation. */
#ifndef PE_VECTOR_DOCUMENT_H
#define PE_VECTOR_DOCUMENT_H
#include "editor_host/editor.h"
#define VECTOR_MAX_DOCUMENTS (PUI_MAX_TABS - 1)

/** @brief An unsaved empty document, owned by one Vector editor instance. */
typedef struct {
    unsigned id;
    char title[64];
} VectorDocument;

/** @brief Private document collection; the existing Vector overview occupies one tab. */
typedef struct {
    PUILayout *ui;
    VectorDocument items[VECTOR_MAX_DOCUMENTS];
    int count, pending;
} VectorDocuments;

/** @brief Load the YAML template once; no documents are created yet. */
bool vector_documents_init(VectorDocuments *documents, const EditorServices *services, char *error,
                           size_t capacity);
/** @brief Queue a document from an action callback; updates only a status message on limit. */
void vector_documents_request(VectorDocuments *documents, PUIView *view);
/** @brief Create queued documents and tabs between frames; model changes only after success. */
bool vector_documents_commit(VectorDocuments *documents, PUIView *view, PUIContext *input,
                             char *error, size_t capacity);
/** @brief Release template and document collection when the Vector editor is unloaded. */
void vector_documents_destroy(VectorDocuments *documents);
#endif
