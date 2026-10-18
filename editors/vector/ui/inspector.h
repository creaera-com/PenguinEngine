/** @file @brief Inspector UI assembly; independent of the editor's private instance. */
#ifndef PE_VECTOR_INSPECTOR_H
#define PE_VECTOR_INSPECTOR_H
#include "editor_host/editor.h"
#include "editors/vector/document/document.h"
#include "rectangle_inspector.h"

typedef struct {
    PUILayout *artboard_layout;
    VectorRectangleInspector rectangle;
    unsigned document_id, artboard_id;
    VectorArtboard presented;
    bool mounted;
    unsigned dirty;
} VectorInspector;

/** @brief Mount properties only for a selected artboard; synchronize model changes between frames.
 */
bool vector_inspector_sync(VectorInspector *inspector, VectorDocument *document, PUIView *view,
                           PUIContext *input, char *error, size_t capacity);
/** @brief Translate committed input values into undoable model changes. */
bool vector_inspector_action(VectorInspector *inspector, VectorDocuments *documents, PUIView *view,
                             const char *action);
/** @brief Load the inspector description; caller owns the returned layout. */
PUILayout *vector_inspector_load(const EditorServices *services, char *error, size_t capacity);
#endif
