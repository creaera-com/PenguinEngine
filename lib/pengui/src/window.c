#include "pengui/pengui.h"
#include "view/internal.h"
#include <string.h>

SDL_HitTestResult pui_window_hit_test(const PUIWindow *chrome, int x, int y) {
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
    if (!chrome->view || !chrome->pui || chrome->pui->active || chrome->pui->down) {
        return SDL_HITTEST_NORMAL;
    }
    const PUIView *view = chrome->view;
    bool draggable = false;
    for (int i = 0; i < view->layout->count; i++) {
        if (!view->visible[i] || !pui_contains(view->hit[i], (float)x, (float)y)) {
            continue;
        }
        const PUINode *node = &view->layout->nodes[i];
        if (node->type == PUI_NODE_BUTTON || node->type == PUI_NODE_INPUT ||
            node->type == PUI_NODE_TABS || node->type == PUI_NODE_TREE) {
            return SDL_HITTEST_NORMAL;
        }
        draggable = draggable || node->draggable;
    }
    return draggable ? SDL_HITTEST_DRAGGABLE : SDL_HITTEST_NORMAL;
}

static SDL_HitTestResult SDLCALL hit(SDL_Window *window, const SDL_Point *point, void *context) {
    (void)window;
    return pui_window_hit_test(context, point->x, point->y);
}

bool pui_window_attach(PUIWindow *chrome, SDL_Window *window, const PUIView *view,
                       const PUIContext *pui) {
    *chrome = (PUIWindow){window, view, pui};
    bool supported = SDL_SetWindowHitTest(window, hit, chrome);
    SDL_SetWindowBordered(window, !supported);
    return supported;
}

void pui_window_detach(PUIWindow *chrome) {
    if (chrome->window) {
        SDL_SetWindowHitTest(chrome->window, NULL, NULL);
    }
    *chrome = (PUIWindow){0};
}

bool pui_window_action(SDL_Window *window, const char *action) {
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
