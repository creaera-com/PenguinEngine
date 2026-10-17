/** @file @brief Text-field drawing, focus, clipboard and native input events. */
#include "view/internal.h"
#include <math.h>
#include <string.h>

void pui_draw_text_input(SDL_Renderer *r, SDL_FRect rect, const char *text, PUITextEdit *edit,
                         bool focused, bool enabled, bool invalid, bool cursor_visible) {
    pui_panel(r, rect, PUI_CANVAS);
    Uint32 border = invalid ? 0xc97565ffu : focused ? PUI_ACCENT : PUI_BORDER;
    pui_render_color(r, border);
    SDL_RenderRect(r, &rect);
    SDL_Rect previous;
    bool clipped = SDL_RenderClipEnabled(r);
    SDL_GetRenderClipRect(r, &previous);
    SDL_Rect inner = {(int)ceilf(rect.x + 5), (int)ceilf(rect.y + 2), (int)fmaxf(0, rect.w - 10),
                      (int)fmaxf(0, rect.h - 4)};
    if (clipped) {
        SDL_GetRectIntersection(&inner, &previous, &inner);
    }
    SDL_SetRenderClipRect(r, &inner);
    float cursor = (float)pui_text_edit_column(text, edit->cursor) * PUI_FONT_WIDTH;
    float available = fmaxf(0, rect.w - 14);
    if (focused) {
        edit->scroll = fmaxf(0, fminf(edit->scroll, cursor));
        if (cursor - edit->scroll > available) {
            edit->scroll = cursor - available;
        }
    } else {
        edit->scroll = 0;
    }
    float x = rect.x + 7 - edit->scroll, y = rect.y + (rect.h - PUI_FONT_HEIGHT) * .5f;
    if (focused && edit->cursor != edit->anchor) {
        float anchor = (float)pui_text_edit_column(text, edit->anchor) * PUI_FONT_WIDTH;
        pui_panel(
            r, (SDL_FRect){x + fminf(cursor, anchor), y, fabsf(cursor - anchor), PUI_FONT_HEIGHT},
            PUI_HOVER);
    }
    pui_text(r, x, y, text, enabled ? PUI_TEXT : PUI_DISABLED);
    if (focused && cursor_visible) {
        pui_render_color(r, PUI_ACCENT);
        SDL_RenderLine(r, x + cursor, rect.y + 4, x + cursor, rect.y + rect.h - 4);
    }
    SDL_SetRenderClipRect(r, clipped ? &previous : NULL);
}

void pui_widget_text_input_draw(PUIView *v, PUIContext *pui, int i, PUIRect area, PUIAction action,
                                void *context) {
    const PUINode *n = &v->layout->nodes[i];
    Widget *s = &v->state[i];
    pui_draw_text_input(v->renderer, pui_frect(area), s->text, &s->edit, v->focused == i,
                        !pui->blocked, false, SDL_GetTicks() % 1000 < 500);
    if (v->submit == i) {
        if (action && *n->action) {
            action(context, n->action, n->name);
        }
        v->submit = -1;
    }
}

void pui_view_blur(PUIView *view, PUIContext *pui) {
    if (view->focused >= 0) {
        if (view->window) {
            SDL_StopTextInput(view->window);
        }
        view->focused = -1;
        pui_focus(pui, 0);
    }
}

static bool text_key(PUIView *view, PUIContext *pui, const SDL_KeyboardEvent *key) {
    Widget *state = &view->state[view->focused];
    bool ctrl = (key->mod & (SDL_KMOD_CTRL | SDL_KMOD_GUI)) != 0;
    bool shift = (key->mod & SDL_KMOD_SHIFT) != 0;
    PUITextKey action;
    switch (key->key) {
    case SDLK_LEFT:
        action = ctrl ? PUI_TEXT_HOME : PUI_TEXT_LEFT;
        break;
    case SDLK_RIGHT:
        action = ctrl ? PUI_TEXT_END : PUI_TEXT_RIGHT;
        break;
    case SDLK_HOME:
        action = PUI_TEXT_HOME;
        break;
    case SDLK_END:
        action = PUI_TEXT_END;
        break;
    case SDLK_BACKSPACE:
        action = PUI_TEXT_BACKSPACE;
        break;
    case SDLK_DELETE:
        action = PUI_TEXT_DELETE;
        break;
    case SDLK_A:
        if (!ctrl) {
            return false;
        }
        action = PUI_TEXT_SELECT_ALL;
        break;
    case SDLK_ESCAPE:
        pui_view_blur(view, pui);
        return true;
    case SDLK_RETURN:
    case SDLK_KP_ENTER:
        view->submit = view->focused;
        return true;
    case SDLK_V:
        if (ctrl) {
            char *text = SDL_GetClipboardText();
            if (text) {
                pui_text_edit_insert(&state->edit, state->text, sizeof state->text, text);
                SDL_free(text);
            }
            return true;
        }
        return false;
    case SDLK_C:
    case SDLK_X:
        if (ctrl) {
            size_t a =
                state->edit.cursor < state->edit.anchor ? state->edit.cursor : state->edit.anchor;
            size_t b =
                state->edit.cursor > state->edit.anchor ? state->edit.cursor : state->edit.anchor;
            if (a < b && b <= strlen(state->text)) {
                char text[256];
                memcpy(text, state->text + a, b - a);
                text[b - a] = 0;
                if (SDL_SetClipboardText(text) && key->key == SDLK_X) {
                    pui_text_edit_key(&state->edit, state->text, PUI_TEXT_DELETE, false);
                }
            }
            return true;
        }
        return false;
    default:
        return false;
    }
    pui_text_edit_key(&state->edit, state->text, action, shift);
    return true;
}

void pui_widget_text_input_pointer(PUIView *view, PUIContext *pui, SDL_Window *window,
                                   const SDL_Event *event) {

    int input = -1;
    for (int i = 0; i < view->layout->count; i++) {
        if (view->visible[i] && view->enabled[i] && view->layout->nodes[i].type == PUI_NODE_INPUT &&
            pui_contains(view->hit[i], event->button.x, event->button.y)) {
            input = i;
        }
    }
    if (input != view->focused) {
        pui_view_blur(view, pui);
    }
    if (input >= 0) {
        view->focused = input;
        pui_focus(pui, view->layout->nodes[input].id);
        SDL_StartTextInput(window);
        float column =
            (event->button.x - view->rect[input].x - 7 + view->state[input].edit.scroll) /
            PUI_FONT_WIDTH;
        pui_text_edit_click(&view->state[input].edit, view->state[input].text,
                            (size_t)fmaxf(0, floorf(column + .5f)),
                            (SDL_GetModState() & SDL_KMOD_SHIFT) != 0);
    }
}

/* YAML order, restricted to fields visible and enabled in the last rendered frame. */
static bool focus_next_input(PUIView *view, PUIContext *pui, bool backwards) {
    int count = view->layout->count;
    int start = view->focused >= 0 ? view->focused : backwards ? 0 : count - 1;
    for (int step = 1; step <= count; step++) {
        int i = (start + (backwards ? -step : step) + count) % count;
        if (view->layout->nodes[i].type != PUI_NODE_INPUT || !view->visible[i] ||
            !view->enabled[i]) {
            continue;
        }
        if (i != view->focused) {
            pui_view_blur(view, pui);
            view->focused = i;
            pui_text_edit_init(&view->state[i].edit, view->state[i].text, true);
        }
        pui_focus(pui, view->layout->nodes[i].id);
        if (view->window) {
            SDL_StartTextInput(view->window);
        }
        return true;
    }
    return false;
}

bool pui_widget_text_input_event(PUIView *view, PUIContext *pui, const SDL_Event *event) {
    if (event->type == SDL_EVENT_KEY_DOWN && event->key.key == SDLK_TAB &&
        !(event->key.mod & (SDL_KMOD_CTRL | SDL_KMOD_GUI | SDL_KMOD_ALT))) {
        if (event->key.repeat) {
            return true;
        }
        return focus_next_input(view, pui, (event->key.mod & SDL_KMOD_SHIFT) != 0);
    }
    if (view->focused < 0) {
        return false;
    }
    if (event->type == SDL_EVENT_TEXT_INPUT) {
        Widget *state = &view->state[view->focused];
        pui_text_edit_insert(&state->edit, state->text, sizeof state->text, event->text.text);
        return true;
    }
    return event->type == SDL_EVENT_KEY_DOWN && text_key(view, pui, &event->key);
}
