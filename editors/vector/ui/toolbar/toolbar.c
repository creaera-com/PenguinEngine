/** @file @brief Tool highlights stay active until completion or cancellation. */
#include "toolbar.h"

void vector_toolbar_sync(PUIView *view, const VectorDocument *document) {
    pui_view_set_checked(view, "vector_pen", document && document->curve_tool.pen);
    pui_view_set_checked(view, "vector_nodes", document && document->curve_tool.nodes);
    pui_view_set_checked(view, "vector_element_snap", document && document->element_snap_enabled);
    pui_view_set_checked(view, "vector_add_artboard", document && document->artboard_tool.enabled);
    pui_view_set_checked(view, "vector_add_rect",
                         document && document->rectangle_tool.enabled &&
                             !document->rectangle_tool.ellipse);
    pui_view_set_checked(view, "vector_add_ellipse",
                         document && document->rectangle_tool.enabled &&
                             document->rectangle_tool.ellipse);
    pui_view_set_checked(view, "vector_selection",
                         document && !document->curve_tool.pen && !document->curve_tool.nodes &&
                             !document->artboard_tool.enabled && !document->rectangle_tool.enabled);
}
