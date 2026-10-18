/** @file @brief Rectangle properties remain separate from artboard inspector bindings. */
#include "rectangle_inspector.h"
#include "editors/vector/artboard/tool.h"
#include <errno.h>
#include <math.h>
#include <stdlib.h>
#include <string.h>

static bool dimension(const char *text, float *value) {
    if (!text || !*text) {
        return false;
    }
    char *end;
    errno = 0;
    *value = strtof(text, &end);
    return !errno && end != text && !*end && isfinite(*value) && *value >= 1;
}

void vector_rectangle_inspector_cancel_color(VectorRectangleInspector *inspector,
                                             VectorDocuments *documents, PUIView *view) {
    VectorDocument *document = vector_documents_find(documents, inspector->document_id);
    VectorRectangle *rectangle =
        document ? vector_rectangle_find(document, inspector->rectangle_id) : NULL;
    if (document) {
        document->fill_preview = (VectorFillPreview){0};
    }
    if (rectangle && pui_view_color_editing(view, "rectangle_fill")) {
        pui_view_set_color(view, "rectangle_fill", rectangle->fill);
    }
}

bool vector_rectangle_inspector_action(VectorRectangleInspector *inspector,
                                       VectorDocuments *documents, PUIView *view,
                                       const char *action) {
    bool fill = !strcmp(action, "vector.rectangle.fill"),
         preview = !strcmp(action, "vector.rectangle.fill_preview");
    bool name = !strcmp(action, "vector.rectangle.name"),
         width = !strcmp(action, "vector.rectangle.width"),
         height = !strcmp(action, "vector.rectangle.height");
    bool visible = !strcmp(action, "vector.rectangle.visible"),
         locked = !strcmp(action, "vector.rectangle.locked");
    if (!fill && !preview && !name && !width && !height && !visible && !locked) {
        return false;
    }
    VectorDocument *document = vector_documents_find(documents, inspector->document_id);
    VectorRectangle *rectangle =
        document ? vector_rectangle_find(document, inspector->rectangle_id) : NULL;
    if (!rectangle) {
        return true;
    }
    if (fill || preview) {
        uint32_t color;
        if (!pui_view_color(view, "rectangle_fill", &color)) {
            return true;
        }
        if (preview) {
            document->fill_preview = (VectorFillPreview){
                .rectangle_id = rectangle->id,
                .color = color,
                .active = pui_view_color_editing(view, "rectangle_fill") &&
                          !(vector_rectangle_effective_flags(document, rectangle) &
                            VECTOR_OBJECT_LOCKED)};
            return true;
        }
        document->fill_preview = (VectorFillPreview){0};
        bool changed = vector_rectangle_set_fill(document, rectangle->id, color);
        inspector->dirty |= 8u;
        pui_view_set_text(view, "status",
                          changed ? "Fuellfarbe aktualisiert"
                                  : "Rechteck oder Artboard ist gesperrt");
        return true;
    }
    bool changed = false;
    if (visible || locked) {
        unsigned bit = visible ? VECTOR_OBJECT_HIDDEN : VECTOR_OBJECT_LOCKED;
        bool enabled = pui_view_checked(view, visible ? "rectangle_visible" : "rectangle_locked");
        if (visible) {
            enabled = !enabled;
        }
        vector_artboard_tool_cancel(document);
        changed = vector_rectangle_set_state(
            document, rectangle->id, enabled ? rectangle->flags | bit : rectangle->flags & ~bit);
    } else {
        PUIRect rect = rectangle->rect;
        const char *text = name ? pui_view_text(view, "rectangle_name") : rectangle->name;
        bool valid = !width || dimension(pui_view_text(view, "rectangle_width"), &rect.w);
        valid = valid && (!height || dimension(pui_view_text(view, "rectangle_height"), &rect.h));
        changed = valid && vector_rectangle_update(document, rectangle->id, text, rect);
        inspector->dirty |= name ? 1u : width ? 2u : 4u;
    }
    pui_view_set_text(view, "status",
                      changed ? "Rechteck aktualisiert"
                              : "Rechteck gesperrt oder ungueltiger Wert");
    return true;
}

bool vector_rectangle_inspector_sync(VectorRectangleInspector *inspector, VectorDocument *document,
                                     PUIView *view, PUIContext *input, char *error,
                                     size_t capacity) {
    VectorRectangle *rectangle =
        document ? vector_rectangle_find(document, document->active_rectangle) : NULL;
    bool mount = rectangle != NULL;
    bool switched = inspector->document_id != (document ? document->id : 0) ||
                    inspector->rectangle_id != (rectangle ? rectangle->id : 0);
    if (mount != inspector->mounted) {
        if (!pui_view_replace_slot(view, input, "selection_properties",
                                   mount ? inspector->layout : NULL, error, capacity)) {
            return false;
        }
        inspector->mounted = mount;
        switched = true;
    }
    if (rectangle) {
        char value[64];
        if (switched || (inspector->dirty & 1u) ||
            strcmp(inspector->presented.name, rectangle->name)) {
            pui_view_set_text(view, "rectangle_name", rectangle->name);
        }
        if (switched || (inspector->dirty & 2u) ||
            inspector->presented.rect.w != rectangle->rect.w) {
            snprintf(value, sizeof value, "%.7g", (double)rectangle->rect.w);
            pui_view_set_text(view, "rectangle_width", value);
        }
        if (switched || (inspector->dirty & 4u) ||
            inspector->presented.rect.h != rectangle->rect.h) {
            snprintf(value, sizeof value, "%.7g", (double)rectangle->rect.h);
            pui_view_set_text(view, "rectangle_height", value);
        }
        pui_view_set_checked(view, "rectangle_visible", !(rectangle->flags & VECTOR_OBJECT_HIDDEN));
        pui_view_set_checked(view, "rectangle_locked",
                             (rectangle->flags & VECTOR_OBJECT_LOCKED) != 0);
        pui_view_set_text(view, "rectangle_edit_hint",
                          vector_rectangle_effective_flags(document, rectangle) &
                                  VECTOR_OBJECT_LOCKED
                              ? "Objekt oder Artboard gesperrt."
                              : "");
        if (switched || (inspector->dirty & 8u) || inspector->presented.fill != rectangle->fill) {
            uint32_t current;
            if (switched || !pui_view_color(view, "rectangle_fill", &current) ||
                current != rectangle->fill) {
                document->fill_preview = (VectorFillPreview){0};
                pui_view_set_color(view, "rectangle_fill", rectangle->fill);
            }
        }
        inspector->presented = *rectangle;
    }
    inspector->document_id = document ? document->id : 0;
    inspector->rectangle_id = rectangle ? rectangle->id : 0;
    inspector->dirty = 0;
    return true;
}
