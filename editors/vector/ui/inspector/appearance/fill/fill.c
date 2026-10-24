/** @file @brief Fill form-to-command bindings. */
#include "fill.h"
#include "editors/vector/object/commands.h"
#include "editors/vector/workspace/selection.h"
#include <errno.h>
#include <math.h>
#include <stdlib.h>
#include <string.h>

bool vector_inspector_fill_action(VectorDocument *document, VectorShape *rectangle, PUIView *view,
                                  const char *action) {
    bool fill = !strcmp(action, "vector.rectangle.fill");
    bool preview = !strcmp(action, "vector.rectangle.fill_preview");
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
        bool changed;
        if (rectangle->gradient.kind) {
            VectorShape value = *rectangle;
            value.fill = color;
            value.gradient.kind = VECTOR_GRADIENT_SOLID;
            changed = vector_shape_update(document, &value);
        } else {
            changed = vector_rectangle_set_fill(document, rectangle->id, color);
        }
        pui_view_set_text(view, "status",
                          changed ? "Fuellfarbe aktualisiert"
                                  : "Rechteck oder Artboard ist gesperrt");
        return true;
    }
    return false;
}

void vector_inspector_fill_sync(VectorDocument *document, VectorShape *rectangle,
                                const VectorShape *previous, PUIView *view, bool reset) {
    if (reset || previous->fill != rectangle->fill) {
        uint32_t current;
        if (reset || !pui_view_color(view, "rectangle_fill", &current) ||
            current != rectangle->fill) {
            document->fill_preview = (VectorFillPreview){0};
            pui_view_set_color(view, "rectangle_fill", rectangle->fill);
        }
    }
}
