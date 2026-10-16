#include "application/internal.h"

static void keyboard(SDL_KeyboardEvent *key);

static void keyboard(SDL_KeyboardEvent *key) {
    if (key->repeat) {
        return;
    }

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
            pe_history_cancel(&app.workspace.map->history, &app.workspace.map->scene);
            app.workspace.map->view.painting = app.workspace.map->view.dragging = false;
        } else if (editor_playing()) {
            editor_play_toggle();
        }
        return;
    }
    if (k == SDLK_F6) {
        editor_reload_layout();
        return;
    }
    if (k == SDLK_F5) {
        if (editor_map_active()) {
            editor_play_toggle();
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
    } else if (!ctrl && editor_map_active()) {
        editor_map_key(k);
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
    if (editor_component_popup_event(e)) {
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
            if (app.workspace.script && app.workspace.script->focused) {
                se_focus(app.workspace.script, app.platform.window, false);
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
        if (app.workspace.script) {
            se_focus(app.workspace.script, app.platform.window, false);
        }
        return;
    }
    if (app.platform.window_collapsed && e->type != SDL_EVENT_QUIT &&
        e->type != SDL_EVENT_WINDOW_CLOSE_REQUESTED) {
        return;
    }
    if (e->type == SDL_EVENT_KEY_DOWN && !e->key.repeat &&
        (e->key.key == SDLK_RETURN || e->key.key == SDLK_SPACE) && app.shell.field == FIELD_NONE &&
        !(editor_scripts_active() && (*app.workspace.script).focused)) {
        ui_activate(&app.shell.ui);
    }
    if (editor_scripts_active() && app.shell.canvas_enabled && !modal &&
        se_event(&(*app.workspace.script), app.platform.window, e, app.shell.viewport)) {
        if (e->type == SDL_EVENT_MOUSE_BUTTON_DOWN && (*app.workspace.script).focused) {
            ui_focus(&app.shell.ui, UI_SCRIPT_DOCUMENT);
        }
        return;
    }
    if (vector_event(e)) {
        return;
    }
    switch (e->type) {
    case SDL_EVENT_WINDOW_CLOSE_REQUESTED:
    case SDL_EVENT_QUIT:
        if (app.platform.window_collapsed) {
            editor_window_action("window.collapse");
        }
        if (editor_playing()) {
            editor_play_toggle();
        }
        if (editor_edit_pending()) {
            pe_history_cancel(&app.workspace.map->history, &app.workspace.map->scene);
            app.workspace.map->view.painting = app.workspace.map->view.dragging = false;
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
            } else if (editor_map_active()) {
                editor_import_png(e->drop.data);
            }
        }
        break;
    case SDL_EVENT_WINDOW_FOCUS_LOST:
        ui_cancel(&app.shell.ui);
        if (editor_edit_pending()) {
            pe_history_cancel(&app.workspace.map->history, &app.workspace.map->scene);
            app.workspace.map->view.painting = app.workspace.map->view.dragging = false;
        }
        if (app.workspace.map) {
            app.workspace.map->view.panning = false;
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
                       app.shell.canvas_enabled && editor_map_active()) {
                editor_map_zoom(e->wheel.y);
            } else if (app.shell.mx < LEFT && app.shell.my >= TOP && app.shell.my < SIDE_TOP) {
                ui_tabs_scroll(&app.shell.left_tabs, LEFT - 2 * PANEL_GAP, 112, -e->wheel.y * 56);
            } else if (app.shell.mx > app.platform.width - RIGHT && app.shell.my >= TOP &&
                       app.shell.my < SIDE_TOP) {
                ui_tabs_scroll(&app.shell.right_tabs, RIGHT - 2 * PANEL_GAP, 112, -e->wheel.y * 56);
            } else if (editor_layout_scrolls_at(app.shell.mx, app.shell.my)) {
                /* The YAML panel consumes wheel_delta during layout. */
            } else if (app.shell.mx < LEFT) {
                ui_scroll_by(app.workspace.map && app.shell.left_tabs.selected == UI_MAP_TAB
                                 ? &app.workspace.map->view.map_view
                             : app.shell.left_tabs.selected == UI_SCENE_TAB ? &app.shell.scene_view
                                                                            : &app.shell.tree_view,
                             -e->wheel.y * 69);
            } else if (app.shell.mx > app.platform.width - RIGHT) {
                ui_scroll_by(app.workspace.map && app.shell.right_tabs.selected == UI_TILES_TAB
                                 ? &app.workspace.map->view.tiles_view
                             : app.shell.right_tabs.selected == UI_INFO_TAB
                                 ? &app.shell.info_view
                                 : &app.shell.inspector_view,
                             -e->wheel.y * 60);
            }
        }
        break;
    default:
        editor_map_pointer(e, modal);
        break;
    }
}
