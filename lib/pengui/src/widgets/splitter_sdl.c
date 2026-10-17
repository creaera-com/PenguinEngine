/** @file @brief Interactive YAML splitter and grip drawing. */
#include "view/internal.h"
#include <math.h>
#include <string.h>

void ui_widget_splitter_draw(UiView *v, UiContext *ui, int i, UiRect area, UiAction action,
                             void *context) {
    const UiNode *n = &v->layout->nodes[i];
    Widget *s = &v->state[i];
    float total = n->axis == UI_AXIS_X ? area.w : area.h;
    UiSplit split = ui_split_rects(area, n->axis, total - n->gap - s->extent, n->minimum_first,
                                   n->minimum_second, n->gap);
    s->extent = n->axis == UI_AXIS_X ? split.first.w : split.first.h;
    ui_drag_value(ui, n->id, split.grip, &s->extent, fminf(n->minimum_first, s->extent),
                  fmaxf(s->extent, total - n->gap - n->minimum_second), n->axis, 1);
    split = ui_split_rects(area, n->axis, total - n->gap - s->extent, n->minimum_first,
                           n->minimum_second, n->gap);
    ui_view_draw_node(v, ui, n->child, split.first, action, context);
    ui_view_draw_node(v, ui, v->layout->nodes[n->child].next, split.second, action, context);
    float x = split.grip.x + split.grip.w / 2, y = split.grip.y + split.grip.h / 2;
    for (int dot = -1; dot <= 1; dot++) {
        ui_panel(v->renderer,
                 (SDL_FRect){x + (n->axis == UI_AXIS_Y ? dot * 6 : 0) - 1,
                             y + (n->axis == UI_AXIS_X ? dot * 6 : 0) - 1, 2, 2},
                 ui_hovered(ui, split.grip) ? UI_ACCENT : UI_MUTED);
    }
}
