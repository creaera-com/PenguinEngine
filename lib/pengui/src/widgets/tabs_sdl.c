/** @file @brief Tab drawing, YAML initialization and wheel input. */
#include "view/internal.h"
#include <math.h>
#include <string.h>

float pui_tab_content_width(PUIIcon icon, const char *label) {
    return pui_text_width(label) + (icon == ICON_NONE ? 18 : 40);
}

void pui_draw_tab(SDL_Renderer *r, SDL_FRect rect, PUIIcon icon, const char *label, bool active,
                  bool enabled, bool hover) {
    pui_panel(r, rect, active ? PUI_PANEL : hover ? PUI_HOVER : PUI_SURFACE);
    Uint32 foreground = enabled ? PUI_TEXT : PUI_DISABLED;
    if (active) {
        pui_panel(r, (SDL_FRect){rect.x, rect.y, rect.w, 2}, enabled ? PUI_ACCENT : PUI_DISABLED);
    }
    if (icon != ICON_NONE) {
        pui_icon(r, icon, rect.x + 9, rect.y + (rect.h - 16) * .5f, 16, foreground);
    }
    pui_text(r, rect.x + (icon == ICON_NONE ? 9 : 31), rect.y + (rect.h - PUI_FONT_HEIGHT) * .5f,
             label, foreground);
}

void pui_widget_tabs_draw(PUIView *v, PUIContext *pui, int i, PUIRect area, PUIAction action,
                          void *context) {
    const PUINode *n = &v->layout->nodes[i];
    Widget *s = &v->state[i];
    PUIRect bar = {area.x, area.y, area.w, 30};
    pui_tabs_update(pui, &s->tabs, bar, 100);
    for (int k = 0; k < s->tabs.count; k++) {
        const PUINode *tab = NULL;
        for (int c = n->child; c >= 0; c = v->layout->nodes[c].next) {
            if (v->layout->nodes[c].id == s->tabs.items[k].id) {
                tab = &v->layout->nodes[c];
                break;
            }
        }
        PUIRect r = pui_tab_rect(&s->tabs, bar, 100, k);
        pui_draw_tab(v->renderer, pui_frect(r), pui_icon_from_name(tab->icon), tab->text,
                     s->tabs.selected == tab->id, !pui->blocked && tab->enabled,
                     pui_hovered(pui, r));
    }
    for (int k = n->child; k >= 0; k = v->layout->nodes[k].next) {
        if (v->layout->nodes[k].id == s->tabs.selected) {
            pui_view_draw_node(v, pui, k,
                               (PUIRect){area.x, area.y + bar.h, area.w, fmaxf(0, area.h - bar.h)},
                               action, context);
        }
    }
}

bool pui_widget_tabs_scroll(PUIView *view, int i, const SDL_Event *event) {
    if (view->layout->nodes[i].type != PUI_NODE_TABS ||
        event->wheel.mouse_y >= view->rect[i].y + 30) {
        return false;
    }
    pui_tabs_scroll(&view->state[i].tabs, view->rect[i].w, 100, -event->wheel.y * 48);
    return true;
}

void pui_widget_tabs_init(PUIView *view, int i) {
    const PUILayout *layout = view->layout;
    const PUINode *n = &layout->nodes[i];

    PUITab tabs[PUI_MAX_TABS];
    int count = 0;
    for (int k = n->child; k >= 0; k = layout->nodes[k].next) {
        const PUINode *tab = &layout->nodes[k];
        tabs[count++] =
            (PUITab){tab->id, tab->enabled,
                     pui_tab_content_width(pui_icon_from_name(tab->icon), tab->text) + PUI_TAB_GAP};
    }
    pui_tabs_init(&view->state[i].tabs, tabs, count, tabs[0].id);
}

static int tabs_index(const PUIView *view, const char *tabs) {
    if (view && tabs) {
        for (int i = 0; i < view->layout->count; i++) {
            if (view->layout->nodes[i].type == PUI_NODE_TABS &&
                !strcmp(view->layout->nodes[i].name, tabs)) {
                return i;
            }
        }
    }
    return -1;
}

int pui_view_tab_count(const PUIView *view, const char *tabs) {
    int i = tabs_index(view, tabs);
    return i >= 0 ? view->state[i].tabs.count : 0;
}

const char *pui_view_selected_tab(const PUIView *view, const char *tabs) {
    int i = tabs_index(view, tabs);
    if (i >= 0) {
        for (int k = view->layout->nodes[i].child; k >= 0; k = view->layout->nodes[k].next) {
            if (view->layout->nodes[k].id == view->state[i].tabs.selected) {
                return view->layout->nodes[k].name;
            }
        }
    }
    return NULL;
}
