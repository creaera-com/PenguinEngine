/** @file @brief Clipping, widget dispatch and frame orchestration only. */
#include "view/internal.h"
#include <math.h>
#include <string.h>

void pui_view_draw_node(PUIView *v, PUIContext *pui, int i, PUIRect area, PUIAction action,
                        void *context) {
    if (area.w <= 0 || area.h <= 0) {
        return;
    }
    const PUINode *n = &v->layout->nodes[i];
    bool blocked = pui->blocked;
    pui->blocked = blocked || !n->enabled;
    pui_clip_push(pui, area);
    pui_apply_clip(v->renderer, pui);
    v->rect[i] = area;
    v->hit[i] = pui->clips[pui->clip_depth - 1];
    v->visible[i] = v->hit[i].w > 0 && v->hit[i].h > 0;
    v->enabled[i] = !pui->blocked;
    switch (n->type) {
    case PUI_NODE_COLUMN:
    case PUI_NODE_ROW:
    case PUI_NODE_PANEL:
    case PUI_NODE_TOOLBAR:
    case PUI_NODE_TAB:
        pui_view_draw_container(v, pui, i, area, action, context);
        break;
    case PUI_NODE_SECTION:
        pui_widget_section_draw(v, pui, i, area, action, context);
        break;
    case PUI_NODE_SPLIT:
        pui_widget_splitter_draw(v, pui, i, area, action, context);
        break;
    case PUI_NODE_TABS:
        pui_widget_tabs_draw(v, pui, i, area, action, context);
        break;
    case PUI_NODE_TOGGLE:
        pui_widget_toggle_draw(v, pui, i, area, action, context);
        break;
    case PUI_NODE_CHECKBOX:
        pui_widget_checkbox_draw(v, pui, i, area, action, context);
        break;
    case PUI_NODE_BUTTON_GROUP:
        pui_widget_button_group_draw(v, pui, i, area, action, context);
        break;
    case PUI_NODE_BUTTON:
        pui_widget_button_draw(v, pui, i, area, action, context);
        break;
    case PUI_NODE_INPUT:
        pui_widget_text_input_draw(v, pui, i, area, action, context);
        break;
    case PUI_NODE_TREE:
        pui_widget_tree_draw(v, pui, i, area, action, context);
        break;
    default:
        pui_widget_basic_draw(v, pui, i, area, action, context);
        break;
    }
    pui->blocked = blocked;
    pui_clip_pop(pui);
    pui_apply_clip(v->renderer, pui);
}

void pui_view_draw(PUIView *view, PUIContext *pui, PUIRect area, PUIAction action, void *context) {
    memset(view->visible, 0, sizeof view->visible);
    pui_view_draw_node(view, pui, view->layout->root, area, action, context);
    if (view->focused >= 0 && (!view->visible[view->focused] || !view->enabled[view->focused])) {
        pui_view_blur(view, pui);
    }
    pui_end_frame(pui);
}
