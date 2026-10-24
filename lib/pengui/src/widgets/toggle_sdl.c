/** @file @brief Persistent on/off buttons and their YAML actions. */
#include "view/internal.h"

void pui_widget_toggle_draw(PUIView *v, PUIContext *pui, int i, PUIRect area, PUIAction action,
                            void *context) {
    const PUINode *n = &v->layout->nodes[i];
    Widget *s = &v->state[i];
    bool changed = pui_toggle(pui, n->id, area, &s->checked, !pui->blocked);
    pui_widget_button_paint(v, pui, i, area, s->checked, pui_icon_from_name(n->icon));
    if (changed && action && *n->action) {
        action(context, n->action, n->name);
    }
}
