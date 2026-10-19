/** @file @brief Grid preference input validation and control synchronization. */
#include "grid.h"
#include <errno.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static void number(PUIView *view, const char *id, float value) {
    char text[32];
    snprintf(text, sizeof text, "%.7g", (double)value);
    pui_view_set_text(view, id, text);
}

bool vector_grid_action(VectorDocument *document, PUIView *view, const char *action) {
    if (strncmp(action, "vector.grid.", 12)) {
        return false;
    }
    if (!document) {
        return true;
    }
    VectorGrid *grid = &document->grid;
    if (!strcmp(action, "vector.grid.toggle")) {
        grid->visible = !grid->visible;
    } else if (!strcmp(action, "vector.grid.snap")) {
        grid->snap_enabled = !grid->snap_enabled;
    } else if (!strcmp(action, "vector.grid.tolerance")) {
        const char *text = pui_view_text(view, "vector_grid_tolerance");
        char *end;
        errno = 0;
        float value = text ? strtof(text, &end) : -1;
        if (text && end != text && !*end && !errno && isfinite(value) && value >= 0 &&
            value <= 100) {
            grid->snap_tolerance = value;
        } else {
            pui_view_set_text(view, "status", "Einrasttoleranz: 0 bis 100 Bildschirm-Pixel");
        }
        number(view, "vector_grid_tolerance", grid->snap_tolerance);
    } else if (!strcmp(action, "vector.grid.type")) {
        const char *value = pui_view_value(view, "vector_grid_type");
        grid->type = value && !strcmp(value, "hexagonal")   ? VECTOR_GRID_HEXAGONAL
                     : value && !strcmp(value, "isometric") ? VECTOR_GRID_ISOMETRIC
                                                            : VECTOR_GRID_STANDARD;
    } else if (!strcmp(action, "vector.grid.color")) {
        pui_view_color(view, "vector_grid_color", &grid->color);
    } else if (!strcmp(action, "vector.grid.width") || !strcmp(action, "vector.grid.height")) {
        bool width = !strcmp(action, "vector.grid.width");
        const char *id = width ? "vector_grid_width" : "vector_grid_height";
        const char *text = pui_view_text(view, id);
        char *end;
        errno = 0;
        float value = text ? strtof(text, &end) : 0;
        bool valid = text && end != text && !*end && !errno && isfinite(value) && value >= 1 &&
                     value <= 10000;
        float *target = width ? &grid->width : &grid->height;
        if (valid) {
            *target = value;
        } else {
            pui_view_set_text(view, "status", "Rastergroesse: 1 bis 10000 px");
        }
        number(view, id, *target);
    }
    return true;
}

void vector_grid_sync(PUIView *view, const VectorDocument *document, unsigned *presented_id) {
    pui_view_set_checked(view, "vector_grid_toggle", document && document->grid.visible);
    pui_view_set_checked(view, "vector_grid_snap", document && document->grid.snap_enabled);
    if (*presented_id == (document ? document->id : 0)) {
        return;
    }
    *presented_id = document ? document->id : 0;
    if (document) {
        pui_view_set_value(view, "vector_grid_type",
                           document->grid.type == VECTOR_GRID_HEXAGONAL   ? "hexagonal"
                           : document->grid.type == VECTOR_GRID_ISOMETRIC ? "isometric"
                                                                          : "standard");
        number(view, "vector_grid_tolerance", document->grid.snap_tolerance);
        number(view, "vector_grid_width", document->grid.width);
        number(view, "vector_grid_height", document->grid.height);
        pui_view_set_color(view, "vector_grid_color", document->grid.color);
    }
}
