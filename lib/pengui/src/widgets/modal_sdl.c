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

static bool paint(PUIView *v, PUIContext *input, int index, PUIRect rect, PUIAction action,
                  void *context) {
    const PUINode *n = &v->layout->nodes[index];
    SDL_BlendMode blend;
    SDL_GetRenderDrawBlendMode(v->renderer, &blend);
    SDL_SetRenderDrawBlendMode(v->renderer, SDL_BLENDMODE_BLEND);
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
    pui_view_draw_node(
        v, input, n->child,
        (PUIRect){rect.x + 1, rect.y + header, fmaxf(0, rect.w - 2), fmaxf(0, rect.h - header - 1)},
        action, context);
    if (clicked) {
        pui_view_popup_close(v, input);
    }
    return !clicked;
}

/* Render the complete dialog into one layer so text, images and fields fade together.
 * RGBA blending into a transparent target produces premultiplied color values. */
static Uint8 opacity(PUIView *v, int index, bool open) {
    Widget *s = &v->state[index];
    const PUINode *n = &v->layout->nodes[index];
    float duration = n->transition_ms >= 0 ? n->transition_ms : pui_theme.button_transition_ms;
    if (!s->modal_opacity.initialized) {
        pui_color_transition(&s->modal_opacity, 0, v->frame_ms, 0);
    }
    return (Uint8)pui_color_transition(&s->modal_opacity, open ? 255 : 0, v->frame_ms, duration);
}

static void composite(PUIView *v, SDL_Texture *layer, Uint8 alpha) {
    Uint8 red, green, blue, a;
    SDL_GetRenderDrawColor(v->renderer, &red, &green, &blue, &a);
    SDL_BlendMode previous;
    SDL_GetRenderDrawBlendMode(v->renderer, &previous);
    SDL_SetRenderDrawBlendMode(v->renderer, SDL_BLENDMODE_BLEND);
    pui_panel(v->renderer, pui_frect(v->bounds), (uint32_t)(144 * alpha / 255));
    SDL_SetRenderDrawBlendMode(v->renderer, previous);
    SDL_SetRenderDrawColor(v->renderer, red, green, blue, a);
    SDL_SetTextureColorMod(layer, alpha, alpha, alpha);
    SDL_SetTextureAlphaMod(layer, alpha);
    float width, height;
    SDL_GetTextureSize(layer, &width, &height);
    SDL_RenderTexture(v->renderer, layer, NULL, &(SDL_FRect){0, 0, width, height});
}

bool pui_modal_draw(PUIView *v, PUIContext *input, int index, PUIRect rect, PUIAction action,
                    void *context) {
    SDL_Renderer *r = v->renderer;
    Widget *s = &v->state[index];
    SDL_Texture *target = SDL_GetRenderTarget(r);
    SDL_Rect viewport, clip;
    bool viewport_set = SDL_RenderViewportSet(r), clipped = SDL_RenderClipEnabled(r);
    SDL_GetRenderViewport(r, &viewport);
    SDL_GetRenderClipRect(r, &clip);
    float sx, sy;
    SDL_GetRenderScale(r, &sx, &sy);
    Uint8 red, green, blue, alpha;
    SDL_GetRenderDrawColor(r, &red, &green, &blue, &alpha);
    SDL_BlendMode blend;
    SDL_GetRenderDrawBlendMode(r, &blend);
    int width = (int)ceilf(v->bounds.x + v->bounds.w);
    int height = (int)ceilf(v->bounds.y + v->bounds.h);
    float old_width = 0, old_height = 0;
    if (s->modal_layer) {
        SDL_GetTextureSize(s->modal_layer, &old_width, &old_height);
    }
    if (width != old_width || height != old_height) {
        SDL_DestroyTexture(s->modal_layer);
        s->modal_layer = NULL;
    }
    bool supported = sx == 1 && sy == 1 && viewport.x == 0 && viewport.y == 0 && width > 0 &&
                     height > 0 && (double)width * height <= 4194304;
    if (supported && !s->modal_layer) {
        s->modal_layer =
            SDL_CreateTexture(r, SDL_PIXELFORMAT_RGBA8888, SDL_TEXTUREACCESS_TARGET, width, height);
        if (s->modal_layer &&
            !SDL_SetTextureBlendMode(s->modal_layer, SDL_BLENDMODE_BLEND_PREMULTIPLIED)) {
            SDL_DestroyTexture(s->modal_layer);
            s->modal_layer = NULL;
        }
    }
    if (!supported || !s->modal_layer || !SDL_SetRenderTarget(r, s->modal_layer)) {
        SDL_SetRenderDrawBlendMode(r, SDL_BLENDMODE_BLEND);
        pui_panel(r, pui_frect(v->bounds), 0x00000090);
        bool result = paint(v, input, index, rect, action, context);
        SDL_SetRenderDrawBlendMode(r, blend);
        return result;
    }
    SDL_SetRenderScale(r, 1, 1);
    SDL_SetRenderViewport(r, NULL);
    SDL_SetRenderClipRect(r, NULL);
    SDL_SetRenderDrawColor(r, 0, 0, 0, 0);
    SDL_RenderClear(r);
    pui_apply_clip(r, input);
    bool result = paint(v, input, index, rect, action, context);
    SDL_SetRenderTarget(r, target);
    SDL_SetRenderScale(r, sx, sy);
    SDL_SetRenderViewport(r, viewport_set ? &viewport : NULL);
    SDL_SetRenderClipRect(r, clipped ? &clip : NULL);
    composite(v, s->modal_layer, opacity(v, index, true));
    s->modal_drawn_ms = v->frame_ms;
    SDL_SetRenderDrawColor(r, red, green, blue, alpha);
    SDL_SetRenderDrawBlendMode(r, blend);
    return result;
}

void pui_modal_draw_closing(PUIView *v) {
    for (int i = 0; i < v->layout->count; i++) {
        Widget *s = &v->state[i];
        if (!s->modal_layer) {
            continue;
        }
        bool open = false;
        for (int j = 0; j < v->popup_count; j++) {
            open |= v->popups[j] == i;
        }
        if (open || s->modal_drawn_ms == v->frame_ms) {
            continue;
        }
        Uint8 alpha = opacity(v, i, false);
        if (alpha) {
            composite(v, s->modal_layer, alpha);
        } else {
            SDL_DestroyTexture(s->modal_layer);
            s->modal_layer = NULL;
            s->modal_opacity = (PUIColorTransition){0};
        }
    }
}
