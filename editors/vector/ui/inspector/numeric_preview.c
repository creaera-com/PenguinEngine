/** @file @brief Preview field commands on owned values, without borrowing history payloads. */
#include "numeric_preview.h"
#include "inspector.h"
#include <stdlib.h>
#include <string.h>

void vector_numeric_preview_destroy(VectorNumericPreview *preview) {
    if (preview->documents) {
        /* Only this sandbox's commands may be destroyed; source history is never copied here. */
        vector_history_destroy(&preview->documents->items[0].history);
        free(preview->documents);
    }
    *preview = (VectorNumericPreview){0};
}

void vector_numeric_preview_sync(VectorNumericPreview *preview, const VectorInspector *inspector,
                                 VectorDocument *source, PUIView *view) {
    const char *field, *action;
    if (!source || !pui_view_numeric_preview(view, &field, &action) ||
        (strncmp(action, "vector.rectangle.", 17) && strncmp(action, "vector.artboard.", 16) &&
         strncmp(action, "vector.ellipse.", 15) && strncmp(action, "vector.polygon.", 15) &&
         strncmp(action, "vector.text.", 12) && strncmp(action, "vector.shadow.", 14) &&
         strncmp(action, "vector.gradient.", 16))) {
        vector_numeric_preview_destroy(preview);
        return;
    }
    const char *text = pui_view_text(view, field);
    if (!text) {
        vector_numeric_preview_destroy(preview);
        return;
    }
    if (preview->documents && preview->source_id == source->id &&
        preview->object_id == source->active_rectangle &&
        preview->artboard_id == source->active_artboard &&
        preview->cursor == source->history.cursor && !strcmp(preview->field, field) &&
        !strcmp(preview->text, text)) {
        return;
    }
    if (!preview->documents) {
        preview->documents = calloc(1, sizeof *preview->documents);
    }
    if (!preview->documents) {
        return;
    }
    VectorDocument *candidate = &preview->documents->items[0];
    vector_history_destroy(&candidate->history);
    *candidate = *source;
    candidate->history = (VectorHistory){0};
    preview->documents->count = 1;
    /* Inspector bindings are borrowed. Their dirty flags and selection cache stay local. */
    VectorInspector probe = *inspector;
    char status[256];
    const char *current_status = pui_view_text(view, "status");
    snprintf(status, sizeof status, "%s", current_status ? current_status : "");
    preview->valid = vector_inspector_action(&probe, preview->documents, view, action);
    vector_history_destroy(&candidate->history);
    pui_view_set_text(view, "status", status);
    preview->source_id = source->id;
    preview->object_id = source->active_rectangle;
    preview->artboard_id = source->active_artboard;
    preview->cursor = source->history.cursor;
    snprintf(preview->field, sizeof preview->field, "%s", field);
    snprintf(preview->text, sizeof preview->text, "%s", text);
}

VectorDocument *vector_numeric_preview_document(VectorNumericPreview *preview,
                                                VectorDocument *source) {
    return source && preview->documents && preview->valid && preview->source_id == source->id
               ? &preview->documents->items[0]
               : source;
}
