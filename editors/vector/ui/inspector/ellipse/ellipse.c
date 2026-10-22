/** @file @brief Ellipse form parsing and synchronization, separate from geometry. */
#include "ellipse.h"
#include "editors/vector/ellipse/ellipse.h"
#include "editors/vector/workspace/selection.h"
#include <errno.h>
#include <math.h>
#include <stdlib.h>
#include <string.h>

bool vector_inspector_ellipse_action(VectorDocument *document, VectorShape *shape, PUIView *view,
                                     const char *action) {
    if (strncmp(action, "vector.ellipse.", 15)) {
        return false;
    }
    VectorEllipse value = shape->ellipse;
    bool valid = true;
    if (!strcmp(action, "vector.ellipse.mode")) {
        const char *mode = pui_view_value(view, "ellipse_mode");
        value.mode = mode && !strcmp(mode, "arc")     ? VECTOR_ELLIPSE_ARC
                     : mode && !strcmp(mode, "pie")   ? VECTOR_ELLIPSE_PIE
                     : mode && !strcmp(mode, "chord") ? VECTOR_ELLIPSE_CHORD
                                                      : VECTOR_ELLIPSE_FULL;
        if (value.start == value.end) {
            value.start = 0;
            value.end = 360;
        }
    } else {
        bool start = !strcmp(action, "vector.ellipse.start");
        const char *text = pui_view_text(view, start ? "ellipse_start" : "ellipse_end");
        char *end;
        errno = 0;
        float angle = text ? strtof(text, &end) : NAN;
        valid = text && end != text && !*end && !errno && isfinite(angle);
        if (start) {
            value.start = angle;
        } else {
            value.end = angle;
        }
    }
    vector_selection_cancel(document);
    bool changed = valid && vector_ellipse_set_segment(document, shape->id, value);
    pui_view_set_text(view, "status",
                      changed ? "Ellipsensegment aktualisiert"
                              : "Ungueltige Winkel: -360 bis 360, Winkelspanne 0.01 bis 360 Grad");
    return true;
}

void vector_inspector_ellipse_sync(const VectorShape *shape, const VectorShape *previous,
                                   PUIView *view, bool reset) {
    if (reset || memcmp(&shape->ellipse, &previous->ellipse, sizeof shape->ellipse)) {
        static const char *modes[] = {"full", "arc", "pie", "chord"};
        char text[32];
        pui_view_set_value(view, "ellipse_mode", modes[shape->ellipse.mode]);
        snprintf(text, sizeof text, "%.7g", (double)shape->ellipse.start);
        pui_view_set_text(view, "ellipse_start", text);
        float end =
            shape->ellipse.mode == VECTOR_ELLIPSE_FULL && shape->ellipse.start == shape->ellipse.end
                ? 360
                : shape->ellipse.end;
        snprintf(text, sizeof text, "%.7g", (double)end);
        pui_view_set_text(view, "ellipse_end", text);
    }
}
