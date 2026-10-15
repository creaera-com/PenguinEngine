#include "app/editor_internal.h"

void editor_draw_map_tools(SDL_FRect area) {
    editor_draw_yaml_toolbar(area);
}

void editor_draw_map_content(void) {
    bool modal = app.field != FIELD_NONE || app.request != REQUEST_NONE;
    /* Distinguish the workspace from empty/transparent tiles inside the map. */
    editor_box(app.viewport, 0x383838ffu);
    SDL_Rect map_clip = {(int)app.viewport.x, (int)app.viewport.y, (int)app.viewport.w,
                         (int)app.viewport.h};
    SDL_SetRenderClipRect(app.renderer, &map_clip);
    const PeScene *visible_scene = app.playing ? &app.map->runtime : &app.map->scene;
    editor_box((SDL_FRect){app.viewport.x - app.camera.x * app.camera.zoom,
                           app.viewport.y - app.camera.y * app.camera.zoom,
                           visible_scene->width * visible_scene->tile_size * app.camera.zoom,
                           visible_scene->height * visible_scene->tile_size * app.camera.zoom},
               UI_CANVAS);
    SDL_SetRenderClipRect(app.renderer, NULL);
    pe_render_scene(app.renderer, app.playing ? &app.map->runtime : &app.map->scene,
                    &app.map->assets, &app.camera, app.viewport, app.grid && !app.playing,
                    app.collision && !app.playing);
    if (!app.playing) {
        PeObject *o = pe_entity_get(&app.map->scene, pe_entity_find(&app.map->scene, app.selected));
        float x, y;
        if (o && pe_entity_world_position(&app.map->scene,
                                          pe_entity_find(&app.map->scene, app.selected), &x, &y)) {
            SDL_FRect rect = {app.viewport.x + (x - app.camera.x) * app.camera.zoom,
                              app.viewport.y + (y - app.camera.y) * app.camera.zoom,
                              o->w * app.camera.zoom, o->h * app.camera.zoom};
            editor_draw_color(UI_ACCENT);
            SDL_RenderRect(app.renderer, &rect);
        }
        if (editor_inside(app.mx, app.my, app.viewport) && !modal && app.tool != SELECT) {
            int x, y;
            editor_tile_coords(app.mx, app.my, &x, &y);
            int sx = app.painting && app.tool == RECTANGLE ? app.start_x : x,
                sy = app.painting && app.tool == RECTANGLE ? app.start_y : y;
            int minx = sx < x ? sx : x, miny = sy < y ? sy : y;
            SDL_FRect rect = {
                app.viewport.x + (minx * app.map->scene.tile_size - app.camera.x) * app.camera.zoom,
                app.viewport.y + (miny * app.map->scene.tile_size - app.camera.y) * app.camera.zoom,
                (abs(x - sx) + 1) * app.map->scene.tile_size * app.camera.zoom,
                (abs(y - sy) + 1) * app.map->scene.tile_size * app.camera.zoom};
            SDL_Rect clip = {(int)app.viewport.x, (int)app.viewport.y, (int)app.viewport.w,
                             (int)app.viewport.h};
            SDL_SetRenderClipRect(app.renderer, &clip);
            editor_draw_color(UI_ACCENT);
            SDL_RenderRect(app.renderer, &rect);
            SDL_SetRenderClipRect(app.renderer, NULL);
        }
    }
}
