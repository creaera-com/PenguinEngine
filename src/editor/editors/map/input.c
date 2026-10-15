#include "app/editor_internal.h"

void editor_map_key(SDL_Keycode k) {
    if (k == SDLK_B)
        app.tool = BRUSH;
    else if (k == SDLK_E)
        app.tool = ERASE;
    else if (k == SDLK_R)
        app.tool = RECTANGLE;
    else if (k == SDLK_F)
        app.tool = FILL;
    else if (k == SDLK_V)
        app.tool = SELECT;
    else if (k == SDLK_DELETE && app.selected && editor_begin()) {
        pe_entity_destroy(&app.map->scene, pe_entity_find(&app.map->scene, app.selected));
        app.selected = 0;
        editor_commit();
    }
}

void editor_map_zoom(float delta) {
    pe_camera_zoom(&app.camera, app.mx - app.viewport.x, app.my - app.viewport.y,
                   powf(1.15f, delta));
}

void editor_map_pointer(SDL_Event *e, bool modal) {
    switch (e->type) {
    case SDL_EVENT_MOUSE_BUTTON_DOWN:
        app.mx = e->button.x;
        app.my = e->button.y;
        if (e->button.button == SDL_BUTTON_LEFT) {
            if (modal)
                app.click = true;
            else if (editor_inside(app.mx, app.my, app.viewport) && app.canvas_enabled &&
                     !app.playing && editor_map_active())
                editor_edit_start(app.mx, app.my, false);
            else if (!editor_edit_pending())
                app.click = true;
        } else if (!modal && editor_inside(app.mx, app.my, app.viewport) && app.canvas_enabled &&
                   editor_map_active()) {
            if (e->button.button == SDL_BUTTON_MIDDLE)
                app.panning = true;
            else if (e->button.button == SDL_BUTTON_RIGHT && (app.map || app.script) &&
                     !app.playing && !editor_edit_pending())
                editor_edit_start(app.mx, app.my, true);
        }
        break;
    case SDL_EVENT_MOUSE_BUTTON_UP:
        app.mx = e->button.x;
        app.my = e->button.y;
        if (e->button.button == SDL_BUTTON_MIDDLE)
            app.panning = false;
        else if (e->button.button == SDL_BUTTON_LEFT || e->button.button == SDL_BUTTON_RIGHT)
            editor_edit_end(e->button.button == SDL_BUTTON_RIGHT || app.tool == ERASE);
        break;
    case SDL_EVENT_MOUSE_MOTION:
        app.mx = e->motion.x;
        app.my = e->motion.y;
        if (app.panning) {
            app.camera.x -= e->motion.xrel / app.camera.zoom;
            app.camera.y -= e->motion.yrel / app.camera.zoom;
        }
        if (app.painting && app.tool != RECTANGLE && editor_inside(app.mx, app.my, app.viewport)) {
            int x, y;
            editor_tile_coords(app.mx, app.my, &x, &y);
            editor_paint_line(app.last_x, app.last_y, x, y,
                              app.tool == ERASE || (e->motion.state & SDL_BUTTON_RMASK));
            app.last_x = x;
            app.last_y = y;
        }
        if (app.dragging) {
            PeObject *o =
                pe_entity_get(&app.map->scene, pe_entity_find(&app.map->scene, app.selected));
            if (o) {
                float x, y;
                pe_screen_to_world(&app.camera, app.mx - app.viewport.x, app.my - app.viewport.y,
                                   &x, &y);
                o->x = x - app.drag_x;
                o->y = y - app.drag_y;
            }
        }
        break;
    default:
        break;
    }
}
