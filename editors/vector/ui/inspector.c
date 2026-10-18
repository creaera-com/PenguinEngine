/** @file @brief Optional artboard property panel and committed field-to-command bindings. */
#include "inspector.h"

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

bool vector_inspector_action(VectorInspector *inspector, VectorDocuments *documents, PUIView *view,
                             const char *action) {
    if (strcmp(action, "vector.artboard.name") && strcmp(action, "vector.artboard.width") &&
        strcmp(action, "vector.artboard.height")) {
        return false;
    }
    VectorDocument *document = vector_documents_find(documents, inspector->document_id);
    VectorArtboard *board = selected(document, inspector->artboard_id);
    if (!board) {
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
                              : "Ungueltiger Wert (Name: 1-63 Bytes, Groesse: 1-1000000 px)");
    return true;
}

bool vector_inspector_sync(VectorInspector *inspector, VectorDocument *document, PUIView *view,
                           PUIContext *input, char *error, size_t capacity) {
    VectorArtboard *board = selected(document, document ? document->active_artboard : 0);
    bool mount = board != NULL;
    bool switched = inspector->document_id != (document ? document->id : 0) ||
                    inspector->artboard_id != (board ? board->id : 0);
    if (mount != inspector->mounted) {
        if (!pui_view_replace_slot(view, input, "artboard_properties",
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
        inspector->presented = *board;
    }
    inspector->document_id = document ? document->id : 0;
    inspector->artboard_id = board ? board->id : 0;
    inspector->dirty = false;
    return true;
}
