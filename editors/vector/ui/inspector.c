/** @file @brief Optional artboard property panel and committed field-to-command bindings. */
#include "inspector.h"
#include "editors/vector/workspace/selection.h"

#include <errno.h>
#include <math.h>
#include <stdlib.h>
#include <string.h>

PUILayout *vector_inspector_load(const EditorServices *services, char *error, size_t capacity) {
    return editor_load_ui(services, "editors/vector/ui/inspector.yaml", error, capacity);
}

static VectorArtboard *selected(VectorDocument *document, unsigned id) {
    if (document) {
        for (int i = 0; i < document->artboard_count; i++) {
            if (document->artboards[i].id == id) {
                return &document->artboards[i];
            }
        }
    }
    return NULL;
}

static bool dimension(const char *text, float *value) {
    if (!text || !*text) {
        return false;
    }
    char *end;
    errno = 0;
    *value = strtof(text, &end);
    return !errno && end != text && !*end && isfinite(*value) && *value >= 1;
}

void vector_inspector_cancel_color(VectorInspector *inspector, VectorDocuments *documents,
                                   PUIView *view) {
    vector_rectangle_inspector_cancel_color(&inspector->rectangle, documents, view);
}

bool vector_inspector_action(VectorInspector *inspector, VectorDocuments *documents, PUIView *view,
                             const char *action) {
    if (vector_rectangle_inspector_action(&inspector->rectangle, documents, view, action)) {
        return true;
    }
    bool visibility = !strcmp(action, "vector.artboard.visible"),
         lock = !strcmp(action, "vector.artboard.locked");
    if (!visibility && !lock && strcmp(action, "vector.artboard.name") &&
        strcmp(action, "vector.artboard.width") && strcmp(action, "vector.artboard.height")) {
        return false;
    }
    VectorDocument *document = vector_documents_find(documents, inspector->document_id);
    VectorArtboard *board = selected(document, inspector->artboard_id);
    if (!board) {
        return true;
    }
    if (visibility || lock) {
        unsigned flags = board->flags;
        unsigned bit = visibility ? VECTOR_OBJECT_HIDDEN : VECTOR_OBJECT_LOCKED;
        bool enabled = pui_view_checked(view, visibility ? "artboard_visible" : "artboard_locked");
        if (visibility) {
            enabled = !enabled;
        }
        flags = enabled ? (flags | bit) : (flags & ~bit);
        vector_selection_cancel(document);
        bool changed = vector_artboard_set_state(document, board->id, flags);
        pui_view_set_text(view, "status",
                          changed ? "Objektstatus aktualisiert"
                                  : "Objektstatus konnte nicht geaendert werden");
        return true;
    }
    const char *name = board->name;
    float width = board->rect.w, height = board->rect.h;
    bool valid = true;
    if (!strcmp(action, "vector.artboard.name")) {
        name = pui_view_text(view, "artboard_name");
    }
    if (!strcmp(action, "vector.artboard.width")) {
        valid = dimension(pui_view_text(view, "artboard_width"), &width);
    }
    if (!strcmp(action, "vector.artboard.height")) {
        valid = dimension(pui_view_text(view, "artboard_height"), &height);
    }
    bool changed = valid && vector_artboard_update(document, board->id, name, width, height);
    inspector->dirty |= !strcmp(action, "vector.artboard.name")    ? 1u
                        : !strcmp(action, "vector.artboard.width") ? 2u
                                                                   : 4u;
    pui_view_set_text(view, "status",
                      changed ? "Artboard aktualisiert"
                      : (board->flags & VECTOR_OBJECT_LOCKED)
                          ? "Artboard ist gesperrt"
                          : "Ungueltiger Wert (Name: 1-63 Bytes, Groesse: 1-1000000 px)");
    return true;
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
            inspector->mounted = inspector->rectangle.mounted = false;
            inspector->document_id = inspector->artboard_id = 0;
            inspector->rectangle.document_id = inspector->rectangle.rectangle_id = 0;
            inspector->multi_mounted = true;
        }
        char text[64];
        snprintf(text, sizeof text, "%d Rechtecke ausgewaehlt", document->selection.count);
        pui_view_set_text(view, "vector_selection_count", text);
        return true;
    }
    if (inspector->multi_mounted) {
        if (!pui_view_replace_slot(view, input, "selection_properties", NULL, error, capacity)) {
            return false;
        }
        inspector->multi_mounted = false;
    }
    bool rectangle = document && vector_rectangle_find(document, document->active_rectangle);
    if (rectangle && inspector->mounted) {
        if (!pui_view_replace_slot(view, input, "selection_properties", NULL, error, capacity)) {
            return false;
        }
        inspector->mounted = false;
    }
    if (!vector_rectangle_inspector_sync(&inspector->rectangle, document, view, input, error,
                                         capacity)) {
        return false;
    }
    if (rectangle) {
        inspector->document_id = 0;
        inspector->artboard_id = 0;
        return true;
    }
    VectorArtboard *board = selected(document, document ? document->active_artboard : 0);
    bool mount = board != NULL;
    bool switched = inspector->document_id != (document ? document->id : 0) ||
                    inspector->artboard_id != (board ? board->id : 0);
    if (mount != inspector->mounted) {
        if (!pui_view_replace_slot(view, input, "selection_properties",
                                   mount ? inspector->artboard_layout : NULL, error, capacity)) {
            return false;
        }
        inspector->mounted = mount;
        switched = true;
    }
    if (board) {
        char value[64];
        /* Updating an unrelated field must preserve the newly focused input's
         * selection and any text already entered before the next frame. */
        if (switched || (inspector->dirty & 1u) || strcmp(inspector->presented.name, board->name)) {
            pui_view_set_text(view, "artboard_name", board->name);
        }
        if (switched || (inspector->dirty & 2u) || inspector->presented.rect.w != board->rect.w) {
            snprintf(value, sizeof value, "%.7g", (double)board->rect.w);
            pui_view_set_text(view, "artboard_width", value);
        }
        if (switched || (inspector->dirty & 4u) || inspector->presented.rect.h != board->rect.h) {
            snprintf(value, sizeof value, "%.7g", (double)board->rect.h);
            pui_view_set_text(view, "artboard_height", value);
        }
        pui_view_set_checked(view, "artboard_visible", !(board->flags & VECTOR_OBJECT_HIDDEN));
        pui_view_set_checked(view, "artboard_locked", (board->flags & VECTOR_OBJECT_LOCKED) != 0);
        pui_view_set_text(view, "artboard_edit_hint",
                          board->flags & VECTOR_OBJECT_LOCKED ? "Zum Bearbeiten entsperren." : "");
        inspector->presented = *board;
    }
    inspector->document_id = document ? document->id : 0;
    inspector->artboard_id = board ? board->id : 0;
    inspector->dirty = false;
    return true;
}
