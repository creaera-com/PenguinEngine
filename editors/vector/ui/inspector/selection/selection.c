/** @file @brief Summary of the current multiple-object selection. */
#include "selection.h"
#include "editors/vector/document/selection.h"
#include <stdio.h>

void vector_selection_inspector_sync(VectorDocument *document, PUIView *view) {
    char text[64];
    int paths = 0;
    for (int i = 0; i < document->rectangle_count; i++) {
        paths += (document->rectangles[i].path.count || document->rectangles[i].curve.count) &&
                 vector_document_is_selected(document, document->rectangles[i].id);
    }
    const char *kind = paths == document->selection.count ? "Pfade"
                       : paths                            ? "Objekte"
                                                          : "Rechtecke";
    snprintf(text, sizeof text, "%d %s ausgewaehlt", document->selection.count, kind);
    pui_view_set_text(view, "vector_selection_count", text);
}
