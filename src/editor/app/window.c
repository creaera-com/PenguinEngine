#include "app/editor_internal.h"

static SDL_HitTestResult SDLCALL window_hit(SDL_Window *window, const SDL_Point *point,
                                            void *data) {
    (void)data;
    (void)window;
    return editor_window_hit_test(point->x, point->y);
}

SDL_HitTestResult editor_window_hit_test(int x, int y) {
    SDL_Window *window = app.window;
    int width, height;
    SDL_GetWindowSize(window, &width, &height);
    if (!(SDL_GetWindowFlags(window) & SDL_WINDOW_MAXIMIZED)) {
        bool left = x < 5, right = x >= width - 5;
        bool top = !app.window_collapsed && y < 5;
        bool bottom = !app.window_collapsed && y >= height - 5;
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
    if (y >= TOP || app.field != FIELD_NONE || app.request != REQUEST_NONE ||
        x >= editor_window_controls_left()) {
        return SDL_HITTEST_NORMAL;
    }
    for (int i = 0; i < app.hit_count; i++) {
        if (ui_contains(app.hits[i], (float)x, (float)y)) {
            return SDL_HITTEST_NORMAL;
        }
    }
    return SDL_HITTEST_DRAGGABLE;
}

void editor_window_init(void) {
    const cJSON *controls = pe_ui_get(app.shell_layout->root, "window_controls");
    if (controls && cJSON_GetArraySize(controls) &&
        SDL_SetWindowHitTest(app.window, window_hit, NULL)) {
        SDL_SetWindowBordered(app.window, false);
    } else {
        SDL_SetWindowHitTest(app.window, NULL, NULL);
        SDL_SetWindowBordered(app.window, true);
    }
}

float editor_window_controls_left(void) {
    const cJSON *controls = pe_ui_get(app.shell_layout->root, "window_controls");
    return app.width - 14.f - cJSON_GetArraySize(controls) * 34.f;
}

static void expand_window(void) {
    app.window_collapsed = false;
    SDL_SetWindowSize(app.window, app.window_restore_width, app.window_restore_height);
    SDL_SetWindowMinimumSize(app.window, 1100, 740);
    if (app.window_was_maximized) {
        SDL_MaximizeWindow(app.window);
    }
    app.needs_layout = true;
}

bool editor_window_action(const char *name) {
    if (!strcmp(name, "window.close")) {
        if (app.window_collapsed) {
            expand_window();
        }
        SDL_Event event = {0};
        event.type = SDL_EVENT_QUIT;
        SDL_PushEvent(&event);
    } else if (!strcmp(name, "window.minimize")) {
        SDL_MinimizeWindow(app.window);
    } else if (!strcmp(name, "window.maximize")) {
        bool maximized = app.window_collapsed
                             ? app.window_was_maximized
                             : (SDL_GetWindowFlags(app.window) & SDL_WINDOW_MAXIMIZED) != 0;
        if (app.window_collapsed) {
            expand_window();
        }
        if (maximized) {
            SDL_RestoreWindow(app.window);
        } else {
            SDL_MaximizeWindow(app.window);
        }
    } else if (!strcmp(name, "window.collapse")) {
        if (app.window_collapsed) {
            expand_window();
        } else {
            SDL_GetWindowSize(app.window, &app.window_restore_width, &app.window_restore_height);
            app.window_was_maximized = (SDL_GetWindowFlags(app.window) & SDL_WINDOW_MAXIMIZED) != 0;
            if (app.window_was_maximized) {
                SDL_RestoreWindow(app.window);
            }
            SDL_SetWindowMinimumSize(app.window, 1100, TOP);
            SDL_SetWindowSize(app.window, app.window_restore_width, TOP);
            app.window_collapsed = true;
            ui_cancel(&app.ui);
        }
    } else {
        return false;
    }
    return true;
}

void editor_window_draw_controls(void) {
    const cJSON *controls = pe_ui_get(app.shell_layout->root, "window_controls");
    float x = editor_window_controls_left();
    for (const cJSON *n = controls ? controls->child : NULL; n; n = n->next) {
        const char *action = pe_ui_string(n, "action", "");
        UiIcon icon = editor_yaml_icon(pe_ui_string(n, "icon", ""));
        if (!strcmp(action, "window.maximize") &&
            (SDL_GetWindowFlags(app.window) & SDL_WINDOW_MAXIMIZED)) {
            icon = ICON_RESTORE;
        }
        if (!strcmp(action, "window.collapse") && app.window_collapsed) {
            icon = ICON_DOWN;
        }
        /* Each YAML ID needs a distinct immediate-mode button ID. */
        app.widget_scope = ui_id(pe_ui_string(n, "id", ""));
        bool clicked = editor_icon_button_impl(ui_id("window_control"), x, 12, 32, icon, "",
                                               pe_ui_string(n, "tooltip", ""), false, true);
        app.widget_scope = 0;
        if (clicked) {
            editor_window_action(action);
        }
        x += 34;
    }
}
