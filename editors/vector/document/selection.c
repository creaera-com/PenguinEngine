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

static bool same_unit(const VectorRectangle *a, const VectorRectangle *b) {
    return a->id == b->id || (a->group_id && a->group_id == b->group_id);
}

static void append_unit(VectorDocument *document, const VectorRectangle *member) {
    for (int i = 0; i < document->rectangle_count; i++) {
        VectorRectangle *r = &document->rectangles[i];
        if (same_unit(member, r) && !vector_document_is_selected(document, r->id) &&
            document->selection.count < VECTOR_MAX_RECTANGLES) {
            document->selection.ids[document->selection.count++] = r->id;
        }
    }
}

void vector_document_select(VectorDocument *document, unsigned id) {
    document->selection = (VectorRectangleSelection){0};
    document->active_rectangle = 0;
    VectorRectangle *rectangle = vector_rectangle_find(document, id);
    if (rectangle) {
        append_unit(document, rectangle);
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
    if (vector_document_is_selected(document, id)) {
        int count = 0;
        for (int i = 0; i < document->selection.count; i++) {
            VectorRectangle *r = vector_rectangle_find(document, document->selection.ids[i]);
            if (r && !same_unit(rectangle, r)) {
                document->selection.ids[count++] = r->id;
            }
        }
        document->selection.count = count;
        document->active_rectangle = count ? document->selection.ids[count - 1] : 0;
        VectorRectangle *primary = vector_rectangle_find(document, document->active_rectangle);
        document->active_artboard = primary ? primary->parent_id : 0;
    } else {
        append_unit(document, rectangle);
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
        if (rectangle &&
            (rectangle->group_id || !group ||
             !(vector_rectangle_effective_flags(document, rectangle) & VECTOR_OBJECT_STATE_MASK))) {
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
    /* Preserve complete groups, including locked children: gestures then reject the whole unit. */
    for (int i = 0; i < document->selection.count; i++) {
        VectorRectangle *r = vector_rectangle_find(document, document->selection.ids[i]);
        if (r && r->group_id) {
            append_unit(document, r);
        }
    }
}
