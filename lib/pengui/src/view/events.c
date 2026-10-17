#include "internal.h"
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

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

bool ui_view_event(UiView *view, UiContext *ui, SDL_Window *window, const SDL_Event *event) {
    view->window = window;
    if (event->type == SDL_EVENT_WINDOW_FOCUS_LOST) {
        ui_cancel(ui);
        ui_view_blur(view, ui);
    } else if (event->type == SDL_EVENT_MOUSE_MOTION) {
        ui_pointer(ui, event->motion.x, event->motion.y, ui->down);
    } else if ((event->type == SDL_EVENT_MOUSE_BUTTON_DOWN ||
                event->type == SDL_EVENT_MOUSE_BUTTON_UP) &&
               event->button.button == SDL_BUTTON_LEFT) {
        ui_pointer(ui, event->button.x, event->button.y,
                   event->type == SDL_EVENT_MOUSE_BUTTON_DOWN);
        if (event->type == SDL_EVENT_MOUSE_BUTTON_DOWN) {
            int input = -1;
            for (int i = 0; i < view->layout->count; i++) {
                if (view->visible[i] && view->enabled[i] &&
                    view->layout->nodes[i].type == UI_NODE_INPUT &&
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
    } else if (event->type == SDL_EVENT_MOUSE_WHEEL) {
        for (int i = view->layout->count - 1; i >= 0; i--) {
            if (!view->visible[i] || !view->enabled[i] ||
                !ui_contains(view->rect[i], event->wheel.mouse_x, event->wheel.mouse_y)) {
                continue;
            }
            if (view->layout->nodes[i].type == UI_NODE_TREE) {
                ui_scroll_by(&view->state[i].scroll, -event->wheel.y * 48);
                return true;
            }
            if (view->layout->nodes[i].type == UI_NODE_TABS &&
                event->wheel.mouse_y < view->rect[i].y + 30) {
                ui_tabs_scroll(&view->state[i].tabs, view->rect[i].w, 100, -event->wheel.y * 48);
                return true;
            }
        }
    } else if (event->type == SDL_EVENT_TEXT_INPUT && view->focused >= 0) {
        Widget *state = &view->state[view->focused];
        ui_text_edit_insert(&state->edit, state->text, sizeof state->text, event->text.text);
        return true;
    } else if (event->type == SDL_EVENT_KEY_DOWN) {
        if (view->focused >= 0 && text_key(view, ui, &event->key)) {
            return true;
        }
        if (!event->key.repeat && (event->key.key == SDLK_RETURN || event->key.key == SDLK_SPACE)) {
            ui_activate(ui);
        }
    }
    return false;
}
