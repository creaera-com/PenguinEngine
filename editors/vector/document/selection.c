/** @file @brief Model selection bookkeeping; no widget or native event knowledge. */
#include "selection.h"

bool vector_document_is_selected(const VectorDocument *document, unsigned id) {
    if (!id) {
        return false;
    }
    if (!document->selection.count) {
        return document->active_rectangle == id;
    }
    for (int i = 0; i < document->selection.count; i++) {
        if (document->selection.ids[i] == id) {
            return true;
        }
    }
    return false;
}

void vector_document_select(VectorDocument *document, unsigned id) {
    document->selection = (VectorRectangleSelection){0};
    document->active_rectangle = 0;
    VectorRectangle *rectangle = vector_rectangle_find(document, id);
    if (rectangle) {
        document->selection.ids[0] = id;
        document->selection.count = 1;
        document->active_rectangle = id;
        document->active_artboard = rectangle->parent_id;
    }
}

void vector_document_toggle(VectorDocument *document, unsigned id) {
    VectorRectangle *rectangle = vector_rectangle_find(document, id);
    if (!rectangle ||
        vector_rectangle_effective_flags(document, rectangle) & VECTOR_OBJECT_STATE_MASK) {
        return;
    }
    if (!document->selection.count && document->active_rectangle) {
        vector_document_select(document, document->active_rectangle);
    }
    for (int i = 0; i < document->selection.count; i++) {
        if (document->selection.ids[i] == id) {
            for (int j = i; j + 1 < document->selection.count; j++) {
                document->selection.ids[j] = document->selection.ids[j + 1];
            }
            document->selection.count--;
            document->active_rectangle =
                document->selection.count ? document->selection.ids[document->selection.count - 1]
                                          : 0;
            VectorRectangle *primary = vector_rectangle_find(document, document->active_rectangle);
            document->active_artboard = primary ? primary->parent_id : 0;
            return;
        }
    }
    if (document->selection.count < VECTOR_MAX_RECTANGLES) {
        document->selection.ids[document->selection.count++] = id;
        document->active_rectangle = id;
        document->active_artboard = rectangle->parent_id;
    }
}

void vector_document_selection_sync(VectorDocument *document) {
    if (!document->active_rectangle) {
        document->selection = (VectorRectangleSelection){0};
        return;
    }
    if (!document->selection.count ||
        !vector_document_is_selected(document, document->active_rectangle)) {
        vector_document_select(document, document->active_rectangle);
        return;
    }
    bool group = document->selection.count > 1;
    int count = 0;
    bool primary_found = false;
    for (int i = 0; i < document->selection.count; i++) {
        VectorRectangle *rectangle = vector_rectangle_find(document, document->selection.ids[i]);
        if (rectangle && (!group || !(vector_rectangle_effective_flags(document, rectangle) &
                                      VECTOR_OBJECT_STATE_MASK))) {
            document->selection.ids[count++] = rectangle->id;
            primary_found |= rectangle->id == document->active_rectangle;
        }
    }
    document->selection.count = count;
    if (!primary_found) {
        document->active_rectangle = count ? document->selection.ids[count - 1] : 0;
    }
    VectorRectangle *primary = vector_rectangle_find(document, document->active_rectangle);
    document->active_artboard = primary ? primary->parent_id : 0;
}
