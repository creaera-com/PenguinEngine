/** @file @brief Two/three-pane splitter; preferred fixed sizes survive window resizing. */
#include "view/internal.h"
#include <math.h>

static float length(PUIRect area, PUIAxis axis) {
    return axis == PUI_AXIS_X ? area.w : area.h;
}

static void grip(PUIView *view, PUIContext *pui, PUIRect area, PUIAxis axis, PUIId id) {
    float x = area.x + area.w / 2, y = area.y + area.h / 2;
    bool active = pui_hovered(pui, area) || pui->active == id;
    for (int dot = -1; dot <= 1; dot++) {
        pui_panel(view->renderer,
                  (SDL_FRect){x + (axis == PUI_AXIS_Y ? dot * 6 : 0) - 1,
                              y + (axis == PUI_AXIS_X ? dot * 6 : 0) - 1, 2, 2},
                  active ? PUI_ACCENT : PUI_MUTED);
    }
}

void pui_widget_splitter_draw(PUIView *v, PUIContext *pui, int i, PUIRect area, PUIAction action,
                              void *context) {
    const PUINode *n = &v->layout->nodes[i];
    Widget *state = &v->state[i];
    int children[3], count = 0;
    for (int child = n->child; child >= 0; child = v->layout->nodes[child].next) {
        children[count++] = child;
    }
    float preferred[3] = {state->preferred[0], state->preferred[1], state->preferred[2]};
    float minimum[3] = {n->minimum_first, n->minimum_second, n->minimum_last};
    if (count == 2) {
        preferred[1] = state->preferred[2];
        minimum[1] = n->minimum_last;
    }
    PUIRegions regions =
        pui_split_layout(area, n->axis, count, n->flexible, preferred, minimum, n->gap);
    for (int k = 0; k < count - 1; k++) {
        PUIId id = k == 0 ? n->id : n->id ^ 0x73000000u;
        float first = length(regions.panes[k], n->axis);
        float second = length(regions.panes[k + 1], n->axis);
        float value = first;
        if (pui_drag_value(pui, id, regions.grips[k], &value, fminf(minimum[k], first),
                           first + second - fminf(minimum[k + 1], second), n->axis, 1)) {
            float delta = value - first;
            if (k != n->flexible) {
                preferred[k] = first + delta;
            }
            if (k + 1 != n->flexible) {
                preferred[k + 1] = second - delta;
            }
            state->preferred[0] = preferred[0];
            if (count == 3) {
                state->preferred[1] = preferred[1];
            }
            state->preferred[2] = preferred[count - 1];
            regions =
                pui_split_layout(area, n->axis, count, n->flexible, preferred, minimum, n->gap);
        }
    }
    for (int k = 0; k < count; k++) {
        pui_view_draw_node(v, pui, children[k], regions.panes[k], action, context);
    }
    for (int k = 0; k < count - 1; k++) {
        grip(v, pui, regions.grips[k], n->axis, k == 0 ? n->id : n->id ^ 0x73000000u);
    }
}
