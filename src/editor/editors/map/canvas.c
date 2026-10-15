#include "app/editor_internal.h"

void editor_draw_map_tools(SDL_FRect area) {
    editor_draw_yaml_toolbar(area);
}

void editor_draw_map_content(void) {
    bool modal = app.field != FIELD_NONE || app.request != REQUEST_NONE;
    /* Distinguish the workspace from empty/transparent tiles inside the map. */
    editor_box(app.viewport, 0x383838ffu);
    const PeScene *selected_scene = app.playing ? &app.map->runtime : &app.map->scene;
    if (app.map->reveal_selected && !app.playing) {
        float x, y;
        PeEntity selected = pe_entity_find(selected_scene, app.selected);
        const PeObject *o = pe_entity_get(&app.map->scene, selected);
        if (o && pe_entity_world_position(selected_scene, selected, &x, &y)) {
            float margin = 20 / app.camera.zoom;
            float width = app.viewport.w / app.camera.zoom;
            float height = app.viewport.h / app.camera.zoom;
            if (x < app.camera.x + margin || y < app.camera.y + margin ||
                x + o->w > app.camera.x + width - margin ||
                y + o->h > app.camera.y + height - margin) {
                app.camera.x = x + o->w * .5f - width * .5f;
                app.camera.y = y + o->h * .5f - height * .5f;
            }
        }
        app.map->reveal_selected = false;
    }
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
            /* Draw after the map/grid so selection remains legible on matching sprite colors. */
            SDL_SetRenderClipRect(app.renderer, &map_clip);
            SDL_FRect outer = {rect.x - 4, rect.y - 4, rect.w + 8, rect.h + 8};
            editor_draw_color(UI_CANVAS);
            SDL_RenderRect(app.renderer, &outer);
            for (int i = 1; i <= 2; i++) {
                SDL_FRect line = {outer.x + i, outer.y + i, outer.w - 2 * i, outer.h - 2 * i};
                editor_draw_color(UI_ACCENT);
                SDL_RenderRect(app.renderer, &line);
            }
            float cx[] = {outer.x, outer.x + outer.w, outer.x, outer.x + outer.w};
            float cy[] = {outer.y, outer.y, outer.y + outer.h, outer.y + outer.h};
            for (int i = 0; i < 4; i++) {
                editor_box((SDL_FRect){cx[i] - 4, cy[i] - 4, 8, 8}, UI_CANVAS);
                editor_box((SDL_FRect){cx[i] - 2, cy[i] - 2, 4, 4}, UI_ACCENT);
            }
            char label[48];
            snprintf(label, sizeof label, "Entity #%u", o->stable_id);
            float label_width = ui_text_width(label) + 12;
            float lx = fmaxf(app.viewport.x + 4,
                             fminf(rect.x - 4, app.viewport.x + app.viewport.w - label_width - 4));
            float ly = fmaxf(app.viewport.y + 4, rect.y - 28);
            editor_box((SDL_FRect){lx, ly, label_width, 22}, UI_CANVAS);
            editor_text(lx + 6, ly + 8, label, UI_ACCENT);
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
