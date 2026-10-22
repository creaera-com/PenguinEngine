/** @file @brief Inspector UI assembly; independent of the editor's private instance. */
#ifndef PE_VECTOR_INSPECTOR_H
#define PE_VECTOR_INSPECTOR_H
#include "artboard/artboard.h"
#include "editor_host/editor.h"
#include "editors/vector/document/document.h"
#include "object/object.h"

typedef struct {
    PUILayout *multi_layout;
    bool multi_mounted;
    VectorObjectInspector object;
    VectorArtboardInspector artboard;
} VectorInspector;

/** @brief Load inspector resources; fonts and text renderer are borrowed. Start with zeroed state.
 * On failure all acquired layouts are freed; the error describes the first failed load.
 */
bool vector_inspector_init(VectorInspector *inspector, const EditorServices *services,
                           PUIFonts *fonts, VectorTextRenderer *text, char *error, size_t capacity);
/** @brief Free owned layouts; safe after failed initialization and repeated destruction. */
void vector_inspector_destroy(VectorInspector *inspector);

/** @brief Select and synchronize the appropriate artboard, object or selection panel.
 */
bool vector_inspector_sync(VectorInspector *inspector, VectorDocument *document, PUIView *view,
                           PUIContext *input, char *error, size_t capacity);
/** @brief Discard pending color preview before tool/history context changes. */
void vector_inspector_cancel_color(VectorInspector *inspector, VectorDocuments *documents,
                                   PUIView *view);
/** @brief Translate committed input values into undoable model changes. */
bool vector_inspector_action(VectorInspector *inspector, VectorDocuments *documents, PUIView *view,
                             const char *action);
/** @brief Load the inspector description; caller owns the returned layout. */
PUILayout *vector_inspector_load(const EditorServices *services, char *error, size_t capacity);
#endif
