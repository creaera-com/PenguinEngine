/** @file @brief Tab drawing, YAML initialization and wheel input. */
#include "view/internal.h"
#include <math.h>
#include <string.h>

float ui_tab_content_width(UiIcon icon, const char *label) {
    return ui_text_width(label) + (icon == ICON_NONE ? 18 : 40);
}

void ui_draw_tab(SDL_Renderer *r, SDL_FRect rect, UiIcon icon, const char *label, bool active,
                 bool enabled, bool hover) {
    ui_panel(r, rect, active ? UI_PANEL : hover ? UI_HOVER : UI_SURFACE);
    Uint32 foreground = enabled ? UI_TEXT : UI_DISABLED;
    if (active) {
        ui_panel(r, (SDL_FRect){rect.x, rect.y, rect.w, 2}, enabled ? UI_ACCENT : UI_DISABLED);
    }
    if (icon != ICON_NONE) {
        ui_icon(r, icon, rect.x + 9, rect.y + (rect.h - 16) * .5f, 16, foreground);
    }
    ui_text(r, rect.x + (icon == ICON_NONE ? 9 : 31), rect.y + (rect.h - UI_FONT_HEIGHT) * .5f,
            label, foreground);
}

void ui_widget_tabs_draw(UiView *v, UiContext *ui, int i, UiRect area, UiAction action,
                         void *context) {
    const UiNode *n = &v->layout->nodes[i];
    Widget *s = &v->state[i];
    UiRect bar = {area.x, area.y, area.w, 30};
    ui_tabs_update(ui, &s->tabs, bar, 100);
    for (int k = 0; k < s->tabs.count; k++) {
        const UiNode *tab = NULL;
        for (int c = n->child; c >= 0; c = v->layout->nodes[c].next) {
            if (v->layout->nodes[c].id == s->tabs.items[k].id) {
                tab = &v->layout->nodes[c];
                break;
            }
        }
        UiRect r = ui_tab_rect(&s->tabs, bar, 100, k);
        ui_draw_tab(v->renderer, ui_frect(r), ui_icon_from_name(tab->icon), tab->text,
                    s->tabs.selected == tab->id, !ui->blocked && tab->enabled, ui_hovered(ui, r));
    }
    for (int k = n->child; k >= 0; k = v->layout->nodes[k].next) {
        if (v->layout->nodes[k].id == s->tabs.selected) {
            ui_view_draw_node(v, ui, k,
                              (UiRect){area.x, area.y + 32, area.w, fmaxf(0, area.h - 32)}, action,
                              context);
        }
    }
}

bool ui_widget_tabs_scroll(UiView *view, int i, const SDL_Event *event) {
    if (view->layout->nodes[i].type != UI_NODE_TABS ||
        event->wheel.mouse_y >= view->rect[i].y + 30) {
        return false;
    }
    ui_tabs_scroll(&view->state[i].tabs, view->rect[i].w, 100, -event->wheel.y * 48);
    return true;
}

void ui_widget_tabs_init(UiView *view, int i) {
    const UiLayout *layout = view->layout;
    const UiNode *n = &layout->nodes[i];

    UiTab tabs[UI_MAX_TABS];
    int count = 0;
    for (int k = n->child; k >= 0; k = layout->nodes[k].next) {
        const UiNode *tab = &layout->nodes[k];
        tabs[count++] =
            (UiTab){tab->id, tab->enabled,
                    ui_tab_content_width(ui_icon_from_name(tab->icon), tab->text) + UI_TAB_GAP};
    }
    ui_tabs_init(&view->state[i].tabs, tabs, count, tabs[0].id);
}
