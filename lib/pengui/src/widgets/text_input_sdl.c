/** @file @brief Text-field drawing, focus, clipboard and native input events. */
#include "view/internal.h"
#include <math.h>
#include <string.h>

void ui_draw_text_input(SDL_Renderer *r, SDL_FRect rect, const char *text, UiTextEdit *edit,
                        bool focused, bool enabled, bool invalid, bool cursor_visible) {
    ui_panel(r, rect, UI_CANVAS);
    Uint32 border = invalid ? 0xc97565ffu : focused ? UI_ACCENT : UI_BORDER;
    ui_render_color(r, border);
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
    float cursor = (float)ui_text_edit_column(text, edit->cursor) * UI_FONT_WIDTH;
    float available = fmaxf(0, rect.w - 14);
    if (focused) {
        edit->scroll = fmaxf(0, fminf(edit->scroll, cursor));
        if (cursor - edit->scroll > available) {
            edit->scroll = cursor - available;
        }
    } else {
        edit->scroll = 0;
    }
    float x = rect.x + 7 - edit->scroll, y = rect.y + (rect.h - UI_FONT_HEIGHT) * .5f;
    if (focused && edit->cursor != edit->anchor) {
        float anchor = (float)ui_text_edit_column(text, edit->anchor) * UI_FONT_WIDTH;
        ui_panel(r,
                 (SDL_FRect){x + fminf(cursor, anchor), y, fabsf(cursor - anchor), UI_FONT_HEIGHT},
                 UI_HOVER);
    }
    ui_text(r, x, y, text, enabled ? UI_TEXT : UI_DISABLED);
    if (focused && cursor_visible) {
        ui_render_color(r, UI_ACCENT);
        SDL_RenderLine(r, x + cursor, rect.y + 4, x + cursor, rect.y + rect.h - 4);
    }
    SDL_SetRenderClipRect(r, clipped ? &previous : NULL);
}

void ui_widget_text_input_draw(UiView *v, UiContext *ui, int i, UiRect area, UiAction action,
                               void *context) {
    const UiNode *n = &v->layout->nodes[i];
    Widget *s = &v->state[i];
    ui_draw_text_input(v->renderer, ui_frect(area), s->text, &s->edit, v->focused == i,
                       !ui->blocked, false, SDL_GetTicks() % 1000 < 500);
    if (v->submit == i) {
        if (action && *n->action) {
            action(context, n->action, n->name);
        }
        v->submit = -1;
    }
}

void ui_view_blur(UiView *view, UiContext *ui) {
    if (view->focused >= 0) {
        if (view->window) {
            SDL_StopTextInput(view->window);
        }
        view->focused = -1;
        ui_focus(ui, 0);
    }
}

static bool text_key(UiView *view, UiContext *ui, const SDL_KeyboardEvent *key) {
    Widget *state = &view->state[view->focused];
    bool ctrl = (key->mod & (SDL_KMOD_CTRL | SDL_KMOD_GUI)) != 0;
    bool shift = (key->mod & SDL_KMOD_SHIFT) != 0;
    UiTextKey action;
    switch (key->key) {
    case SDLK_LEFT:
        action = ctrl ? UI_TEXT_HOME : UI_TEXT_LEFT;
        break;
    case SDLK_RIGHT:
        action = ctrl ? UI_TEXT_END : UI_TEXT_RIGHT;
        break;
    case SDLK_HOME:
        action = UI_TEXT_HOME;
        break;
    case SDLK_END:
        action = UI_TEXT_END;
        break;
    case SDLK_BACKSPACE:
        action = UI_TEXT_BACKSPACE;
        break;
    case SDLK_DELETE:
        action = UI_TEXT_DELETE;
        break;
    case SDLK_A:
        if (!ctrl) {
            return false;
        }
        action = UI_TEXT_SELECT_ALL;
        break;
    case SDLK_ESCAPE:
        ui_view_blur(view, ui);
        return true;
    case SDLK_RETURN:
    case SDLK_KP_ENTER:
        view->submit = view->focused;
        return true;
    case SDLK_V:
        if (ctrl) {
            char *text = SDL_GetClipboardText();
            if (text) {
                ui_text_edit_insert(&state->edit, state->text, sizeof state->text, text);
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
                    ui_text_edit_key(&state->edit, state->text, UI_TEXT_DELETE, false);
                }
            }
            return true;
        }
        return false;
    default:
        return false;
    }
    ui_text_edit_key(&state->edit, state->text, action, shift);
    return true;
}

void ui_widget_text_input_pointer(UiView *view, UiContext *ui, SDL_Window *window,
                                  const SDL_Event *event) {

    int input = -1;
    for (int i = 0; i < view->layout->count; i++) {
        if (view->visible[i] && view->enabled[i] && view->layout->nodes[i].type == UI_NODE_INPUT &&
            ui_contains(view->hit[i], event->button.x, event->button.y)) {
            input = i;
        }
    }
    if (input != view->focused) {
        ui_view_blur(view, ui);
    }
    if (input >= 0) {
        view->focused = input;
        ui_focus(ui, view->layout->nodes[input].id);
        SDL_StartTextInput(window);
        float column =
            (event->button.x - view->rect[input].x - 7 + view->state[input].edit.scroll) /
            UI_FONT_WIDTH;
        ui_text_edit_click(&view->state[input].edit, view->state[input].text,
                           (size_t)fmaxf(0, floorf(column + .5f)),
                           (SDL_GetModState() & SDL_KMOD_SHIFT) != 0);
    }
}

bool ui_widget_text_input_event(UiView *view, UiContext *ui, const SDL_Event *event) {
    if (view->focused < 0) {
        return false;
    }
    if (event->type == SDL_EVENT_TEXT_INPUT) {
        Widget *state = &view->state[view->focused];
        ui_text_edit_insert(&state->edit, state->text, sizeof state->text, event->text.text);
        return true;
    }
    return event->type == SDL_EVENT_KEY_DOWN && text_key(view, ui, &event->key);
}
