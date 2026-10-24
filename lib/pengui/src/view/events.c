/** @file @brief Route native input to the responsible widgets. */
#include "internal.h"

bool pui_view_event(PUIView *view, PUIContext *pui, SDL_Window *window, const SDL_Event *event) {
    view->window = window;
    if (pui_numeric_drag_event(view, pui, event)) {
        return true;
    }
    if (!pui_view_modal_active(view) && pui_widget_radial_event(view, pui, event)) {
        return true;
    }
    bool color_consumed = pui_widget_color_picker_event(view, pui, event);
    if (color_consumed && event->type != SDL_EVENT_WINDOW_FOCUS_LOST) {
        return true;
    }
    if (pui_view_popup_event(view, pui, event)) {
        return true;
    }
    if (event->type == SDL_EVENT_WINDOW_FOCUS_LOST) {
        view->pointer_inside = false;
        while (view->popup_count &&
               !view->layout->nodes[view->popups[view->popup_count - 1]].modal) {
            pui_view_popup_close(view, pui);
        }
        pui_cancel(pui);
        pui_view_blur(view, pui);
    } else if (event->type == SDL_EVENT_WINDOW_MOUSE_ENTER ||
               event->type == SDL_EVENT_WINDOW_MOUSE_LEAVE) {
        view->pointer_inside = event->type == SDL_EVENT_WINDOW_MOUSE_ENTER;
    } else if (event->type == SDL_EVENT_MOUSE_MOTION) {
        view->pointer_inside = true;
        pui_pointer(pui, event->motion.x, event->motion.y, pui->down);
        if (pui_widget_text_input_drag(view, pui, event)) {
            return true;
        }
    } else if ((event->type == SDL_EVENT_MOUSE_BUTTON_DOWN ||
                event->type == SDL_EVENT_MOUSE_BUTTON_UP) &&
               event->button.button == SDL_BUTTON_LEFT) {
        view->pointer_inside = true;
        pui_pointer(pui, event->button.x, event->button.y,
                    event->type == SDL_EVENT_MOUSE_BUTTON_DOWN);
        if (event->type == SDL_EVENT_MOUSE_BUTTON_DOWN) {
            pui_widget_text_input_pointer(view, pui, window, event);
            if (view->selecting) {
                pui_numeric_drag_begin(view, event);
            }
        } else if (pui_widget_text_input_drag(view, pui, event)) {
            return true;
        }
    } else if (event->type == SDL_EVENT_MOUSE_WHEEL) {
        for (int i = view->layout->count - 1; i >= 0; i--) {
            if (!view->visible[i] || !view->enabled[i] ||
                !pui_contains(view->hit[i], event->wheel.mouse_x, event->wheel.mouse_y)) {
                continue;
            }
            if (pui_widget_tree_scroll(view, i, event) || pui_widget_tabs_scroll(view, i, event)) {
                return true;
            }
            if (view->layout->nodes[i].scrollable && pui_scroll_max(&view->state[i].scroll) > 0) {
                float delta = -event->wheel.y * 48;
                if (event->wheel.direction == SDL_MOUSEWHEEL_FLIPPED) {
                    delta = -delta;
                }
                pui_scroll_by(&view->state[i].scroll, delta);
                return true;
            }
        }
    } else if (pui_widget_text_input_event(view, pui, event)) {
        return true;
    } else if (event->type == SDL_EVENT_KEY_DOWN && !event->key.repeat &&
               (event->key.key == SDLK_RETURN || event->key.key == SDLK_SPACE)) {
        pui_activate(pui);
    }
    return pui_view_modal_active(view);
}
