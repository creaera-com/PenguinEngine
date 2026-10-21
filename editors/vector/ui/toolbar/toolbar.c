/** @file @brief Tool highlights stay active until completion or cancellation. */
#include "toolbar.h"
#include "editors/vector/preview/tool.h"
#include <string.h>

bool vector_toolbar_action(PUIView *view, VectorDocument *document, const char *action) {
    if (strcmp(action, "vector.rulers.toggle")) {
        return false;
    }
    if (document) {
        document->rulers_hidden = !document->rulers_hidden;
    }
    vector_toolbar_sync(view, document);
    return true;
}

void vector_toolbar_sync(PUIView *view, const VectorDocument *document) {
    vector_preview_sync(view, document);
    pui_view_set_checked(view, "vector_rulers_toggle", document && !document->rulers_hidden);
    pui_view_set_checked(view, "vector_gradient", document && document->gradient_tool.enabled);
    pui_view_set_checked(view, "vector_pen", document && document->curve_tool.pen);
    pui_view_set_checked(view, "vector_nodes", document && document->curve_tool.nodes);
    pui_view_set_checked(view, "vector_element_snap", document && document->element_snap_enabled);
    pui_view_set_checked(view, "vector_add_artboard", document && document->artboard_tool.enabled);
    pui_view_set_checked(view, "vector_add_rect",
                         document && document->rectangle_tool.enabled &&
                             !document->rectangle_tool.ellipse &&
                             !document->rectangle_tool.polygon && !document->rectangle_tool.text);
    pui_view_set_checked(view, "vector_add_ellipse",
                         document && document->rectangle_tool.enabled &&
                             document->rectangle_tool.ellipse);
    pui_view_set_checked(view, "vector_add_polygon",
                         document && document->rectangle_tool.enabled &&
                             document->rectangle_tool.polygon);
    pui_view_set_checked(view, "vector_add_text",
                         document && document->rectangle_tool.enabled &&
                             document->rectangle_tool.text);
    pui_view_set_checked(view, "vector_selection",
                         document && !document->gradient_tool.enabled && !document->preview.armed &&
                             !document->curve_tool.pen && !document->curve_tool.nodes &&
                             !document->artboard_tool.enabled && !document->rectangle_tool.enabled);
}
