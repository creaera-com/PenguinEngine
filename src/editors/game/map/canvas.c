#include "editors/game/map/internal.h"

void editor_draw_map_tools(SDL_FRect area) {
    editor_draw_yaml_toolbar(area);
}

void editor_draw_map_content(void) {
    bool modal = app.shell.field != FIELD_NONE || app.shell.request != REQUEST_NONE;
    /* Distinguish the workspace from empty/transparent tiles inside the map. */
    editor_box(app.shell.viewport, 0x383838ffu);
    const PeScene *selected_scene =
        app.workspace.map->view.playing ? &app.workspace.map->runtime : &app.workspace.map->scene;
    if (app.workspace.map->reveal_selected && !app.workspace.map->view.playing) {
        float x, y;
        PeEntity selected = pe_entity_find(selected_scene, app.shell.selected);
        const PeObject *o = pe_entity_get(&app.workspace.map->scene, selected);
        if (o && pe_entity_world_position(selected_scene, selected, &x, &y)) {
            float margin = 20 / app.workspace.map->view.camera.zoom;
            float width = app.shell.viewport.w / app.workspace.map->view.camera.zoom;
            float height = app.shell.viewport.h / app.workspace.map->view.camera.zoom;
            if (x < app.workspace.map->view.camera.x + margin ||
                y < app.workspace.map->view.camera.y + margin ||
                x + o->w > app.workspace.map->view.camera.x + width - margin ||
                y + o->h > app.workspace.map->view.camera.y + height - margin) {
                app.workspace.map->view.camera.x = x + o->w * .5f - width * .5f;
                app.workspace.map->view.camera.y = y + o->h * .5f - height * .5f;
            }
        }
        app.workspace.map->reveal_selected = false;
    }
    SDL_Rect map_clip = {(int)app.shell.viewport.x, (int)app.shell.viewport.y,
                         (int)app.shell.viewport.w, (int)app.shell.viewport.h};
    SDL_SetRenderClipRect(app.platform.renderer, &map_clip);
    const PeScene *visible_scene =
        app.workspace.map->view.playing ? &app.workspace.map->runtime : &app.workspace.map->scene;
    editor_box(
        (SDL_FRect){
            app.shell.viewport.x -
                app.workspace.map->view.camera.x * app.workspace.map->view.camera.zoom,
            app.shell.viewport.y -
                app.workspace.map->view.camera.y * app.workspace.map->view.camera.zoom,
            visible_scene->width * visible_scene->tile_size * app.workspace.map->view.camera.zoom,
            visible_scene->height * visible_scene->tile_size * app.workspace.map->view.camera.zoom},
        UI_CANVAS);
    SDL_SetRenderClipRect(app.platform.renderer, NULL);
    pe_render_scene(app.platform.renderer,
                    app.workspace.map->view.playing ? &app.workspace.map->runtime
                                                    : &app.workspace.map->scene,
                    &app.workspace.map->assets, &app.workspace.map->view.camera, app.shell.viewport,
                    app.workspace.map->view.grid && !app.workspace.map->view.playing,
                    app.workspace.map->view.collision && !app.workspace.map->view.playing);
    if (!app.workspace.map->view.playing) {
        PeObject *o = pe_entity_get(&app.workspace.map->scene,
                                    pe_entity_find(&app.workspace.map->scene, app.shell.selected));
        float x, y;
        if (o && pe_entity_world_position(
                     &app.workspace.map->scene,
                     pe_entity_find(&app.workspace.map->scene, app.shell.selected), &x, &y)) {
            SDL_FRect rect = {app.shell.viewport.x + (x - app.workspace.map->view.camera.x) *
                                                         app.workspace.map->view.camera.zoom,
                              app.shell.viewport.y + (y - app.workspace.map->view.camera.y) *
                                                         app.workspace.map->view.camera.zoom,
                              o->w * app.workspace.map->view.camera.zoom,
                              o->h * app.workspace.map->view.camera.zoom};
            /* Draw after the map/grid so selection remains legible on matching sprite colors. */
            SDL_SetRenderClipRect(app.platform.renderer, &map_clip);
            SDL_FRect outer = {rect.x - 4, rect.y - 4, rect.w + 8, rect.h + 8};
            editor_draw_color(UI_CANVAS);
            SDL_RenderRect(app.platform.renderer, &outer);
            for (int i = 1; i <= 2; i++) {
                SDL_FRect line = {outer.x + i, outer.y + i, outer.w - 2 * i, outer.h - 2 * i};
                editor_draw_color(UI_ACCENT);
                SDL_RenderRect(app.platform.renderer, &line);
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
            float lx = fmaxf(
                app.shell.viewport.x + 4,
                fminf(rect.x - 4, app.shell.viewport.x + app.shell.viewport.w - label_width - 4));
            float ly = fmaxf(app.shell.viewport.y + 4, rect.y - 28);
            editor_box((SDL_FRect){lx, ly, label_width, 22}, UI_CANVAS);
            editor_text(lx + 6, ly + 8, label, UI_ACCENT);
        }
        if (editor_inside(app.shell.mx, app.shell.my, app.shell.viewport) && !modal &&
            app.workspace.map->view.tool != SELECT) {
            int x, y;
            editor_tile_coords(app.shell.mx, app.shell.my, &x, &y);
            int sx = app.workspace.map->view.painting && app.workspace.map->view.tool == RECTANGLE
                         ? app.workspace.map->view.start_x
                         : x,
                sy = app.workspace.map->view.painting && app.workspace.map->view.tool == RECTANGLE
                         ? app.workspace.map->view.start_y
                         : y;
            int minx = sx < x ? sx : x, miny = sy < y ? sy : y;
            SDL_FRect rect = {app.shell.viewport.x + (minx * app.workspace.map->scene.tile_size -
                                                      app.workspace.map->view.camera.x) *
                                                         app.workspace.map->view.camera.zoom,
                              app.shell.viewport.y + (miny * app.workspace.map->scene.tile_size -
                                                      app.workspace.map->view.camera.y) *
                                                         app.workspace.map->view.camera.zoom,
                              (abs(x - sx) + 1) * app.workspace.map->scene.tile_size *
                                  app.workspace.map->view.camera.zoom,
                              (abs(y - sy) + 1) * app.workspace.map->scene.tile_size *
                                  app.workspace.map->view.camera.zoom};
            SDL_Rect clip = {(int)app.shell.viewport.x, (int)app.shell.viewport.y,
                             (int)app.shell.viewport.w, (int)app.shell.viewport.h};
            SDL_SetRenderClipRect(app.platform.renderer, &clip);
            editor_draw_color(UI_ACCENT);
            SDL_RenderRect(app.platform.renderer, &rect);
            SDL_SetRenderClipRect(app.platform.renderer, NULL);
        }
    }
}
