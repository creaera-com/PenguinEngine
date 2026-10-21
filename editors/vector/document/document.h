/** @file @brief Document data, identity and ownership; independent of UI tabs and event handling.
 */
#ifndef PE_VECTOR_DOCUMENT_H
#define PE_VECTOR_DOCUMENT_H
#include "editors/vector/artboard/artboard.h"
#include "editors/vector/artboard/tool_state.h"
#include "editors/vector/curve/tool_state.h"
#include "editors/vector/gradient/tool_state.h"
#include "editors/vector/grid/state.h"
#include "editors/vector/history/history.h"
#include "editors/vector/preview/state.h"
#include "editors/vector/rectangle/rectangle.h"
#include "editors/vector/text/tool_state.h"
#include "editors/vector/workspace/box_tool_state.h"
#include "editors/vector/workspace/selection_state.h"
#include "editors/vector/workspace/viewport.h"
#define VECTOR_MAX_DOCUMENTS 15

/** @brief An unsaved document with artboards, history and viewport; owned by the active editor. */
typedef struct VectorDocument {
    unsigned id;
    uint64_t saved_content; ///< Fingerprint of the last successfully saved artwork.
    bool has_saved_content;
    char title[64];
    char filename[4096]; ///< Absolute saved filename, empty until first save.
    VectorArtboard artboards[VECTOR_MAX_ARTBOARDS];
    int artboard_count;
    unsigned next_artboard_id, active_artboard;
    VectorRectangle rectangles[VECTOR_MAX_RECTANGLES];
    int rectangle_count;
    unsigned next_rectangle_id, active_rectangle;
    VectorRectangleTool rectangle_tool;
    VectorCurveTool curve_tool;
    VectorGradientTool gradient_tool;
    VectorTextTool text_tool;
    VectorFillPreview fill_preview;
    VectorHistory history;
    VectorViewport viewport;
    VectorGrid grid;
    bool rulers_hidden;        ///< Per-document viewing preference; rulers start visible.
    VectorPreview preview;     ///< Per-document viewing region; not a persistent artwork edit.
    bool element_snap_enabled; ///< Object alignment guides and magnetic movement, independent of
                               ///< grid.
    VectorArtboardTool artboard_tool;
    VectorSelectionTool selection_tool;
    VectorRectangleSelection selection;
} VectorDocument;

/** @brief Owned collection of documents; only entries below count are live. */
typedef struct {
    VectorDocument items[VECTOR_MAX_DOCUMENTS];
    int count;
    unsigned next_id; ///< Monotonic document IDs survive closing other tabs.
} VectorDocuments;

/** @brief Initialize an empty collection. */
void vector_documents_init(VectorDocuments *documents);
/** @brief Initialize an empty document with its title, next artboard ID and default viewport. */
void vector_document_init(VectorDocument *document, unsigned id);
/** @brief Initialize the automatic startup document with one selected 1920x1080 artboard.
 * Creation uses history and returns false on allocation failure. */
bool vector_document_init_default(VectorDocument *document, unsigned id);
/** @brief Find a document by its model ID; NULL when it does not exist. */
VectorDocument *vector_documents_find(VectorDocuments *documents, unsigned id);
/** @brief Append a prepared empty document by value.
 * Caller must check count < VECTOR_MAX_DOCUMENTS and pass a document with no owned history data.
 */
void vector_documents_append(VectorDocuments *documents, const VectorDocument *document);
/** @brief Whether the artwork differs from its last save; untouched empty documents are clean. */
bool vector_document_modified(const VectorDocument *document);
/** @brief Record the current artwork as successfully saved. */
void vector_document_mark_saved(VectorDocument *document);
/** @brief Next unused identity, or zero when the identity range is exhausted. */
unsigned vector_documents_next_id(const VectorDocuments *documents);
/** @brief Release a document's history and remove it without renumbering remaining IDs. */
bool vector_documents_remove(VectorDocuments *documents, unsigned id);
/** @brief Release histories and document collection when the Vector editor is unloaded. */
void vector_documents_destroy(VectorDocuments *documents);
#endif
