/** @file @brief Value-based dropdown selection, keyboard navigation and scrolling options. */
#include "view/internal.h"
#include <math.h>
#include <string.h>

void pui_widget_dropdown_init(PUIView *view, int i) {
    const PUINode *n = &view->layout->nodes[i];
    for (int k = n->child; k >= 0; k = view->layout->nodes[k].next) {
        const PUINode *option = &view->layout->nodes[k];
        if (option->enabled && (!*n->value || !strcmp(n->value, option->value))) {
            view->state[i].selected = option->id;
            view->state[i].highlighted = k;
            break;
        }
    }
}

const char *pui_view_value(const PUIView *view, const char *id) {
    if (!view || !id) {
        return NULL;
    }
    for (int i = 0; i < view->layout->count; i++) {
        const PUINode *n = &view->layout->nodes[i];
        if (n->type != PUI_NODE_DROPDOWN || strcmp(n->name, id)) {
            continue;
        }
        for (int k = n->child; k >= 0; k = view->layout->nodes[k].next) {
            if (view->layout->nodes[k].id == view->state[i].selected) {
                return view->layout->nodes[k].value;
            }
        }
    }
    return NULL;
}

bool pui_view_set_value(PUIView *view, const char *id, const char *value) {
    if (!view || !id || !value) {
        return false;
    }
    for (int i = 0; i < view->layout->count; i++) {
        const PUINode *n = &view->layout->nodes[i];
        if (n->type != PUI_NODE_DROPDOWN || strcmp(n->name, id)) {
            continue;
        }
        for (int k = n->child; k >= 0; k = view->layout->nodes[k].next) {
            const PUINode *option = &view->layout->nodes[k];
            if (option->enabled && !strcmp(option->value, value)) {
                view->state[i].selected = option->id;
                view->state[i].highlighted = k;
                view->state[i].reveal = true;
                return true;
            }
        }
    }
    return false;
}

void pui_widget_dropdown_draw(PUIView *v, PUIContext *pui, int i, PUIRect area, PUIAction action,
                              void *context) {
    (void)action;
    (void)context;
    const PUINode *n = &v->layout->nodes[i];
    const char *label = n->text;
    for (int k = n->child; k >= 0; k = v->layout->nodes[k].next) {
        if (v->layout->nodes[k].id == v->state[i].selected) {
            label = v->layout->nodes[k].text;
        }
    }
    if (pui_button(pui, n->id, area, !pui->blocked)) {
        pui_view_popup_open(v, pui, i);
    }
    PUIRect face = area;
    face.w = fmaxf(0, face.w - 24);
    pui_draw_button(v->renderer, pui_frect(area), ICON_NONE, NULL, false, !pui->blocked,
                    pui_hovered(pui, area));
    pui_clip_push(pui, face);
    pui_apply_clip(v->renderer, pui);
    pui_text(v->renderer, area.x + 9, area.y + (area.h - PUI_FONT_HEIGHT) / 2, label,
             pui->blocked ? PUI_DISABLED : PUI_TEXT);
    pui_clip_pop(pui);
    pui_apply_clip(v->renderer, pui);
    pui_icon(v->renderer, ICON_DOWN, area.x + area.w - 22, area.y + (area.h - 16) / 2, 16,
             pui->blocked ? PUI_DISABLED : PUI_TEXT);
}

static void reveal_option(PUIView *view, int host, int option) {
    int row = 0;
    for (int k = view->layout->nodes[host].child; k >= 0; k = view->layout->nodes[k].next) {
        if (k == option) {
            PUIScroll *s = &view->state[host].scroll;
            if (row * 28 < s->offset) {
                s->offset = row * 28;
            } else if ((row + 1) * 28 > s->offset + s->viewport) {
                s->offset = (row + 1) * 28 - s->viewport;
            }
            pui_scroll_clamp(s);
            return;
        }
        row++;
    }
}

bool pui_widget_dropdown_key(PUIView *view, const SDL_KeyboardEvent *key) {
    int host = view->popups[view->popup_count - 1];
    Widget *s = &view->state[host];
    if (key->key == SDLK_RETURN || key->key == SDLK_KP_ENTER || key->key == SDLK_SPACE) {
        if (!key->repeat) {
            s->commit = true;
        }
        return true;
    }
    int options[PUI_LAYOUT_NODES], count = 0, current = 0;
    for (int k = view->layout->nodes[host].child; k >= 0; k = view->layout->nodes[k].next) {
        if (view->layout->nodes[k].enabled) {
            if (k == s->highlighted) {
                current = count;
            }
            options[count++] = k;
        }
    }
    if (!count) {
        return true;
    }
    switch (key->key) {
    case SDLK_UP:
        current = (current + count - 1) % count;
        break;
    case SDLK_DOWN:
        current = (current + 1) % count;
        break;
    case SDLK_HOME:
        current = 0;
        break;
    case SDLK_END:
        current = count - 1;
        break;
    default:
        return false;
    }
    s->keyboard_navigation = true;
    s->highlighted = options[current];
    reveal_option(view, host, s->highlighted);
    return true;
}

void pui_widget_dropdown_list(PUIView *v, PUIContext *pui, int i, PUIRect area, PUIAction action,
                              void *context) {
    const PUINode *n = &v->layout->nodes[i];
    Widget *s = &v->state[i];
    pui_panel(v->renderer, pui_frect(area), PUI_PANEL);
    area.x += 4;
    area.y += 4;
    area.w = fmaxf(0, area.w - 8);
    area.h = fmaxf(0, area.h - 8);
    int count = 0;
    for (int k = n->child; k >= 0; k = v->layout->nodes[k].next) {
        count++;
    }
    s->scroll.content = count * 28.f;
    s->scroll.viewport = area.h;
    if (s->reveal) {
        reveal_option(v, i, s->highlighted);
        s->reveal = false;
    }
    pui_scroll_clamp(&s->scroll);
    bool scrollable = pui_scroll_max(&s->scroll) > 0;
    PUIRect track = {area.x + area.w - 8, area.y, 8, area.h};
    PUIId scroll_id = n->id ^ 0x71000000u;
    pui_clip_push(pui, area);
    pui_apply_clip(v->renderer, pui);
    if (scrollable) {
        pui_scrollbar(pui, scroll_id, track, &s->scroll, PUI_AXIS_Y);
    }
    int choice = s->commit ? s->highlighted : -1;
    s->commit = false;
    int row = 0;
    for (int k = n->child; k >= 0; k = v->layout->nodes[k].next, row++) {
        const PUINode *option = &v->layout->nodes[k];
        PUIRect rect = {area.x, area.y + row * 28 - s->scroll.offset,
                        fmaxf(0, area.w - (scrollable ? 12 : 0)), 28};
        bool hover = option->enabled && pui_hovered(pui, rect);
        if (hover && !s->keyboard_navigation) {
            s->highlighted = k;
        }
        if (pui_button(pui, option->id, rect, option->enabled)) {
            choice = k;
        }
        pui_draw_button(v->renderer, pui_frect(rect),
                        s->selected == option->id ? ICON_CHECK : ICON_NONE, option->text,
                        s->selected == option->id, option->enabled, hover || s->highlighted == k);
    }
    if (scrollable) {
        pui_draw_scrollbar(v->renderer, pui, scroll_id, track, &s->scroll, PUI_AXIS_Y);
    }
    pui_clip_pop(pui);
    pui_apply_clip(v->renderer, pui);
    if (choice >= 0 && v->layout->nodes[choice].enabled) {
        bool changed = s->selected != v->layout->nodes[choice].id;
        s->selected = v->layout->nodes[choice].id;
        pui_view_popup_close(v, pui);
        if (changed && action && *n->action) {
            action(context, n->action, n->name);
        }
    }
}
