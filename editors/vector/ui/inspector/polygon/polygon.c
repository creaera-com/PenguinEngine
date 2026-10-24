/** @file @brief Polygon form-to-command bindings. */
#include "polygon.h"
#include "editors/vector/object/commands.h"
#include "editors/vector/workspace/selection.h"
#include <errno.h>
#include <math.h>
#include <stdlib.h>
#include <string.h>

#include "editors/vector/polygon/polygon.h"

bool vector_inspector_polygon_action(VectorDocument *document, VectorShape *shape, PUIView *view,
                                     const char *action) {
    if (!strcmp(action, "vector.polygon.sides")) {
        VectorDocument *d = document;
        const char *text = pui_view_text(view, "polygon_sides");
        char *end = NULL;
        errno = 0;
        long sides = text ? strtol(text, &end, 10) : 0;
        bool valid = text && end != text && !*end && !errno && sides >= 3 &&
                     sides <= VECTOR_POLYGON_MAX_SIDES;
        if (d) {
            vector_selection_cancel(d);
        }
        bool changed = d && valid && vector_polygon_set_sides(d, shape->id, (unsigned)sides);
        pui_view_set_text(view, "status",
                          changed ? "Eckenzahl aktualisiert"
                                  : "Polygon gesperrt oder Eckenzahl ungueltig (3 bis 32)");
        return true;
    }
    return false;
}

void vector_inspector_polygon_sync(VectorShape *shape, const VectorShape *previous, PUIView *view,
                                   bool reset) {
    if (reset || previous->curve.count != shape->curve.count) {
        char value[16];
        snprintf(value, sizeof value, "%u", shape->curve.count);
        pui_view_set_text(view, "polygon_sides", value);
    }
}
