/** @file @brief Rectangle properties remain separate from artboard inspector bindings. */
#include "shape.h"
#include "editors/vector/artboard/tool.h"
#include "editors/vector/polygon/polygon.h"
#include "editors/vector/workspace/selection.h"
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
    if (rectangle && pui_view_color_editing(view, "rectangle_stroke_color")) {
        pui_view_set_color(view, "rectangle_stroke_color", rectangle->stroke.color);
    }
    if (rectangle && pui_view_color_editing(view, "rectangle_fill")) {
        pui_view_set_color(view, "rectangle_fill", rectangle->fill);
    }
}

/* Translate inspector controls into one atomic domain edit. Picker changes commit on release. */
static bool stroke_action(VectorRectangleInspector *inspector, VectorDocument *document,
                          VectorRectangle *rectangle, PUIView *view, const char *action) {
    VectorStroke stroke = rectangle->stroke;
    bool valid = true;
    if (!strcmp(action, "vector.rectangle.stroke_width")) {
        const char *text = pui_view_text(view, "rectangle_stroke_width");
        char *end;
        errno = 0;
        stroke.width = text ? strtof(text, &end) : -1;
        valid = text && *text && end != text && !*end && !errno;
    } else if (!strcmp(action, "vector.rectangle.stroke_color")) {
        valid = pui_view_color(view, "rectangle_stroke_color", &stroke.color);
    } else if (!strcmp(action, "vector.rectangle.stroke_inside")) {
        stroke.alignment = VECTOR_STROKE_INSIDE;
    } else if (!strcmp(action, "vector.rectangle.stroke_center")) {
        stroke.alignment = VECTOR_STROKE_CENTER;
    } else if (!strcmp(action, "vector.rectangle.stroke_outside")) {
        stroke.alignment = VECTOR_STROKE_OUTSIDE;
    } else {
        return false;
    }
    vector_selection_cancel(document);
    bool changed = valid && vector_rectangle_set_stroke(document, rectangle->id, stroke);
    inspector->dirty |= 32u;
    pui_view_set_text(view, "status",
                      changed ? "Rahmen aktualisiert (0 px: aus)"
                              : "Rechteck gesperrt oder ungueltiger Rahmen");
    return true;
}

bool vector_rectangle_inspector_action(VectorRectangleInspector *inspector,
                                       VectorDocuments *documents, PUIView *view,
                                       const char *action) {
    if (!strcmp(action, "vector.polygon.sides")) {
        VectorDocument *d = vector_documents_find(documents, inspector->document_id);
        const char *text = pui_view_text(view, "polygon_sides");
        char *end = NULL;
        errno = 0;
        long sides = text ? strtol(text, &end, 10) : 0;
        bool valid = text && end != text && !*end && !errno && sides >= 3 &&
                     sides <= VECTOR_POLYGON_MAX_SIDES;
        if (d) {
            vector_selection_cancel(d);
        }
        bool changed =
            d && valid && vector_polygon_set_sides(d, inspector->rectangle_id, (unsigned)sides);
        inspector->dirty |= 64u;
        pui_view_set_text(view, "status",
                          changed ? "Eckenzahl aktualisiert"
                                  : "Polygon gesperrt oder Eckenzahl ungueltig (3 bis 32)");
        return true;
    }
    bool fill = !strcmp(action, "vector.rectangle.fill"),
         preview = !strcmp(action, "vector.rectangle.fill_preview");
    bool name = !strcmp(action, "vector.rectangle.name"),
         width = !strcmp(action, "vector.rectangle.width"),
         height = !strcmp(action, "vector.rectangle.height");
    bool angle = !strcmp(action, "vector.rectangle.angle");
    bool visible = !strcmp(action, "vector.rectangle.visible"),
         locked = !strcmp(action, "vector.rectangle.locked");
    bool stroke = !strncmp(action, "vector.rectangle.stroke_", 24);
    if (!stroke && !fill && !preview && !name && !width && !height && !angle && !visible &&
        !locked) {
        return false;
    }
    VectorDocument *document = vector_documents_find(documents, inspector->document_id);
    VectorRectangle *rectangle =
        document ? vector_rectangle_find(document, inspector->rectangle_id) : NULL;
    if (!rectangle) {
        return true;
    }
    if (stroke) {
        return stroke_action(inspector, document, rectangle, view, action);
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
    if (angle) {
        const char *text = pui_view_text(view, "rectangle_angle");
        char *end;
        errno = 0;
        float value = text ? strtof(text, &end) : 0;
        bool valid = text && end != text && !*end && !errno && isfinite(value);
        vector_selection_cancel(document);
        changed = valid && vector_rectangle_set_angle(document, rectangle->id, value);
        inspector->dirty |= 16u;
    } else if (visible || locked) {
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
        inspector->polygon_mounted = false;
        switched = true;
    }
    if (rectangle) {
        bool polygon = rectangle->curve_kind == VECTOR_CURVE_POLYGON;
        if (polygon != inspector->polygon_mounted) {
            if (!pui_view_replace_slot(view, input, "polygon_properties",
                                       polygon ? inspector->polygon_layout : NULL, error,
                                       capacity)) {
                return false;
            }
            inspector->polygon_mounted = polygon;
            switched = true;
        }
        if (polygon && (switched || (inspector->dirty & 64u) ||
                        inspector->presented.curve.count != rectangle->curve.count)) {
            char corners[16];
            snprintf(corners, sizeof corners, "%u", rectangle->curve.count);
            pui_view_set_text(view, "polygon_sides", corners);
        }
        pui_view_set_text(view, "rectangle_kind",
                          rectangle->curve_kind == VECTOR_CURVE_POLYGON   ? "Polygon"
                          : rectangle->curve_kind == VECTOR_CURVE_ELLIPSE ? "Ellipse"
                          : rectangle->curve.count                        ? "Bezierkurve"
                          : rectangle->path.count                         ? "Pfad"
                                                                          : "Rechteck");
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
        bool angle_preview = document->selection_tool.rotating;
        float displayed_angle = vector_rectangle_display_angle(document, rectangle);
        if (switched || (inspector->dirty & 16u) ||
            inspector->presented.angle != rectangle->angle ||
            inspector->angle_preview != angle_preview ||
            inspector->displayed_angle != displayed_angle) {
            snprintf(value, sizeof value, "%.7g", (double)displayed_angle);
            pui_view_set_text(view, "rectangle_angle", value);
        }
        inspector->angle_preview = angle_preview;
        inspector->displayed_angle = displayed_angle;
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
        if (switched || (inspector->dirty & 32u) ||
            inspector->presented.stroke.width != rectangle->stroke.width) {
            snprintf(value, sizeof value, "%.7g", (double)rectangle->stroke.width);
            pui_view_set_text(view, "rectangle_stroke_width", value);
        }
        const char *alignment =
            rectangle->stroke.alignment == VECTOR_STROKE_INSIDE    ? "rectangle_stroke_inside"
            : rectangle->stroke.alignment == VECTOR_STROKE_OUTSIDE ? "rectangle_stroke_outside"
                                                                   : "rectangle_stroke_center";
        pui_view_set_checked(view, alignment, true);
        if (switched || (inspector->dirty & 32u) ||
            inspector->presented.stroke.color != rectangle->stroke.color) {
            pui_view_set_color(view, "rectangle_stroke_color", rectangle->stroke.color);
        }
        inspector->presented = *rectangle;
    }
    inspector->document_id = document ? document->id : 0;
    inspector->rectangle_id = rectangle ? rectangle->id : 0;
    inspector->dirty = 0;
    return true;
}
