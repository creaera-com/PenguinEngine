/** @file @brief Tab drawing, YAML initialization and wheel input. */
#include "view/internal.h"
#include <math.h>
#include <string.h>

float pui_tab_content_width(PUIIcon icon, const char *label) {
    return pui_text_width(label) + (icon == ICON_NONE ? 18 : 40);
}

static void draw_tab_colors(SDL_Renderer *r, SDL_FRect rect, PUIIcon icon, const char *label,
                            Uint32 background, Uint32 foreground, float radius) {
    pui_rounded_panel(r, rect, background, radius, true);
    if (icon != ICON_NONE) {
        pui_icon(r, icon, rect.x + 9, rect.y + (rect.h - 16) * .5f, 16, foreground);
    }
    pui_text(r, rect.x + (icon == ICON_NONE ? 9 : 31), rect.y + (rect.h - PUI_FONT_HEIGHT) * .5f,
             label, foreground);
}

void pui_draw_tab(SDL_Renderer *r, SDL_FRect rect, PUIIcon icon, const char *label, bool active,
                  bool enabled, bool hover) {
    draw_tab_colors(r, rect, icon, label,
                    active  ? PUI_PANEL
                    : hover ? PUI_HOVER
                            : PUI_SURFACE,
                    enabled ? PUI_TEXT : PUI_DISABLED, pui_theme.tab_radius);
}

void pui_widget_tabs_draw(PUIView *v, PUIContext *pui, int i, PUIRect area, PUIAction action,
                          void *context) {
    const PUINode *n = &v->layout->nodes[i];
    Widget *s = &v->state[i];
    PUIRect bar = {area.x, area.y, area.w, 30};
    pui_tabs_update(pui, &s->tabs, bar, 100);
    for (int k = 0; k < s->tabs.count; k++) {
        const PUINode *tab = NULL;
        int tab_index = -1;
        for (int c = n->child; c >= 0; c = v->layout->nodes[c].next) {
            if (v->layout->nodes[c].id == s->tabs.items[k].id) {
                tab = &v->layout->nodes[c];
                tab_index = c;
                break;
            }
        }
        PUIRect r = pui_tab_rect(&s->tabs, bar, 100, k);
        Widget *state = &v->state[tab_index];
        bool active = s->tabs.selected == tab->id;
        bool enabled = !pui->blocked && tab->enabled;
        bool hover = enabled && pui_hovered(pui, r);
        float duration = tab->transition_ms >= 0 ? tab->transition_ms
                         : n->transition_ms >= 0 ? n->transition_ms
                                                 : pui_theme.button_transition_ms;
        Uint32 background = active ? PUI_PANEL : hover ? PUI_HOVER : PUI_SURFACE;
        Uint32 foreground = enabled ? PUI_TEXT : PUI_DISABLED;
        background =
            pui_color_transition(&state->button_background, background, v->frame_ms, duration);
        foreground =
            pui_color_transition(&state->button_foreground, foreground, v->frame_ms, duration);
        draw_tab_colors(v->renderer, pui_frect(r), pui_icon_from_name(tab->icon), state->text,
                        background, foreground,
                        tab->radius >= 0 ? tab->radius
                        : n->radius >= 0 ? n->radius
                                         : pui_theme.tab_radius);
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
    if (view->layout->nodes[i].type != PUI_NODE_TABS || !view->state[i].tabs.count ||
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
    PUIId selected = 0;
    for (int k = n->child; k >= 0; k = layout->nodes[k].next) {
        const PUINode *tab = &layout->nodes[k];
        if (tab->checked && tab->enabled) {
            selected = tab->id;
        }
        tabs[count++] =
            (PUITab){tab->id, tab->enabled,
                     pui_tab_content_width(pui_icon_from_name(tab->icon), tab->text) + PUI_TAB_GAP};
    }
    pui_tabs_init(&view->state[i].tabs, tabs, count, selected ? selected : count ? tabs[0].id : 0);
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
