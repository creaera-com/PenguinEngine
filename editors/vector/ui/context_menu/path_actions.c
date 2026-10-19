/** @file @brief Native UI names remain outside path and object domain operations. */
#include "path_actions.h"
#include "editors/vector/artboard/tool.h"
#include "editors/vector/path/operations.h"
#include <string.h>

static const struct {
    const char *name, *widget;
    VectorPathOperation operation;
} operations[] = {{"vector.objects.path_convert", "vector_path_convert", VECTOR_PATH_CONVERT},
                  {"vector.objects.path_stroke", "vector_path_stroke", VECTOR_PATH_STROKE},
                  {"vector.objects.path_union", "vector_path_union", VECTOR_PATH_UNION},
                  {"vector.objects.path_subtract", "vector_path_subtract", VECTOR_PATH_SUBTRACT},
                  {"vector.objects.path_intersect", "vector_path_intersect", VECTOR_PATH_INTERSECT},
                  {"vector.objects.path_xor", "vector_path_xor", VECTOR_PATH_XOR},
                  {"vector.objects.path_divide", "vector_path_divide", VECTOR_PATH_DIVIDE}};

bool vector_path_actions_action(VectorDocument *d, PUIView *view, const char *name) {
    for (size_t i = 0; i < sizeof operations / sizeof *operations; i++) {
        if (!strcmp(name, operations[i].name)) {
            if (d) {
                vector_artboard_tool_cancel(d);
                bool ok = vector_paths_operate(d, operations[i].operation);
                pui_view_set_text(view, "status",
                                  ok ? "Pfadoperation abgeschlossen"
                                     : "Keine Aenderung: Auswahl, gemeinsames Artboard, Sperre "
                                       "oder Pfadlimit (128 Punkte/32 Konturen) pruefen");
            }
            return true;
        }
    }
    return false;
}

void vector_path_actions_sync(VectorDocument *d, PUIView *view) {
    bool any = false;
    for (size_t i = 0; i < sizeof operations / sizeof *operations; i++) {
        bool enabled = d && vector_paths_can_operate(d, operations[i].operation);
        pui_view_set_radial_enabled(view, operations[i].widget, enabled);
        any |= enabled;
    }
    pui_view_set_radial_enabled(view, "vector_paths_menu", any);
}
