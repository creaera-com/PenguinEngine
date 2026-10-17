/** @file @brief Interactive YAML splitter and grip drawing. */
#include "view/internal.h"
#include <math.h>
#include <string.h>

void pui_widget_splitter_draw(PUIView *v, PUIContext *pui, int i, PUIRect area, PUIAction action,
                              void *context) {
    const PUINode *n = &v->layout->nodes[i];
    Widget *s = &v->state[i];
    float total = n->axis == PUI_AXIS_X ? area.w : area.h;
    PUISplit split = pui_split_rects(area, n->axis, total - n->gap - s->extent, n->minimum_first,
                                     n->minimum_second, n->gap);
    s->extent = n->axis == PUI_AXIS_X ? split.first.w : split.first.h;
    pui_drag_value(pui, n->id, split.grip, &s->extent, fminf(n->minimum_first, s->extent),
                   fmaxf(s->extent, total - n->gap - n->minimum_second), n->axis, 1);
    split = pui_split_rects(area, n->axis, total - n->gap - s->extent, n->minimum_first,
                            n->minimum_second, n->gap);
    pui_view_draw_node(v, pui, n->child, split.first, action, context);
    pui_view_draw_node(v, pui, v->layout->nodes[n->child].next, split.second, action, context);
    float x = split.grip.x + split.grip.w / 2, y = split.grip.y + split.grip.h / 2;
    for (int dot = -1; dot <= 1; dot++) {
        pui_panel(v->renderer,
                  (SDL_FRect){x + (n->axis == PUI_AXIS_Y ? dot * 6 : 0) - 1,
                              y + (n->axis == PUI_AXIS_X ? dot * 6 : 0) - 1, 2, 2},
                  pui_hovered(pui, split.grip) ? PUI_ACCENT : PUI_MUTED);
    }
}
