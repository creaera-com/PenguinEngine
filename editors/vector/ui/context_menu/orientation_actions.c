/** @file @brief Menu configuration and status text stay outside geometric domain operations. */
#include "orientation_actions.h"
#include "editors/vector/artboard/tool.h"
#include "editors/vector/object/orientation.h"
#include <string.h>

static const struct {
    const char *name, *widget;
    VectorOrientation operation;
} operations[] = {
    {"vector.objects.mirror_horizontal", "vector_mirror_horizontal", VECTOR_MIRROR_HORIZONTAL},
    {"vector.objects.mirror_vertical", "vector_mirror_vertical", VECTOR_MIRROR_VERTICAL},
    {"vector.objects.rotate_left", "vector_rotate_left", VECTOR_ROTATE_LEFT},
    {"vector.objects.rotate_right", "vector_rotate_right", VECTOR_ROTATE_RIGHT}};

bool vector_orientation_actions_action(VectorDocument *d, PUIView *view, const char *name) {
    for (size_t i = 0; i < sizeof operations / sizeof *operations; i++) {
        if (!strcmp(name, operations[i].name)) {
            if (d) {
                vector_artboard_tool_cancel(d);
                bool ok = vector_objects_orient(d, operations[i].operation);
                pui_view_set_text(view, "status",
                                  ok ? "Auswahl transformiert"
                                     : "Keine Aenderung: Auswahl, Sperre oder Geometrie pruefen");
            }
            return true;
        }
    }
    return false;
}

void vector_orientation_actions_sync(VectorDocument *d, PUIView *view, bool alignment_available) {
    bool any = false;
    for (size_t i = 0; i < sizeof operations / sizeof *operations; i++) {
        bool enabled = d && vector_objects_can_orient(d, operations[i].operation);
        pui_view_set_radial_enabled(view, operations[i].widget, enabled);
        any |= enabled;
    }
    pui_view_set_radial_enabled(view, "vector_transform_menu", any || alignment_available);
}
