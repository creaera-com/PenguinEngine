/** @file @brief Common object fields and composition of optional property sections. */
#include "object.h"
#include "../appearance/effects/effects.h"
#include "../appearance/fill/fill.h"
#include "../appearance/stroke/stroke.h"
#include "../polygon/polygon.h"
#include "../rectangle/corners.h"
#include "editors/vector/artboard/tool.h"
#include "editors/vector/gradient/tool.h"
#include "editors/vector/object/commands.h"
#include "editors/vector/workspace/selection.h"
#include <errno.h>
#include <math.h>
#include <stdlib.h>
#include <string.h>

enum {
    FIELD_NAME = 1u,
    FIELD_WIDTH = 2u,
    FIELD_HEIGHT = 4u,
    FIELD_FILL = 8u,
    FIELD_ANGLE = 16u,
    FIELD_STROKE = 32u,
    FIELD_POLYGON = 64u,
    FIELD_EFFECTS = 128u,
    FIELD_CORNERS = 256u
};

static bool dimension(const char *text, float *value) {
    if (!text || !*text) {
        return false;
    }
    char *end;
    errno = 0;
    *value = strtof(text, &end);
    return !errno && end != text && !*end && isfinite(*value) && *value >= 1;
}

void vector_object_inspector_cancel_color(VectorObjectInspector *inspector,
                                          VectorDocuments *documents, PUIView *view) {
    VectorDocument *document = vector_documents_find(documents, inspector->document_id);
    VectorRectangle *rectangle =
        document ? vector_rectangle_find(document, inspector->object_id) : NULL;
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

bool vector_object_inspector_action(VectorObjectInspector *inspector, VectorDocuments *documents,
                                    PUIView *view, const char *action) {
    if (vector_gradient_inspector_action(&inspector->gradient,
                                         vector_documents_find(documents, inspector->document_id),
                                         inspector->object_id, view, action)) {
        return true;
    }
    if (vector_effects_action(vector_documents_find(documents, inspector->document_id),
                              inspector->object_id, view, action)) {
        inspector->dirty |= FIELD_EFFECTS;
        return true;
    }
    if (vector_text_inspector_action(&inspector->text,
                                     vector_documents_find(documents, inspector->document_id),
                                     inspector->object_id, view, action)) {
        return true;
    }
    VectorDocument *selected_document = vector_documents_find(documents, inspector->document_id);
    VectorShape *selected_shape =
        selected_document ? vector_rectangle_find(selected_document, inspector->object_id) : NULL;
    if (!strcmp(action, "vector.polygon.sides")) {
        if (selected_shape) {
            vector_inspector_polygon_action(selected_document, selected_shape, view, action);
        }
        inspector->dirty |= FIELD_POLYGON;
        return true;
    }
    bool fill = !strcmp(action, "vector.rectangle.fill"),
         preview = !strcmp(action, "vector.rectangle.fill_preview");
    bool name = !strcmp(action, "vector.rectangle.name"),
         width = !strcmp(action, "vector.rectangle.width"),
         height = !strcmp(action, "vector.rectangle.height");
    bool radius = !strcmp(action, "vector.rectangle.radius");
    bool angle = !strcmp(action, "vector.rectangle.angle");
    bool visible = !strcmp(action, "vector.rectangle.visible"),
         locked = !strcmp(action, "vector.rectangle.locked");
    bool stroke = !strncmp(action, "vector.rectangle.stroke_", 24);
    if (!radius && !stroke && !fill && !preview && !name && !width && !height && !angle &&
        !visible && !locked) {
        return false;
    }
    VectorDocument *document = vector_documents_find(documents, inspector->document_id);
    VectorRectangle *rectangle =
        document ? vector_rectangle_find(document, inspector->object_id) : NULL;
    if (!rectangle) {
        return true;
    }
    if (stroke) {
        inspector->dirty |= FIELD_STROKE;
        return vector_inspector_stroke_action(document, rectangle, view, action);
    }
    if (fill || preview) {
        if (fill) {
            inspector->dirty |= FIELD_FILL;
        }
        return vector_inspector_fill_action(document, rectangle, view, action);
    }
    bool changed = false;
    if (radius) {
        inspector->dirty |= FIELD_CORNERS;
        return vector_inspector_corners_action(document, rectangle, view, action);
    } else if (angle) {
        const char *text = pui_view_text(view, "rectangle_angle");
        char *end;
        errno = 0;
        float value = text ? strtof(text, &end) : 0;
        bool valid = text && end != text && !*end && !errno && isfinite(value);
        vector_selection_cancel(document);
        changed = valid && vector_rectangle_set_angle(document, rectangle->id, value);
        inspector->dirty |= FIELD_ANGLE;
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
        inspector->dirty |= name ? FIELD_NAME : width ? FIELD_WIDTH : FIELD_HEIGHT;
    }
    pui_view_set_text(view, "status",
                      changed ? "Rechteck aktualisiert"
                              : "Rechteck gesperrt oder ungueltiger Wert");
    return true;
}

bool vector_object_inspector_sync(VectorObjectInspector *inspector, VectorDocument *document,
                                  PUIView *view, PUIContext *input, char *error, size_t capacity) {
    VectorRectangle *rectangle =
        document ? vector_rectangle_find(document, document->active_rectangle) : NULL;
    bool mount = rectangle != NULL;
    bool switched = inspector->document_id != (document ? document->id : 0) ||
                    inspector->object_id != (rectangle ? rectangle->id : 0);
    bool image = rectangle && *rectangle->image.source;
    if (mount != inspector->mounted || image != inspector->image_mounted) {
        if (!pui_view_replace_slot(view, input, "selection_properties",
                                   mount ? (image ? inspector->image_layout : inspector->layout)
                                         : NULL,
                                   error, capacity)) {
            return false;
        }
        inspector->mounted = mount;
        inspector->image_mounted = image;
        inspector->polygon_mounted = false;
        inspector->corners_mounted = false;
        inspector->gradient.mounted = false;
        switched = true;
    }
    if (rectangle && !vector_text_inspector_sync(&inspector->text, document, rectangle, view, input,
                                                 switched, error, capacity)) {
        return false;
    }
    if (rectangle) {
        bool gradient = vector_gradient_eligible(rectangle);
        if (gradient != inspector->gradient.mounted) {
            if (!pui_view_replace_slot(view, input, "object_gradient",
                                       gradient ? inspector->gradient_layout : NULL, error,
                                       capacity)) {
                return false;
            }
            inspector->gradient.mounted = gradient;
            switched = true;
        }
        if (gradient) {
            if (document->gradient_tool.dragging &&
                document->gradient_tool.shape_id == rectangle->id) {
                unsigned handle = document->gradient_tool.handle;
                unsigned stop = handle > 1 ? handle - 2
                                : handle   ? rectangle->gradient.count - 1
                                           : 0;
                if (stop < rectangle->gradient.count && inspector->gradient.stop != stop) {
                    inspector->gradient.stop = stop;
                    inspector->gradient.dirty = true;
                }
            }
            vector_gradient_inspector_sync(&inspector->gradient, view, &rectangle->gradient,
                                           &inspector->presented.gradient, switched);
        }
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
        if (polygon) {
            vector_inspector_polygon_sync(rectangle, &inspector->presented, view,
                                          switched || (inspector->dirty & FIELD_POLYGON));
        }
        pui_view_set_text(view, "rectangle_kind",
                          *rectangle->image.source                        ? "Bild"
                          : rectangle->clip_mask                          ? "Schnittmaske"
                          : rectangle->text.enabled                       ? "Textfeld"
                          : rectangle->curve_kind == VECTOR_CURVE_POLYGON ? "Polygon"
                          : rectangle->curve_kind == VECTOR_CURVE_ELLIPSE ? "Ellipse"
                          : rectangle->curve.count                        ? "Bezierkurve"
                          : rectangle->path.count                         ? "Pfad"
                                                                          : "Rechteck");
        bool parametric_rect = !rectangle->curve.count && !rectangle->path.count &&
                               !rectangle->text.enabled && !*rectangle->image.source;
        if (parametric_rect != inspector->corners_mounted) {
            if (!pui_view_replace_slot(view, input, "rectangle_corner_properties",
                                       parametric_rect ? inspector->corners_layout : NULL, error,
                                       capacity)) {
                return false;
            }
            inspector->corners_mounted = parametric_rect;
            switched = true;
        }
        char value[64];
        vector_inspector_corners_sync(rectangle, &inspector->presented, view,
                                      switched || (inspector->dirty & FIELD_CORNERS));
        if (switched || (inspector->dirty & FIELD_NAME) ||
            strcmp(inspector->presented.name, rectangle->name)) {
            pui_view_set_text(view, "rectangle_name", rectangle->name);
        }
        if (switched || (inspector->dirty & FIELD_WIDTH) ||
            inspector->presented.rect.w != rectangle->rect.w) {
            snprintf(value, sizeof value, "%.7g", (double)rectangle->rect.w);
            pui_view_set_text(view, "rectangle_width", value);
        }
        if (switched || (inspector->dirty & FIELD_HEIGHT) ||
            inspector->presented.rect.h != rectangle->rect.h) {
            snprintf(value, sizeof value, "%.7g", (double)rectangle->rect.h);
            pui_view_set_text(view, "rectangle_height", value);
        }
        bool angle_preview = document->selection_tool.rotating;
        float displayed_angle = vector_rectangle_display_angle(document, rectangle);
        if (switched || (inspector->dirty & FIELD_ANGLE) ||
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
        vector_inspector_fill_sync(document, rectangle, &inspector->presented, view,
                                   switched || (inspector->dirty & FIELD_FILL));
        vector_inspector_stroke_sync(rectangle, &inspector->presented, view,
                                     switched || (inspector->dirty & FIELD_STROKE));
        vector_effects_sync(view, &rectangle->shadow, &inspector->presented.shadow,
                            switched || (inspector->dirty & FIELD_EFFECTS));
        inspector->presented = *rectangle;
    }
    inspector->document_id = document ? document->id : 0;
    inspector->object_id = rectangle ? rectangle->id : 0;
    inspector->dirty = 0;
    return true;
}
