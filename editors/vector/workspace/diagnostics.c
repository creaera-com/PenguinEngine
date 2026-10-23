/** @file @brief Translate vector state into shared diagnostic values without exposing its model. */
#include "diagnostics.h"
#include "editors/vector/internal.h"
#include <stdio.h>

EditorDiagnostics vector_workspace_diagnostics(const void *instance, const PUIView *view) {
    const VectorEditor *editor = instance;
    EditorDiagnostics result = {0};
    /* Tab lookup only reads the collection; find via the active ID to retain const ownership. */
    const VectorDocument *d = NULL;
    for (int i = 0; i < editor->documents.count; i++) {
        const VectorDocument *candidate = &editor->documents.items[i];
        char tab[64];
        snprintf(tab, sizeof tab, "vector_document_%u", candidate->id);
        const char *active = pui_view_selected_tab(view, "vector_document_tabs");
        if (active && !SDL_strcmp(tab, active)) {
            d = candidate;
            break;
        }
    }
    if (!d) {
        return result;
    }
    result.document = d->id;
    result.objects = (unsigned)d->rectangle_count;
    for (int i = 0; i < d->rectangle_count; i++) {
        const VectorRepeat *repeat = &d->rectangles[i].repeat;
        result.instances += repeat->enabled ? repeat->rows * repeat->columns : 1;
    }
    result.selected = (unsigned)d->selection.count;
    result.active_artboard = d->active_artboard;
    result.active_object = d->active_rectangle;
    for (unsigned i = 0; i < result.selected && i < EDITOR_DIAGNOSTIC_SELECTION_CAPACITY; i++) {
        result.selected_ids[i] = d->selection.ids[i];
    }
    if (!result.selected && d->active_rectangle) {
        result.selected = 1;
        result.selected_ids[0] = d->active_rectangle;
    }
    result.viewport_available = true;
    result.viewport_x = d->viewport.x;
    result.viewport_y = d->viewport.y;
    result.zoom = d->viewport.zoom;
    result.history_cursor = d->history.cursor;
    const char *operation = "idle";
    if (d->viewport.panning) {
        operation = "viewport.pan";
    } else if (d->viewport.zooming) {
        operation = "viewport.zoom";
    } else if (d->selection_tool.dragging || d->selection_tool.marquee) {
        operation = d->selection_tool.rotating       ? "selection.rotate"
                    : d->selection_tool.resize_edges ? "selection.resize"
                    : d->selection_tool.marquee      ? "selection.marquee"
                                                     : "selection.move";
    } else if (d->rectangle_tool.dragging) {
        operation = "shape.create";
    } else if (d->artboard_tool.dragging) {
        operation = "artboard.create";
    } else if (d->curve_tool.dragging || d->curve_tool.editing) {
        operation = "curve.edit";
    } else if (d->gradient_tool.dragging) {
        operation = "gradient.edit";
    } else if (pui_view_numeric_preview(view, NULL, NULL)) {
        operation = "inspector.scrub";
    }
    result.interacting = SDL_strcmp(operation, "idle") != 0;
    snprintf(result.operation, sizeof result.operation, "%s", operation);
    return result;
}
