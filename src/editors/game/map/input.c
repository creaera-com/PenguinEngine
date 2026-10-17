#include "editors/game/map/internal.h"

void editor_map_key(SDL_Keycode k) {
    if (k == SDLK_B) {
        pe_map_active_instance()->view.tool = BRUSH;
    } else if (k == SDLK_E) {
        pe_map_active_instance()->view.tool = ERASE;
    } else if (k == SDLK_R) {
        pe_map_active_instance()->view.tool = RECTANGLE;
    } else if (k == SDLK_F) {
        pe_map_active_instance()->view.tool = FILL;
    } else if (k == SDLK_V) {
        pe_map_active_instance()->view.tool = SELECT;
    } else if (k == SDLK_DELETE && app.shell.selected) {
        editor_scene_delete();
    }
}

void editor_map_zoom(float delta) {
    pe_camera_zoom(&pe_map_active_instance()->view.camera, app.shell.mx - app.shell.viewport.x,
                   app.shell.my - app.shell.viewport.y, powf(1.15f, delta));
}

void editor_map_pointer(SDL_Event *e, bool modal) {
    if (!pe_map_active_instance()) {
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
                       app.shell.canvas_enabled && !pe_map_active_instance()->view.playing &&
                       editor_map_active()) {
                editor_edit_start(app.shell.mx, app.shell.my, false);
            } else if (!editor_edit_pending()) {
                app.shell.click = true;
            }
        } else if (!modal && editor_inside(app.shell.mx, app.shell.my, app.shell.viewport) &&
                   app.shell.canvas_enabled && editor_map_active()) {
            if (e->button.button == SDL_BUTTON_MIDDLE) {
                pe_map_active_instance()->view.panning = true;
            } else if (e->button.button == SDL_BUTTON_RIGHT && pe_map_active_instance() &&
                       !pe_map_active_instance()->view.playing && !editor_edit_pending()) {
                editor_edit_start(app.shell.mx, app.shell.my, true);
            }
        }
        break;
    case SDL_EVENT_MOUSE_BUTTON_UP:
        app.shell.mx = e->button.x;
        app.shell.my = e->button.y;
        if (e->button.button == SDL_BUTTON_MIDDLE) {
            pe_map_active_instance()->view.panning = false;
        } else if (e->button.button == SDL_BUTTON_LEFT || e->button.button == SDL_BUTTON_RIGHT) {
            editor_edit_end(e->button.button == SDL_BUTTON_RIGHT ||
                            pe_map_active_instance()->view.tool == ERASE);
        }
        break;
    case SDL_EVENT_MOUSE_MOTION:
        app.shell.mx = e->motion.x;
        app.shell.my = e->motion.y;
        if (pe_map_active_instance()->view.panning) {
            pe_map_active_instance()->view.camera.x -=
                e->motion.xrel / pe_map_active_instance()->view.camera.zoom;
            pe_map_active_instance()->view.camera.y -=
                e->motion.yrel / pe_map_active_instance()->view.camera.zoom;
        }
        if (pe_map_active_instance()->view.painting &&
            pe_map_active_instance()->view.tool != RECTANGLE &&
            editor_inside(app.shell.mx, app.shell.my, app.shell.viewport)) {
            int x, y;
            editor_tile_coords(app.shell.mx, app.shell.my, &x, &y);
            editor_paint_line(pe_map_active_instance()->view.last_x,
                              pe_map_active_instance()->view.last_y, x, y,
                              pe_map_active_instance()->view.tool == ERASE ||
                                  (e->motion.state & SDL_BUTTON_RMASK));
            pe_map_active_instance()->view.last_x = x;
            pe_map_active_instance()->view.last_y = y;
        }
        if (pe_map_active_instance()->view.dragging) {
            PeObject *o =
                pe_entity_get(&pe_map_active_instance()->scene,
                              pe_entity_find(&pe_map_active_instance()->scene, app.shell.selected));
            if (o) {
                float x, y;
                pe_screen_to_world(&pe_map_active_instance()->view.camera,
                                   app.shell.mx - app.shell.viewport.x,
                                   app.shell.my - app.shell.viewport.y, &x, &y);
                pe_entity_set_world_position(
                    &pe_map_active_instance()->scene,
                    pe_entity_find(&pe_map_active_instance()->scene, app.shell.selected),
                    x - pe_map_active_instance()->view.drag_x,
                    y - pe_map_active_instance()->view.drag_y);
            }
        }
        break;
    default:
        break;
    }
}
