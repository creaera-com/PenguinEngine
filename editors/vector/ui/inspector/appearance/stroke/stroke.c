/** @file @brief Stroke form-to-command bindings. */
#include "stroke.h"
#include "editors/vector/object/commands.h"
#include "editors/vector/workspace/selection.h"
#include <errno.h>
#include <math.h>
#include <stdlib.h>
#include <string.h>

bool vector_inspector_stroke_action(VectorDocument *document, VectorShape *rectangle, PUIView *view,
                                    const char *action) {
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
    pui_view_set_text(view, "status",
                      changed ? "Rahmen aktualisiert (0 px: aus)"
                              : "Rechteck gesperrt oder ungueltiger Rahmen");
    return true;
}

void vector_inspector_stroke_sync(VectorShape *rectangle, const VectorShape *previous,
                                  PUIView *view, bool reset) {
    char value[64];
    if (reset || previous->stroke.width != rectangle->stroke.width) {
        snprintf(value, sizeof value, "%.7g", (double)rectangle->stroke.width);
        pui_view_set_text(view, "rectangle_stroke_width", value);
    }
    const char *alignment =
        rectangle->stroke.alignment == VECTOR_STROKE_INSIDE    ? "rectangle_stroke_inside"
        : rectangle->stroke.alignment == VECTOR_STROKE_OUTSIDE ? "rectangle_stroke_outside"
                                                               : "rectangle_stroke_center";
    pui_view_set_checked(view, alignment, true);
    if (reset || previous->stroke.color != rectangle->stroke.color) {
        pui_view_set_color(view, "rectangle_stroke_color", rectangle->stroke.color);
    }
}
