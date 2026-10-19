/** @file @brief Document-to-tree binding; no tree widgets leak into the document model. */
#ifndef PE_VECTOR_DOCUMENT_TREE_H
#define PE_VECTOR_DOCUMENT_TREE_H
#include "editors/vector/document/document.h"

/** @brief Last presented artboard IDs; derived UI state, never persisted as document data. */
typedef struct {
    bool initialized;
    unsigned document_id;
    unsigned artboard_ids[VECTOR_MAX_ARTBOARDS];
    char names[VECTOR_MAX_ARTBOARDS][64];
    unsigned flags[VECTOR_MAX_ARTBOARDS];
    int count;
    VectorRectangle rectangles[VECTOR_MAX_RECTANGLES];
    int rectangle_count;
    VectorRectangleSelection selection;
    unsigned primary;
} VectorDocumentTree;

/** @brief Select a right-clicked row without toggling disclosure or an existing multi-selection. */
void vector_document_tree_context(VectorDocument *document, PUIView *view, float x, float y);
/** @brief Select an artboard from its stable tree row ID. */
bool vector_document_tree_action(VectorDocument *document, PUIView *view, const char *action,
                                 const char *widget);

/** @brief Refresh runtime tree rows between frames when the active document changes. */
bool vector_document_tree_sync(VectorDocumentTree *tree, const VectorDocument *document,
                               PUIView *view, char *error, size_t capacity);
#endif
