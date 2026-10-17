/** @file @brief Route native input to the responsible widgets. */
#include "internal.h"

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
            ui_widget_text_input_pointer(view, ui, window, event);
        }
    } else if (event->type == SDL_EVENT_MOUSE_WHEEL) {
        for (int i = view->layout->count - 1; i >= 0; i--) {
            if (!view->visible[i] || !view->enabled[i] ||
                !ui_contains(view->rect[i], event->wheel.mouse_x, event->wheel.mouse_y)) {
                continue;
            }
            if (ui_widget_tree_scroll(view, i, event) || ui_widget_tabs_scroll(view, i, event)) {
                return true;
            }
        }
    } else if (ui_widget_text_input_event(view, ui, event)) {
        return true;
    } else if (event->type == SDL_EVENT_KEY_DOWN && !event->key.repeat &&
               (event->key.key == SDLK_RETURN || event->key.key == SDLK_SPACE)) {
        ui_activate(ui);
    }
    return false;
}
