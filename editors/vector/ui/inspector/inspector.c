/** @file @brief Inspector selection routing and panel composition. */
#include "inspector.h"
#include "selection/selection.h"

PUILayout *vector_inspector_load(const EditorServices *services, char *error, size_t capacity) {
    return editor_load_ui(services, "editors/vector/ui/inspector/inspector.yaml", error, capacity);
}

void vector_inspector_cancel_color(VectorInspector *inspector, VectorDocuments *documents,
                                   PUIView *view) {
    vector_object_inspector_cancel_color(&inspector->object, documents, view);
}

bool vector_inspector_action(VectorInspector *inspector, VectorDocuments *documents, PUIView *view,
                             const char *action) {
    return vector_object_inspector_action(&inspector->object, documents, view, action) ||
           vector_artboard_inspector_action(&inspector->artboard, documents, view, action);
}

bool vector_inspector_sync(VectorInspector *inspector, VectorDocument *document, PUIView *view,
                           PUIContext *input, char *error, size_t capacity) {
    bool multi = document && document->selection.count > 1;
    if (multi) {
        if (!inspector->multi_mounted) {
            if (!pui_view_replace_slot(view, input, "selection_properties", inspector->multi_layout,
                                       error, capacity)) {
                return false;
            }
            inspector->artboard.mounted = inspector->object.mounted = false;
            inspector->artboard.document_id = inspector->artboard.artboard_id = 0;
            inspector->object.document_id = inspector->object.object_id = 0;
            inspector->multi_mounted = true;
        }
        vector_selection_inspector_sync(document, view);
        return true;
    }
    if (inspector->multi_mounted) {
        if (!pui_view_replace_slot(view, input, "selection_properties", NULL, error, capacity)) {
            return false;
        }
        inspector->multi_mounted = false;
    }
    bool rectangle = document && vector_rectangle_find(document, document->active_rectangle);
    if (rectangle && inspector->artboard.mounted) {
        if (!pui_view_replace_slot(view, input, "selection_properties", NULL, error, capacity)) {
            return false;
        }
        inspector->artboard.mounted = false;
    }
    if (!vector_object_inspector_sync(&inspector->object, document, view, input, error, capacity)) {
        return false;
    }
    if (rectangle) {
        inspector->artboard.document_id = 0;
        inspector->artboard.artboard_id = 0;
        return true;
    }
    return vector_artboard_inspector_sync(&inspector->artboard, document, view, input, error,
                                          capacity);
}
