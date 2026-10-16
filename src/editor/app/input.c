#include "app/editor_internal.h"

static void keyboard(SDL_KeyboardEvent *key);

static void keyboard(SDL_KeyboardEvent *key) {
    if (key->repeat) {
        return;
    }

    SDL_Keycode k = key->key;
    bool ctrl = (key->mod & (SDL_KMOD_CTRL | SDL_KMOD_GUI)) != 0,
         shift = (key->mod & SDL_KMOD_SHIFT) != 0;
    if (app.request != REQUEST_NONE) {
        if (k == SDLK_ESCAPE) {
            app.request = REQUEST_NONE;
        }
        return;
    }
    if (k == SDLK_ESCAPE) {
        if (editor_edit_pending()) {
            pe_history_cancel(&app.map->history, &app.map->scene);
            app.painting = app.dragging = false;
        } else if (app.playing) {
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
    if (app.playing || editor_edit_pending() || !app.loaded_view) {
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
    if (app.needs_layout) {
        editor_draw();
    }

    bool modal = (app.field != FIELD_NONE && !app.field_inline) || app.request != REQUEST_NONE;
    if (e->type == SDL_EVENT_MOUSE_MOTION) {
        app.mx = e->motion.x;
        app.my = e->motion.y;
    } else if (e->type == SDL_EVENT_MOUSE_BUTTON_DOWN || e->type == SDL_EVENT_MOUSE_BUTTON_UP) {
        app.mx = e->button.x;
        app.my = e->button.y;
    }
    if (editor_field_event(e)) {
        return;
    }
    if (editor_component_popup_event(e)) {
        return;
    }
    if (e->type == SDL_EVENT_MOUSE_MOTION) {
        ui_pointer(&app.ui, app.mx, app.my, app.ui.down);
    }
    if ((e->type == SDL_EVENT_MOUSE_BUTTON_DOWN || e->type == SDL_EVENT_MOUSE_BUTTON_UP) &&
        e->button.button == SDL_BUTTON_LEFT) {
        ui_pointer(&app.ui, app.mx, app.my, e->type == SDL_EVENT_MOUSE_BUTTON_DOWN);
    }

    if (pe_console_event(app.console, app.window, e, !modal && !editor_edit_pending())) {
        if (pe_console_focused(app.console)) {
            ui_focus(&app.ui, 0x10010u);
            if (app.script && app.script->focused) {
                se_focus(app.script, app.window, false);
                SDL_StartTextInput(app.window);
            }
        }
        return;
    }
    bool captured = app.ui.active == PE_CONSOLE_SCROLL_ID || editor_layout_captured() ||
                    app.ui.active == UI_SPLIT_LEFT || app.ui.active == UI_SPLIT_RIGHT ||
                    app.ui.active == UI_TREE_SCROLL || app.ui.active == UI_INSPECTOR_SCROLL ||
                    app.ui.active == UI_SCENE_SCROLL || app.ui.active == UI_INFO_SCROLL ||
                    app.ui.active == UI_MAP_SCROLL || app.ui.active == UI_TILES_SCROLL ||
                    app.ui.active == UI_SCRIPT_VSCROLL || app.ui.active == UI_SCRIPT_HSCROLL;
    if (!modal && (e->type == SDL_EVENT_MOUSE_MOTION || e->type == SDL_EVENT_MOUSE_BUTTON_UP) &&
        captured) {
        return;
    }
    if (!modal && e->type == SDL_EVENT_MOUSE_BUTTON_DOWN && e->button.button == SDL_BUTTON_LEFT &&
        editor_ui_controls_hit(app.mx, app.my)) {
        app.click = true;
        if (app.script) {
            se_focus(app.script, app.window, false);
        }
        return;
    }
    if (app.window_collapsed && e->type != SDL_EVENT_QUIT &&
        e->type != SDL_EVENT_WINDOW_CLOSE_REQUESTED) {
        return;
    }
    if (e->type == SDL_EVENT_KEY_DOWN && !e->key.repeat &&
        (e->key.key == SDLK_RETURN || e->key.key == SDLK_SPACE) && app.field == FIELD_NONE &&
        !(editor_scripts_active() && (*app.script).focused)) {
        ui_activate(&app.ui);
    }
    if (editor_scripts_active() && app.canvas_enabled && !modal &&
        se_event(&(*app.script), app.window, e, app.viewport)) {
        if (e->type == SDL_EVENT_MOUSE_BUTTON_DOWN && (*app.script).focused) {
            ui_focus(&app.ui, UI_SCRIPT_DOCUMENT);
        }
        return;
    }
    if (vector_event(e)) {
        return;
    }
    switch (e->type) {
    case SDL_EVENT_WINDOW_CLOSE_REQUESTED:
    case SDL_EVENT_QUIT:
        if (app.window_collapsed) {
            editor_window_action("window.collapse");
        }
        if (app.playing) {
            editor_play_toggle();
        }
        if (editor_edit_pending()) {
            pe_history_cancel(&app.map->history, &app.map->scene);
            app.painting = app.dragging = false;
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
        ui_cancel(&app.ui);
        if (editor_edit_pending()) {
            pe_history_cancel(&app.map->history, &app.map->scene);
            app.painting = app.dragging = false;
        }
        app.panning = false;
        break;
    case SDL_EVENT_MOUSE_WHEEL:
        app.wheel_delta = e->wheel.y;
        if (!modal && !editor_edit_pending()) {
            if (app.my < TOP) {
                ui_tabs_scroll(&app.workspaces,
                               fmaxf(0, editor_window_controls_left() - app.global_nav_x - 12), 112,
                               -e->wheel.y * 56);
            } else if (editor_inside(app.mx, app.my, app.viewport) && app.canvas_enabled &&
                       editor_map_active()) {
                editor_map_zoom(e->wheel.y);
            } else if (app.mx < LEFT && app.my >= TOP && app.my < SIDE_TOP) {
                ui_tabs_scroll(&app.left_tabs, LEFT - 2 * PANEL_GAP, 112, -e->wheel.y * 56);
            } else if (app.mx > app.width - RIGHT && app.my >= TOP && app.my < SIDE_TOP) {
                ui_tabs_scroll(&app.right_tabs, RIGHT - 2 * PANEL_GAP, 112, -e->wheel.y * 56);
            } else if (app.mx < LEFT) {
                ui_scroll_by(app.left_tabs.selected == UI_MAP_TAB     ? &app.map_view
                             : app.left_tabs.selected == UI_SCENE_TAB ? &app.scene_view
                                                                      : &app.tree_view,
                             -e->wheel.y * 69);
            } else if (app.mx > app.width - RIGHT) {
                ui_scroll_by(app.right_tabs.selected == UI_TILES_TAB  ? &app.tiles_view
                             : app.right_tabs.selected == UI_INFO_TAB ? &app.info_view
                                                                      : &app.inspector_view,
                             -e->wheel.y * 60);
            }
        }
        break;
    default:
        editor_map_pointer(e, modal);
        break;
    }
}
