/** @file @brief Optional rectangle inspector and field-to-model bindings. */
#ifndef PE_VECTOR_RECTANGLE_INSPECTOR_H
#define PE_VECTOR_RECTANGLE_INSPECTOR_H
#include "editor_host/editor.h"
#include "editors/vector/document/document.h"

typedef struct {
    PUILayout *layout;
    unsigned document_id, rectangle_id, dirty;
    VectorRectangle presented;
    bool mounted;
} VectorRectangleInspector;

bool vector_rectangle_inspector_action(VectorRectangleInspector *inspector,
                                       VectorDocuments *documents, PUIView *view,
                                       const char *action);
bool vector_rectangle_inspector_sync(VectorRectangleInspector *inspector, VectorDocument *document,
                                     PUIView *view, PUIContext *input, char *error,
                                     size_t capacity);
#endif
