/** @file @brief Document data, identity and ownership; independent of UI tabs and event handling.
 */
#ifndef PE_VECTOR_DOCUMENT_H
#define PE_VECTOR_DOCUMENT_H
#include "editors/vector/artboard/artboard.h"
#include "editors/vector/artboard/tool_state.h"
#include "editors/vector/history/history.h"
#include "editors/vector/rectangle/rectangle.h"
#include "editors/vector/rectangle/tool_state.h"
#include "editors/vector/workspace/viewport.h"
#define VECTOR_MAX_DOCUMENTS 15

/** @brief An unsaved document with artboards, history and viewport; owned by the active editor. */
typedef struct VectorDocument {
    unsigned id;
    char title[64];
    VectorArtboard artboards[VECTOR_MAX_ARTBOARDS];
    int artboard_count;
    unsigned next_artboard_id, active_artboard;
    VectorRectangle rectangles[VECTOR_MAX_RECTANGLES];
    int rectangle_count;
    unsigned next_rectangle_id, active_rectangle;
    VectorRectangleTool rectangle_tool;
    VectorHistory history;
    VectorViewport viewport;
    VectorArtboardTool artboard_tool;
    VectorSelectionTool selection_tool;
} VectorDocument;

/** @brief Owned collection of documents; only entries below count are live. */
typedef struct {
    VectorDocument items[VECTOR_MAX_DOCUMENTS];
    int count;
} VectorDocuments;

/** @brief Initialize an empty collection. */
void vector_documents_init(VectorDocuments *documents);
/** @brief Initialize an empty document with its title, next artboard ID and default viewport. */
void vector_document_init(VectorDocument *document, unsigned id);
/** @brief Find a document by its model ID; NULL when it does not exist. */
VectorDocument *vector_documents_find(VectorDocuments *documents, unsigned id);
/** @brief Append a prepared empty document by value.
 * Caller must check count < VECTOR_MAX_DOCUMENTS and pass a document with no owned history data.
 */
void vector_documents_append(VectorDocuments *documents, const VectorDocument *document);
/** @brief Release histories and document collection when the Vector editor is unloaded. */
void vector_documents_destroy(VectorDocuments *documents);
#endif
