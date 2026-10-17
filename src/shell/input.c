#include "application/internal.h"

static void keyboard(SDL_KeyboardEvent *key);

static void keyboard(SDL_KeyboardEvent *key) {
    if (key->repeat) {
        return;
    }

    const PeEditorBackend *backend = app.workspace.backend;
    SDL_Keycode k = key->key;
    bool ctrl = (key->mod & (SDL_KMOD_CTRL | SDL_KMOD_GUI)) != 0,
         shift = (key->mod & SDL_KMOD_SHIFT) != 0;
    if (app.shell.request != REQUEST_NONE) {
        if (k == SDLK_ESCAPE) {
            app.shell.request = REQUEST_NONE;
        }
        return;
    }
    if (k == SDLK_ESCAPE) {
        if (editor_edit_pending()) {
            if (app.workspace.backend && app.workspace.backend->cancel) {
                app.workspace.backend->cancel();
            }
        } else if (editor_playing()) {
            editor_yaml_action("scene.toggle_play");
        }
        return;
    }
    if (k == SDLK_F6) {
        editor_reload_layout();
        return;
    }
    if (k == SDLK_F5) {
        if (backend && backend->playing) {
            editor_yaml_action("scene.toggle_play");
        }
        return;
    }
    if (editor_playing() || editor_edit_pending() || !app.workspace.loaded_view) {
        return;
    }
    if (ctrl && k == SDLK_S) {
        editor_save();
    } else if (ctrl && (k == SDLK_Y || (k == SDLK_Z && shift))) {
        editor_undo_active(true);
    } else if (ctrl && k == SDLK_Z) {
        editor_undo_active(false);
    } else if (!ctrl && backend && backend->key) {
        backend->key(k);
    }
}

void editor_event(SDL_Event *e) {
    if (app.shell.needs_layout) {
        editor_draw();
    }

    bool modal = (app.shell.field != FIELD_NONE && !app.shell.field_inline) ||
                 app.shell.request != REQUEST_NONE;
    if (e->type == SDL_EVENT_MOUSE_MOTION) {
        app.shell.mx = e->motion.x;
        app.shell.my = e->motion.y;
    } else if (e->type == SDL_EVENT_MOUSE_BUTTON_DOWN || e->type == SDL_EVENT_MOUSE_BUTTON_UP) {
        app.shell.mx = e->button.x;
        app.shell.my = e->button.y;
    }
    if (editor_field_event(e)) {
        return;
    }
    const PeEditorBackend *backend = app.workspace.backend;
    if (backend && backend->overlay_event && backend->overlay_event(e)) {
        return;
    }
    if (e->type == SDL_EVENT_MOUSE_MOTION) {
        ui_pointer(&app.shell.ui, app.shell.mx, app.shell.my, app.shell.ui.down);
    }
    if ((e->type == SDL_EVENT_MOUSE_BUTTON_DOWN || e->type == SDL_EVENT_MOUSE_BUTTON_UP) &&
        e->button.button == SDL_BUTTON_LEFT) {
        ui_pointer(&app.shell.ui, app.shell.mx, app.shell.my,
                   e->type == SDL_EVENT_MOUSE_BUTTON_DOWN);
    }

    if (pe_console_event(app.shell.console, app.platform.window, e,
                         !modal && !editor_edit_pending())) {
        if (pe_console_focused(app.shell.console)) {
            ui_focus(&app.shell.ui, 0x10010u);
            if (backend && backend->text_focus && backend->text_focus()) {
                if (backend->blur) {
                    backend->blur();
                }
                SDL_StartTextInput(app.platform.window);
            }
        }
        return;
    }
    bool captured =
        app.shell.ui.active == PE_CONSOLE_SCROLL_ID || editor_layout_captured() ||
        app.shell.ui.active == UI_SPLIT_LEFT || app.shell.ui.active == UI_SPLIT_RIGHT ||
        app.shell.ui.active == UI_TREE_SCROLL || app.shell.ui.active == UI_INSPECTOR_SCROLL ||
        app.shell.ui.active == UI_SCENE_SCROLL || app.shell.ui.active == UI_INFO_SCROLL ||
        app.shell.ui.active == UI_MAP_SCROLL || app.shell.ui.active == UI_TILES_SCROLL ||
        app.shell.ui.active == UI_SCRIPT_VSCROLL || app.shell.ui.active == UI_SCRIPT_HSCROLL;
    if (!modal && (e->type == SDL_EVENT_MOUSE_MOTION || e->type == SDL_EVENT_MOUSE_BUTTON_UP) &&
        captured) {
        return;
    }
    if (!modal && e->type == SDL_EVENT_MOUSE_BUTTON_DOWN && e->button.button == SDL_BUTTON_LEFT &&
        editor_ui_controls_hit(app.shell.mx, app.shell.my)) {
        app.shell.click = true;
        if (backend && backend->blur) {
            backend->blur();
        }
        return;
    }
    if (app.platform.window_collapsed && e->type != SDL_EVENT_QUIT &&
        e->type != SDL_EVENT_WINDOW_CLOSE_REQUESTED) {
        return;
    }
    if (e->type == SDL_EVENT_KEY_DOWN && !e->key.repeat &&
        (e->key.key == SDLK_RETURN || e->key.key == SDLK_SPACE) && app.shell.field == FIELD_NONE &&
        !(backend && backend->text_focus && backend->text_focus())) {
        ui_activate(&app.shell.ui);
    }
    if (backend && backend->event && backend->event(e, modal)) {
        return;
    }
    switch (e->type) {
    case SDL_EVENT_WINDOW_CLOSE_REQUESTED:
    case SDL_EVENT_QUIT:
        if (app.platform.window_collapsed) {
            editor_window_action("window.collapse");
        }
        if (editor_playing()) {
            editor_yaml_action("scene.toggle_play");
        }
        if (editor_edit_pending()) {
            if (app.workspace.backend && app.workspace.backend->cancel) {
                app.workspace.backend->cancel();
            }
        }
        editor_request(REQUEST_QUIT, NULL);
        break;
    case SDL_EVENT_KEY_DOWN:
        keyboard(&e->key);
        break;
    case SDL_EVENT_DROP_FILE:
        if (!modal && !editor_edit_pending()) {
            if (editor_suffix(e->drop.data, ".svg")) {
                editor_request(REQUEST_VECTOR_OPEN, e->drop.data);
            } else if (editor_suffix(e->drop.data, ".js")) {
                editor_request(REQUEST_SCRIPT_OPEN, e->drop.data);
            } else if (backend && backend->drop) {
                backend->drop(e->drop.data);
            }
        }
        break;
    case SDL_EVENT_WINDOW_FOCUS_LOST:
        ui_cancel(&app.shell.ui);
        if (editor_edit_pending()) {
            if (app.workspace.backend && app.workspace.backend->cancel) {
                app.workspace.backend->cancel();
            }
        }
        if (backend && backend->cancel) {
            backend->cancel();
        }
        break;
    case SDL_EVENT_MOUSE_WHEEL:
        app.shell.wheel_delta = e->wheel.y;
        if (!modal && !editor_edit_pending()) {
            if (app.shell.my < TOP) {
                ui_tabs_scroll(
                    &app.shell.workspaces,
                    fmaxf(0, editor_window_controls_left() - app.shell.global_nav_x - 12), 112,
                    -e->wheel.y * 56);
            } else if (editor_inside(app.shell.mx, app.shell.my, app.shell.viewport) &&
                       app.shell.canvas_enabled && backend && backend->zoom) {
                backend->zoom(e->wheel.y);
            } else if (app.shell.mx < LEFT && app.shell.my >= TOP && app.shell.my < SIDE_TOP) {
                ui_tabs_scroll(&app.shell.left_tabs, LEFT - 2 * PANEL_GAP, 112, -e->wheel.y * 56);
            } else if (app.shell.mx > app.platform.width - RIGHT && app.shell.my >= TOP &&
                       app.shell.my < SIDE_TOP) {
                ui_tabs_scroll(&app.shell.right_tabs, RIGHT - 2 * PANEL_GAP, 112, -e->wheel.y * 56);
            } else if (editor_layout_scrolls_at(app.shell.mx, app.shell.my)) {
                /* The YAML panel consumes wheel_delta during layout. */
            } else if (app.shell.mx < LEFT) {
                UiScroll *scroll = backend && backend->panel_scroll
                                       ? backend->panel_scroll(app.shell.left_tabs.selected)
                                       : NULL;
                ui_scroll_by(scroll                                         ? scroll
                             : app.shell.left_tabs.selected == UI_SCENE_TAB ? &app.shell.scene_view
                                                                            : &app.shell.tree_view,
                             -e->wheel.y * 69);
            } else if (app.shell.mx > app.platform.width - RIGHT) {
                UiScroll *scroll = backend && backend->panel_scroll
                                       ? backend->panel_scroll(app.shell.right_tabs.selected)
                                       : NULL;
                ui_scroll_by(scroll ? scroll
                             : app.shell.right_tabs.selected == UI_INFO_TAB
                                 ? &app.shell.info_view
                                 : &app.shell.inspector_view,
                             -e->wheel.y * 60);
            }
        }
        break;
    default:
        if (backend && backend->pointer) {
            backend->pointer(e, modal);
        }
        break;
    }
}
