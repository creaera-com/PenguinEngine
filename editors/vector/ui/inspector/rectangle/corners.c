/** @file @brief Corners form-to-command bindings. */
#include "corners.h"
#include "editors/vector/object/commands.h"
#include "editors/vector/workspace/selection.h"
#include <errno.h>
#include <math.h>
#include <stdlib.h>
#include <string.h>

bool vector_inspector_corners_action(VectorDocument *document, VectorShape *rectangle,
                                     PUIView *view, const char *action) {
    if (strcmp(action, "vector.rectangle.radius")) {
        return false;
    }

    const char *text = pui_view_text(view, "rectangle_radius");
    char *end = NULL;
    errno = 0;
    float value = text ? strtof(text, &end) : -1;
    bool valid = text && *text && end != text && !*end && !errno && isfinite(value);
    vector_selection_cancel(document);
    bool changed = valid && vector_rectangle_set_radius(document, rectangle->id, value);
    pui_view_set_text(view, "status",
                      changed ? "Rechteck aktualisiert"
                              : "Rechteck gesperrt oder ungueltiger Wert");
    return true;
}

void vector_inspector_corners_sync(VectorShape *rectangle, const VectorShape *previous,
                                   PUIView *view, bool reset) {
    char value[64];
    if (reset || previous->corner_radius != rectangle->corner_radius) {
        snprintf(value, sizeof value, "%.7g", (double)rectangle->corner_radius);
        pui_view_set_text(view, "rectangle_radius", value);
    }
}
