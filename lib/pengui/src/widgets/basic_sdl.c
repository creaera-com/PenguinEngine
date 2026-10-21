/** @file @brief Small widgets: labels, separators and collapsible sections. */
#include "view/internal.h"
#include <math.h>
#include <string.h>

void pui_widget_basic_draw(PUIView *v, PUIContext *pui, int i, PUIRect area, PUIAction action,
                           void *context) {
    (void)pui;
    (void)action;
    (void)context;
    const PUINode *n = &v->layout->nodes[i];
    Widget *s = &v->state[i];
    if (n->type == PUI_NODE_LABEL) {
        pui_text(v->renderer, area.x + 4, area.y + fmaxf(0, (area.h - 16) / 2), s->text, PUI_TEXT);
    } else if (n->type == PUI_NODE_SEPARATOR) {
        pui_panel(v->renderer,
                  (SDL_FRect){area.x + area.w / 2, area.y + 4, 1, fmaxf(0, area.h - 8)},
                  PUI_BORDER);
    }
}

/* A section owns its disclosure header; child placement stays with layout. */
void pui_widget_section_draw(PUIView *v, PUIContext *pui, int i, PUIRect area, PUIAction action,
                             void *context) {
    const PUINode *n = &v->layout->nodes[i];
    Widget *s = &v->state[i];
    PUIRect header = {area.x, area.y, area.w, 28};
    pui_disclosure(pui, n->id, header, &s->open, !pui->blocked);
    Uint32 foreground = pui->blocked               ? PUI_DISABLED
                        : pui_hovered(pui, header) ? PUI_TEXT
                                                   : PUI_MUTED;
    pui_icon(v->renderer, s->open ? ICON_DOWN : ICON_RIGHT, header.x + 9, header.y + 6, 16,
             foreground);
    pui_text(v->renderer, header.x + 31, header.y + 6, n->text, foreground);
    pui_panel(v->renderer, (SDL_FRect){header.x, header.y + header.h - 1, header.w, 1},
              PUI_SURFACE);
    area.y += 30;
    area.h -= 30;
    area.x += n->padding;
    area.y += n->padding;
    area.w = fmaxf(0, area.w - 2 * n->padding);
    area.h = fmaxf(0, area.h - 2 * n->padding);
    if (s->open) {
        pui_view_draw_children(v, pui, i, area, false, action, context);
    }
}
