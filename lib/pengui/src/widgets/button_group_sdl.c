/** @file @brief Equal-size exclusive button groups in either orientation. */
#include "view/internal.h"
#include <math.h>

static PUIRect member_rect(PUIRect area, PUIAxis axis, int count, int index, float gap) {
    float length = axis == PUI_AXIS_X ? area.w : area.h;
    gap = fminf(gap, length / fmaxf(1, count - 1));
    float size = fmaxf(0, length - gap * (count - 1)) / count;
    float offset = index * (size + gap);
    return (PUIRect){area.x + (axis == PUI_AXIS_X ? offset : 0),
                     area.y + (axis == PUI_AXIS_Y ? offset : 0), axis == PUI_AXIS_X ? size : area.w,
                     axis == PUI_AXIS_Y ? size : area.h};
}

void pui_widget_button_group_draw(PUIView *v, PUIContext *pui, int i, PUIRect area,
                                  PUIAction action, void *context) {
    const PUINode *n = &v->layout->nodes[i];
    Widget *s = &v->state[i];
    area.x += n->padding;
    area.y += n->padding;
    area.w = fmaxf(0, area.w - 2 * n->padding);
    area.h = fmaxf(0, area.h - 2 * n->padding);
    int count = 0;
    for (int k = n->child; k >= 0; k = v->layout->nodes[k].next) {
        count++;
    }
    if (!count) {
        return;
    }
    PUIId previous = s->selected;
    int index = 0;
    for (int k = n->child; k >= 0; k = v->layout->nodes[k].next) {
        const PUINode *child = &v->layout->nodes[k];
        pui_radio(pui, child->id, member_rect(area, n->axis, count, index++, n->gap), &s->selected,
                  !pui->blocked && child->enabled);
    }
    /* Update every member before painting or dispatching the selected button's action. */
    for (int k = n->child; k >= 0; k = v->layout->nodes[k].next) {
        v->state[k].checked = v->layout->nodes[k].id == s->selected;
    }
    index = 0;
    int changed = -1;
    for (int k = n->child; k >= 0; k = v->layout->nodes[k].next) {
        pui_view_draw_node(v, pui, k, member_rect(area, n->axis, count, index++, n->gap), action,
                           context);
        if (previous != s->selected && v->state[k].checked) {
            changed = k;
        }
    }
    if (changed >= 0 && action && *v->layout->nodes[changed].action) {
        const PUINode *child = &v->layout->nodes[changed];
        action(context, child->action, child->name);
    }
}

void pui_widget_button_group_init(PUIView *view, int i) {
    const PUILayout *layout = view->layout;
    int selected = -1;
    for (int k = layout->nodes[i].child; k >= 0; k = layout->nodes[k].next) {
        view->state[k].group = i + 1;
        if (layout->nodes[k].checked) {
            selected = k;
        }
    }
    if (selected < 0) {
        for (int k = layout->nodes[i].child; k >= 0; k = layout->nodes[k].next) {
            if (layout->nodes[k].enabled) {
                selected = k;
                break;
            }
        }
    }
    if (selected >= 0) {
        view->state[i].selected = layout->nodes[selected].id;
        view->state[selected].checked = true;
    }
}
