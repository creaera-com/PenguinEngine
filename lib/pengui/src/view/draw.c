/** @file @brief Clipping, widget dispatch and frame orchestration only. */
#include "view/internal.h"
#include <math.h>
#include <string.h>

void ui_view_draw_node(UiView *v, UiContext *ui, int i, UiRect area, UiAction action,
                       void *context) {
    if (area.w <= 0 || area.h <= 0) {
        return;
    }
    const UiNode *n = &v->layout->nodes[i];
    bool blocked = ui->blocked;
    ui->blocked = blocked || !n->enabled;
    ui_clip_push(ui, area);
    ui_apply_clip(v->renderer, ui);
    v->rect[i] = area;
    v->hit[i] = ui->clips[ui->clip_depth - 1];
    v->visible[i] = v->hit[i].w > 0 && v->hit[i].h > 0;
    v->enabled[i] = !ui->blocked;
    switch (n->type) {
    case UI_NODE_COLUMN:
    case UI_NODE_ROW:
    case UI_NODE_PANEL:
    case UI_NODE_TOOLBAR:
    case UI_NODE_TAB:
        ui_view_draw_container(v, ui, i, area, action, context);
        break;
    case UI_NODE_SECTION:
        ui_widget_section_draw(v, ui, i, area, action, context);
        break;
    case UI_NODE_SPLIT:
        ui_widget_splitter_draw(v, ui, i, area, action, context);
        break;
    case UI_NODE_TABS:
        ui_widget_tabs_draw(v, ui, i, area, action, context);
        break;
    case UI_NODE_BUTTON:
        ui_widget_button_draw(v, ui, i, area, action, context);
        break;
    case UI_NODE_INPUT:
        ui_widget_text_input_draw(v, ui, i, area, action, context);
        break;
    case UI_NODE_TREE:
        ui_widget_tree_draw(v, ui, i, area, action, context);
        break;
    default:
        ui_widget_basic_draw(v, ui, i, area, action, context);
        break;
    }
    ui->blocked = blocked;
    ui_clip_pop(ui);
    ui_apply_clip(v->renderer, ui);
}

void ui_view_draw(UiView *view, UiContext *ui, UiRect area, UiAction action, void *context) {
    memset(view->visible, 0, sizeof view->visible);
    ui_view_draw_node(view, ui, view->layout->root, area, action, context);
    if (view->focused >= 0 && (!view->visible[view->focused] || !view->enabled[view->focused])) {
        ui_view_blur(view, ui);
    }
    ui_end_frame(ui);
}
