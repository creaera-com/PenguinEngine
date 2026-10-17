/** @file @brief Button that opens an anchored YAML panel. */
#include "view/internal.h"

void pui_widget_popup_button_draw(PUIView *v, PUIContext *pui, int i, PUIRect area,
                                  PUIAction action, void *context) {
    (void)action;
    (void)context;
    const PUINode *n = &v->layout->nodes[i];
    bool open = false;
    for (int k = 0; k < v->popup_count; k++) {
        open |= v->popups[k] == i;
    }
    if (pui_button(pui, n->id, area, !pui->blocked)) {
        pui_view_popup_open(v, pui, i);
        open = true;
    }
    Widget *s = &v->state[i];
    if (s->image) {
        pui_draw_image_button(v->renderer, pui_frect(area), s->image, n->text, n->image_padding,
                              open, !pui->blocked, pui_hovered(pui, area), false,
                              pui->focus == n->id);
    } else {
        pui_draw_button(v->renderer, pui_frect(area), pui_icon_from_name(n->icon), n->text, open,
                        !pui->blocked, pui_hovered(pui, area));
    }
}
