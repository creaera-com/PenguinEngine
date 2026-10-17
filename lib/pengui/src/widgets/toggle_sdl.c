/** @file @brief Persistent on/off buttons and their YAML actions. */
#include "view/internal.h"

void pui_widget_toggle_draw(PUIView *v, PUIContext *pui, int i, PUIRect area, PUIAction action,
                            void *context) {
    const PUINode *n = &v->layout->nodes[i];
    Widget *s = &v->state[i];
    bool changed = pui_toggle(pui, n->id, area, &s->checked, !pui->blocked);
    if (s->image) {
        pui_draw_image_button(v->renderer, pui_frect(area), s->image, n->text, n->image_padding,
                              s->checked, !pui->blocked, pui_hovered(pui, area),
                              pui->active == n->id && pui->down, pui->focus == n->id);
    } else {
        pui_draw_button(v->renderer, pui_frect(area), pui_icon_from_name(n->icon), n->text,
                        s->checked, !pui->blocked, pui_hovered(pui, area));
    }
    if (changed && action && *n->action) {
        action(context, n->action, n->name);
    }
}
