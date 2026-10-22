/** @file @brief Reusable centered modal presentation on the shared overlay stack. */
#include "view/internal.h"
#include <math.h>
#include <string.h>

bool pui_view_modal_active(const PUIView *view) {
    if (view) {
        for (int i = 0; i < view->popup_count; i++) {
            if (view->layout->nodes[view->popups[i]].modal) {
                return true;
            }
        }
    }
    return false;
}

bool pui_view_open_modal(PUIView *view, PUIContext *input, const char *id) {
    if (!view || !input || !id) {
        return false;
    }
    for (int i = 0; i < view->layout->count; i++) {
        const PUINode *n = &view->layout->nodes[i];
        if (n->modal && !strcmp(n->name, id) && view->visible[i] && view->enabled[i]) {
            if (view->popup_count >= PUI_LAYOUT_DEPTH) {
                return false;
            }
            pui_view_popup_open(view, input, i);
            return true;
        }
    }
    return false;
}

PUIRect pui_modal_rect(const PUIView *view, int index) {
    const PUINode *n = &view->layout->nodes[index];
    PUIRect target = view->bounds;
    if (*n->modal_target) {
        for (int i = 0; i < view->layout->count; i++) {
            if (view->visible[i] && !strcmp(view->layout->nodes[i].name, n->modal_target)) {
                target = view->hit[i];
                break;
            }
        }
    }
    PUIRect bounds = view->bounds;
    float margin = fminf(12, fminf(bounds.w, bounds.h) / 4);
    float w = fminf(n->popup_width, bounds.w - 2 * margin);
    float h = fminf(n->popup_height, bounds.h - 2 * margin);
    return (PUIRect){fmaxf(bounds.x + margin,
                           fminf(target.x + (target.w - w) / 2, bounds.x + bounds.w - margin - w)),
                     fmaxf(bounds.y + margin,
                           fminf(target.y + (target.h - h) / 2, bounds.y + bounds.h - margin - h)),
                     w, h};
}

bool pui_modal_draw(PUIView *v, PUIContext *input, int index, PUIRect rect, PUIAction action,
                    void *context) {
    const PUINode *n = &v->layout->nodes[index];
    SDL_BlendMode blend;
    SDL_GetRenderDrawBlendMode(v->renderer, &blend);
    SDL_SetRenderDrawBlendMode(v->renderer, SDL_BLENDMODE_BLEND);
    pui_panel(v->renderer, pui_frect(v->bounds), 0x00000070);
    pui_rounded_panel(v->renderer, (SDL_FRect){rect.x + 4, rect.y + 6, rect.w, rect.h}, 0x00000055,
                      8, false);
    pui_rounded_panel(v->renderer, pui_frect(rect), PUI_PANEL, 6, false);
    SDL_SetRenderDrawBlendMode(v->renderer, blend);
    float header = fminf(36, rect.h);
    pui_text(v->renderer, rect.x + 12, rect.y + 10, n->modal_title, PUI_TEXT);
    PUIRect close = {rect.x + rect.w - 32, rect.y + 4, 28, 28};
    PUIId id = n->id ^ 0x6d6f6461u;
    bool clicked = pui_button(input, id, close, true);
    pui_draw_button(v->renderer, pui_frect(close), ICON_CLOSE, NULL, false, true,
                    pui_hovered(input, close));
    if (clicked) {
        pui_view_popup_close(v, input);
        return false;
    }
    pui_view_draw_node(
        v, input, n->child,
        (PUIRect){rect.x + 1, rect.y + header, fmaxf(0, rect.w - 2), fmaxf(0, rect.h - header - 1)},
        action, context);
    return true;
}
