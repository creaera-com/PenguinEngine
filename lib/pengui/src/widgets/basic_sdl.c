/** @file @brief Small widgets: labels, separators and collapsible sections. */
#include "view/internal.h"
#include <math.h>
#include <string.h>

void ui_widget_basic_draw(UiView *v, UiContext *ui, int i, UiRect area, UiAction action,
                          void *context) {
    (void)ui;
    (void)action;
    (void)context;
    const UiNode *n = &v->layout->nodes[i];
    Widget *s = &v->state[i];
    if (n->type == UI_NODE_LABEL) {
        ui_text(v->renderer, area.x + 4, area.y + fmaxf(0, (area.h - 16) / 2), s->text, UI_TEXT);
    } else if (n->type == UI_NODE_SEPARATOR) {
        ui_panel(v->renderer, (SDL_FRect){area.x + area.w / 2, area.y + 4, 1, fmaxf(0, area.h - 8)},
                 UI_BORDER);
    }
}

/* A section owns its disclosure header; child placement stays with layout. */
void ui_widget_section_draw(UiView *v, UiContext *ui, int i, UiRect area, UiAction action,
                            void *context) {
    const UiNode *n = &v->layout->nodes[i];
    Widget *s = &v->state[i];
    ui_panel(v->renderer, ui_frect(area), UI_SURFACE);
    UiRect header = {area.x, area.y, area.w, 28};
    ui_disclosure(ui, n->id, header, &s->open, !ui->blocked);
    ui_draw_button(v->renderer, ui_frect(header), s->open ? ICON_DOWN : ICON_RIGHT, n->text, false,
                   !ui->blocked, ui_hovered(ui, header));
    area.y += 30;
    area.h -= 30;
    area.x += n->padding;
    area.y += n->padding;
    area.w = fmaxf(0, area.w - 2 * n->padding);
    area.h = fmaxf(0, area.h - 2 * n->padding);
    if (s->open) {
        ui_view_draw_children(v, ui, i, area, false, action, context);
    }
}
