/** @file @brief Inspector-owned artboard form state and bindings. */
#ifndef PE_VECTOR_ARTBOARD_INSPECTOR_H
#define PE_VECTOR_ARTBOARD_INSPECTOR_H
#include "editors/vector/document/document.h"

typedef struct {
    PUILayout *layout;
    unsigned document_id, artboard_id, dirty;
    VectorArtboard presented;
    bool mounted;
} VectorArtboardInspector;

/** @brief Route committed artboard fields through undoable commands. */
bool vector_artboard_inspector_action(VectorArtboardInspector *inspector,
                                      VectorDocuments *documents, PUIView *view,
                                      const char *action);
/** @brief Mount and update the selected artboard without disturbing unrelated input. */
bool vector_artboard_inspector_sync(VectorArtboardInspector *inspector, VectorDocument *document,
                                    PUIView *view, PUIContext *input, char *error, size_t capacity);
#endif
