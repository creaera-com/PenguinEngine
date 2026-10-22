/** @file @brief Overlay stack, modal input routing and dismissal for anchored popups. */
#include "internal.h"
#include <float.h>
#include <math.h>
#include <string.h>

PUIContext pui_view_inert_context(const PUIContext *pui) {
    PUIContext inert = *pui;
    inert.active = inert.focus = 0;
    inert.down = inert.pressed = inert.released = inert.activate = false;
    inert.x = inert.y = -FLT_MAX;
    return inert;
}

void pui_view_popup_close(PUIView *view, PUIContext *pui) {
    if (!view->popup_count) {
        return;
    }
    int host = view->popups[--view->popup_count];
    view->state[host].commit = false;
    pui_view_blur(view, pui);
    pui_cancel(pui);
    pui_focus(pui, view->layout->nodes[host].id);
}

void pui_view_popup_open(PUIView *view, PUIContext *pui, int index) {
    if (view->popup_count >= PUI_LAYOUT_DEPTH) {
        return;
    }
    for (int k = 0; k < view->popup_count; k++) {
        if (view->popups[k] == index) {
            return;
        }
    }
    pui_view_blur(view, pui);
    pui_cancel(pui);
    pui_focus(pui, view->layout->nodes[index].id);
    view->popups[view->popup_count++] = index;
    view->state[index].highlighted = -1;
    view->state[index].commit = false;
    view->state[index].keyboard_navigation = false;
    view->state[index].reveal = true;
    if (view->layout->nodes[index].type == PUI_NODE_DROPDOWN) {
        for (int k = view->layout->nodes[index].child; k >= 0; k = view->layout->nodes[k].next) {
            if (view->layout->nodes[k].id == view->state[index].selected) {
                view->state[index].highlighted = k;
                break;
            }
        }
    }
}

bool pui_view_popup_prepare(PUIView *view, PUIContext *pui) {
    if (!view->popup_count) {
        return false;
    }
    int top = view->popup_count - 1;
    /* Dismiss on press, consuming the whole click so controls behind cannot fire. */
    if (pui->pressed && !view->layout->nodes[view->popups[top]].modal &&
        !pui_contains(view->popup_rects[top], pui->press_x, pui->press_y)) {
        pui_view_popup_close(view, pui);
    }
    return true;
}

static PUIRect overlay_rect(PUIView *view, int index) {
    const PUINode *n = &view->layout->nodes[index];
    if (n->modal) {
        return pui_modal_rect(view, index);
    }
    float width = n->popup_width > 0             ? n->popup_width
                  : n->type == PUI_NODE_DROPDOWN ? view->rect[index].w
                                                 : 260;
    float height = n->popup_height;
    if (n->type == PUI_NODE_DROPDOWN) {
        int count = 0;
        for (int k = n->child; k >= 0; k = view->layout->nodes[k].next) {
            count++;
        }
        height = fminf(height, count * 28.f + 8);
    }
    return pui_popup_rect(view->bounds, view->hit[index], width, height, n->placement);
}

void pui_view_popup_draw(PUIView *view, PUIContext *pui, PUIAction action, void *context) {
    for (int level = 0; level < view->popup_count; level++) {
        int index = view->popups[level];
        if (!view->visible[index]) {
            while (view->popup_count > level) {
                pui_view_popup_close(view, pui);
            }
            break;
        }
        /* Only the top layer participates in text focus, scrolling and native hit tests. */
        memset(view->enabled, 0, sizeof view->enabled);
        PUIContext inert = pui_view_inert_context(pui);
        PUIContext *input = level == view->popup_count - 1 ? pui : &inert;
        PUIRect rect = overlay_rect(view, index);
        view->popup_rects[level] = rect;
        pui_clip_push(input, view->bounds);
        pui_apply_clip(view->renderer, input);
        const PUINode *n = &view->layout->nodes[index];
        if (n->modal) {
            if (!pui_modal_draw(view, input, index, rect, action, context)) {
                pui_clip_pop(input);
                pui_apply_clip(view->renderer, input);
                break;
            }
        } else if (n->type == PUI_NODE_DROPDOWN) {
            pui_widget_dropdown_list(view, input, index, rect, action, context);
        } else {
            pui_view_draw_node(view, input, n->child, rect, action, context);
        }
        pui_render_color(view->renderer, PUI_BORDER);
        SDL_FRect border = pui_frect(rect);
        SDL_RenderRect(view->renderer, &border);
        pui_clip_pop(input);
        pui_apply_clip(view->renderer, input);
    }
}

bool pui_view_popup_event(PUIView *view, PUIContext *pui, const SDL_Event *event) {
    if (!view->popup_count) {
        return false;
    }
    int top = view->popup_count - 1;
    int host = view->popups[top];
    const PUINode *n = &view->layout->nodes[host];
    if (event->type == SDL_EVENT_KEY_DOWN && event->key.key == SDLK_ESCAPE) {
        if (!event->key.repeat) {
            pui_view_popup_close(view, pui);
        }
        return true;
    }
    if (n->type == PUI_NODE_DROPDOWN) {
        if (event->type == SDL_EVENT_MOUSE_MOTION) {
            view->state[host].keyboard_navigation = false;
        }
        if (event->type == SDL_EVENT_KEY_DOWN && pui_widget_dropdown_key(view, &event->key)) {
            return true;
        }
        if (event->type == SDL_EVENT_MOUSE_WHEEL) {
            pui_scroll_by(&view->state[host].scroll, -event->wheel.y * 28);
            return true;
        }
        if (event->type == SDL_EVENT_TEXT_INPUT || event->type == SDL_EVENT_KEY_DOWN) {
            return true;
        }
    } else if (event->type == SDL_EVENT_MOUSE_WHEEL &&
               !pui_contains(view->popup_rects[top], event->wheel.mouse_x, event->wheel.mouse_y)) {
        return true;
    }
    return false;
}

void pui_view_close_popups(PUIView *view, PUIContext *context) {
    if (!view || !context) {
        return;
    }
    while (view->popup_count) {
        pui_view_popup_close(view, context);
    }
}
