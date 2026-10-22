/** @file
 * @brief UI binding between document IDs, canvas tabs and deferred creation. */
#ifndef PE_VECTOR_DOCUMENT_TABS_H
#define PE_VECTOR_DOCUMENT_TABS_H
#include "editors/vector/document/document.h"

/** @brief Deferred UI requests, kept outside the document model. */
typedef struct {
    int pending;
    bool initialized; ///< An intentionally empty workspace must remain empty.
} VectorDocumentTabs;

/** @brief Map the selected workspace tab to a model document; NULL when no document is selected. */
VectorDocument *vector_document_tabs_active(VectorDocuments *documents, const PUIView *view);
/** @brief Queue one tab creation or show a status message if the capacity is reached. */
void vector_document_tabs_request(VectorDocumentTabs *tabs, const VectorDocuments *documents,
                                  PUIView *view);
/** @brief Add tabs between frames, ensuring a startup document with a 1920x1080 artboard when
 * empty; append model data only after successful UI composition. */
bool vector_document_tabs_commit(VectorDocumentTabs *tabs, VectorDocuments *documents,
                                 PUIView *view, PUIContext *input, char *error, size_t capacity);
/** @brief Refresh all tab captions; an asterisk marks new or modified documents. */
void vector_document_tabs_sync(const VectorDocuments *documents, PUIView *view);
/** @brief Cancel native interactions in all documents except active (NULL cancels all). */
void vector_workspace_cancel_inactive(VectorDocuments *documents, const VectorDocument *active);
#endif
