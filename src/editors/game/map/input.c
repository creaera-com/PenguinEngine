#include "editors/game/map/internal.h"

void editor_map_key(SDL_Keycode k) {
    if (k == SDLK_B) {
        app.workspace.map->view.tool = BRUSH;
    } else if (k == SDLK_E) {
        app.workspace.map->view.tool = ERASE;
    } else if (k == SDLK_R) {
        app.workspace.map->view.tool = RECTANGLE;
    } else if (k == SDLK_F) {
        app.workspace.map->view.tool = FILL;
    } else if (k == SDLK_V) {
        app.workspace.map->view.tool = SELECT;
    } else if (k == SDLK_DELETE && app.shell.selected) {
        editor_scene_delete();
    }
}

void editor_map_zoom(float delta) {
    pe_camera_zoom(&app.workspace.map->view.camera, app.shell.mx - app.shell.viewport.x,
                   app.shell.my - app.shell.viewport.y, powf(1.15f, delta));
}

void editor_map_pointer(SDL_Event *e, bool modal) {
    if (!app.workspace.map) {
        return;
    }
    switch (e->type) {
    case SDL_EVENT_MOUSE_BUTTON_DOWN:
        app.shell.mx = e->button.x;
        app.shell.my = e->button.y;
        if (e->button.button == SDL_BUTTON_LEFT) {
            if (modal) {
                app.shell.click = true;
            } else if (editor_inside(app.shell.mx, app.shell.my, app.shell.viewport) &&
                       app.shell.canvas_enabled && !app.workspace.map->view.playing &&
                       editor_map_active()) {
                editor_edit_start(app.shell.mx, app.shell.my, false);
            } else if (!editor_edit_pending()) {
                app.shell.click = true;
            }
        } else if (!modal && editor_inside(app.shell.mx, app.shell.my, app.shell.viewport) &&
                   app.shell.canvas_enabled && editor_map_active()) {
            if (e->button.button == SDL_BUTTON_MIDDLE) {
                app.workspace.map->view.panning = true;
            } else if (e->button.button == SDL_BUTTON_RIGHT &&
                       (app.workspace.map || app.workspace.script) &&
                       !app.workspace.map->view.playing && !editor_edit_pending()) {
                editor_edit_start(app.shell.mx, app.shell.my, true);
            }
        }
        break;
    case SDL_EVENT_MOUSE_BUTTON_UP:
        app.shell.mx = e->button.x;
        app.shell.my = e->button.y;
        if (e->button.button == SDL_BUTTON_MIDDLE) {
            app.workspace.map->view.panning = false;
        } else if (e->button.button == SDL_BUTTON_LEFT || e->button.button == SDL_BUTTON_RIGHT) {
            editor_edit_end(e->button.button == SDL_BUTTON_RIGHT ||
                            app.workspace.map->view.tool == ERASE);
        }
        break;
    case SDL_EVENT_MOUSE_MOTION:
        app.shell.mx = e->motion.x;
        app.shell.my = e->motion.y;
        if (app.workspace.map->view.panning) {
            app.workspace.map->view.camera.x -=
                e->motion.xrel / app.workspace.map->view.camera.zoom;
            app.workspace.map->view.camera.y -=
                e->motion.yrel / app.workspace.map->view.camera.zoom;
        }
        if (app.workspace.map->view.painting && app.workspace.map->view.tool != RECTANGLE &&
            editor_inside(app.shell.mx, app.shell.my, app.shell.viewport)) {
            int x, y;
            editor_tile_coords(app.shell.mx, app.shell.my, &x, &y);
            editor_paint_line(app.workspace.map->view.last_x, app.workspace.map->view.last_y, x, y,
                              app.workspace.map->view.tool == ERASE ||
                                  (e->motion.state & SDL_BUTTON_RMASK));
            app.workspace.map->view.last_x = x;
            app.workspace.map->view.last_y = y;
        }
        if (app.workspace.map->view.dragging) {
            PeObject *o =
                pe_entity_get(&app.workspace.map->scene,
                              pe_entity_find(&app.workspace.map->scene, app.shell.selected));
            if (o) {
                float x, y;
                pe_screen_to_world(&app.workspace.map->view.camera,
                                   app.shell.mx - app.shell.viewport.x,
                                   app.shell.my - app.shell.viewport.y, &x, &y);
                pe_entity_set_world_position(
                    &app.workspace.map->scene,
                    pe_entity_find(&app.workspace.map->scene, app.shell.selected),
                    x - app.workspace.map->view.drag_x, y - app.workspace.map->view.drag_y);
            }
        }
        break;
    default:
        break;
    }
}
