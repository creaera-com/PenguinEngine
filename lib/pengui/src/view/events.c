/** @file @brief Route native input to the responsible widgets. */
#include "internal.h"

bool pui_view_event(PUIView *view, PUIContext *pui, SDL_Window *window, const SDL_Event *event) {
    view->window = window;
    if (event->type == SDL_EVENT_WINDOW_FOCUS_LOST) {
        pui_cancel(pui);
        pui_view_blur(view, pui);
    } else if (event->type == SDL_EVENT_MOUSE_MOTION) {
        pui_pointer(pui, event->motion.x, event->motion.y, pui->down);
    } else if ((event->type == SDL_EVENT_MOUSE_BUTTON_DOWN ||
                event->type == SDL_EVENT_MOUSE_BUTTON_UP) &&
               event->button.button == SDL_BUTTON_LEFT) {
        pui_pointer(pui, event->button.x, event->button.y,
                    event->type == SDL_EVENT_MOUSE_BUTTON_DOWN);
        if (event->type == SDL_EVENT_MOUSE_BUTTON_DOWN) {
            pui_widget_text_input_pointer(view, pui, window, event);
        }
    } else if (event->type == SDL_EVENT_MOUSE_WHEEL) {
        for (int i = view->layout->count - 1; i >= 0; i--) {
            if (!view->visible[i] || !view->enabled[i] ||
                !pui_contains(view->rect[i], event->wheel.mouse_x, event->wheel.mouse_y)) {
                continue;
            }
            if (pui_widget_tree_scroll(view, i, event) || pui_widget_tabs_scroll(view, i, event)) {
                return true;
            }
        }
    } else if (pui_widget_text_input_event(view, pui, event)) {
        return true;
    } else if (event->type == SDL_EVENT_KEY_DOWN && !event->key.repeat &&
               (event->key.key == SDLK_RETURN || event->key.key == SDLK_SPACE)) {
        pui_activate(pui);
    }
    return false;
}
