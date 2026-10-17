#include "pengui/window.h"
#include "view/internal.h"
#include <string.h>

SDL_HitTestResult ui_window_hit_test(const UiWindow *chrome, int x, int y) {
    int width, height;
    SDL_GetWindowSize(chrome->window, &width, &height);
    if (!(SDL_GetWindowFlags(chrome->window) & SDL_WINDOW_MAXIMIZED)) {
        bool left = x < 5, right = x >= width - 5;
        bool top = y < 5, bottom = y >= height - 5;
        if (top && left) {
            return SDL_HITTEST_RESIZE_TOPLEFT;
        }
        if (top && right) {
            return SDL_HITTEST_RESIZE_TOPRIGHT;
        }
        if (bottom && left) {
            return SDL_HITTEST_RESIZE_BOTTOMLEFT;
        }
        if (bottom && right) {
            return SDL_HITTEST_RESIZE_BOTTOMRIGHT;
        }
        if (left) {
            return SDL_HITTEST_RESIZE_LEFT;
        }
        if (right) {
            return SDL_HITTEST_RESIZE_RIGHT;
        }
        if (top) {
            return SDL_HITTEST_RESIZE_TOP;
        }
        if (bottom) {
            return SDL_HITTEST_RESIZE_BOTTOM;
        }
    }
    if (!chrome->view || !chrome->ui || chrome->ui->active || chrome->ui->down) {
        return SDL_HITTEST_NORMAL;
    }
    const UiView *view = chrome->view;
    bool draggable = false;
    for (int i = 0; i < view->layout->count; i++) {
        if (!view->visible[i] || !ui_contains(view->hit[i], (float)x, (float)y)) {
            continue;
        }
        const UiNode *node = &view->layout->nodes[i];
        if (node->type == UI_NODE_BUTTON || node->type == UI_NODE_INPUT ||
            node->type == UI_NODE_TABS || node->type == UI_NODE_TREE) {
            return SDL_HITTEST_NORMAL;
        }
        draggable = draggable || node->draggable;
    }
    return draggable ? SDL_HITTEST_DRAGGABLE : SDL_HITTEST_NORMAL;
}

static SDL_HitTestResult SDLCALL hit(SDL_Window *window, const SDL_Point *point, void *context) {
    (void)window;
    return ui_window_hit_test(context, point->x, point->y);
}

bool ui_window_attach(UiWindow *chrome, SDL_Window *window, const UiView *view,
                      const UiContext *ui) {
    *chrome = (UiWindow){window, view, ui};
    bool supported = SDL_SetWindowHitTest(window, hit, chrome);
    SDL_SetWindowBordered(window, !supported);
    return supported;
}

void ui_window_detach(UiWindow *chrome) {
    if (chrome->window) {
        SDL_SetWindowHitTest(chrome->window, NULL, NULL);
    }
    *chrome = (UiWindow){0};
}

bool ui_window_action(SDL_Window *window, const char *action) {
    if (!window || !action) {
        return false;
    }
    if (!strcmp(action, "window.minimize")) {
        SDL_MinimizeWindow(window);
    } else if (!strcmp(action, "window.maximize")) {
        if (SDL_GetWindowFlags(window) & SDL_WINDOW_MAXIMIZED) {
            SDL_RestoreWindow(window);
        } else {
            SDL_MaximizeWindow(window);
        }
    } else if (!strcmp(action, "window.close")) {
        SDL_Event event = {0};
        event.type = SDL_EVENT_WINDOW_CLOSE_REQUESTED;
        event.window.windowID = SDL_GetWindowID(window);
        SDL_PushEvent(&event);
    } else {
        return false;
    }
    return true;
}
