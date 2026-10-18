/** @file @brief Tool highlights stay active until completion or cancellation. */
#include "toolbar.h"

void vector_toolbar_sync(PUIView *view, const VectorDocument *document) {
    pui_view_set_checked(view, "vector_add_artboard", document && document->artboard_tool.enabled);
    pui_view_set_checked(view, "vector_selection", document && !document->artboard_tool.enabled);
}
