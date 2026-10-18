/** @file @brief Vector document metadata and queued document-tab creation. */
#ifndef PE_VECTOR_DOCUMENT_H
#define PE_VECTOR_DOCUMENT_H
#include "artboard.h"
#include "canvas.h"
#include "editor_host/editor.h"
#include "history.h"
#define VECTOR_MAX_DOCUMENTS (PUI_MAX_TABS - 1)

/** @brief An unsaved document with artboards, history and viewport; owned by the active editor. */
typedef struct VectorDocument {
    unsigned id;
    char title[64];
    VectorArtboard artboards[VECTOR_MAX_ARTBOARDS];
    int artboard_count;
    unsigned next_artboard_id, active_artboard;
    VectorHistory history;
    VectorViewport viewport;
} VectorDocument;

/** @brief Private document collection; the existing Vector overview occupies one tab. */
typedef struct {
    VectorDocument items[VECTOR_MAX_DOCUMENTS];
    int count, pending;
} VectorDocuments;

/** @brief Initialize an empty collection. */
void vector_documents_init(VectorDocuments *documents);
/** @brief Find the active document by the selected document tab; NULL for the overview. */
VectorDocument *vector_documents_active(VectorDocuments *documents, const PUIView *view);
/** @brief Queue a document from an action callback; updates only a status message on limit. */
void vector_documents_request(VectorDocuments *documents, PUIView *view);
/** @brief Create queued documents and tabs between frames; model changes only after success. */
bool vector_documents_commit(VectorDocuments *documents, PUIView *view, PUIContext *input,
                             char *error, size_t capacity);
/** @brief Release histories and document collection when the Vector editor is unloaded. */
void vector_documents_destroy(VectorDocuments *documents);
#endif
