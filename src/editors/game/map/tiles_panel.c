#include "editors/game/map/internal.h"

void editor_draw_tiles_panel(void) {
    float x = app.shell.panel_area.x + 9;
    char label[128], value[64];
    bool enabled = !app.workspace.map->view.playing && !editor_edit_pending();
    float w = app.shell.panel_area.w - 32;
    UiRect panel = {app.shell.panel_area.x + 5, app.shell.panel_top + 8,
                    app.shell.panel_area.w - 18, fmaxf(0, app.shell.panel_area.h - 16)};
    editor_box((SDL_FRect){panel.x, panel.y, panel.w, panel.h}, editor_widget_background());
    ui_scroll_clamp(&app.workspace.map->view.tiles_view);
    float y = app.shell.panel_top + 18 - app.workspace.map->view.tiles_view.offset;
    ui_clip_push(&app.shell.ui, panel);
    ui_apply_clip(app.platform.renderer, &app.shell.ui);
    editor_section(x, y, ICON_IMAGE, "TILESET");
    y += 28;
    snprintf(label, sizeof label, "%.26s", app.workspace.map->scene.tileset);
    editor_text(x, y, label, UI_TEXT);
    y += 24;
    float import_width = fminf(w, ui_tab_content_width(ICON_IMPORT, "Import PNG"));
    if (icon_button(x + w - import_width, y, import_width, ICON_IMPORT, "Import PNG",
                    "Import or replace tileset", false, enabled)) {
        editor_field_start(FIELD_IMPORT, "");
    }
    y += 32;
    snprintf(label, sizeof label, "PNG tile: %d (change)",
             app.workspace.map->scene.tileset_tile_size);
    if (button(x, y, w, label, false, enabled)) {
        snprintf(value, sizeof value, "%d", app.workspace.map->scene.tileset_tile_size);
        editor_field_start(FIELD_TILESIZE, value);
    }
    y += 32;
    editor_disclosure(x, y, w, "Tile palette", &app.workspace.map->view.palette_open);
    y += 32;
    if (app.workspace.map->view.palette_open) {
        int total =
                app.workspace.map->assets.tileset
                    ? (app.workspace.map->assets.width / app.workspace.map->assets.source_tile) *
                          (app.workspace.map->assets.height / app.workspace.map->assets.source_tile)
                    : 0,
            columns = (int)(w / 37);
        if (columns < 1) {
            columns = 1;
        }
        for (int i = 0; i < total; i++) {
            int row = i / columns, col = i % columns;
            SDL_FRect dst = {x + col * 37, y + row * 37, 32, 32};
            if (dst.y + dst.h < panel.y || dst.y > panel.y + panel.h) {
                continue;
            }
            int source_columns =
                app.workspace.map->assets.width / app.workspace.map->assets.source_tile;
            SDL_FRect src = {(float)(i % source_columns * app.workspace.map->assets.source_tile),
                             (float)(i / source_columns * app.workspace.map->assets.source_tile),
                             (float)app.workspace.map->assets.source_tile,
                             (float)app.workspace.map->assets.source_tile};
            SDL_RenderTexture(app.platform.renderer, app.workspace.map->assets.tileset, &src, &dst);
            if (app.workspace.map->view.tile == i + 1) {
                editor_draw_color(UI_ACCENT);
                SDL_RenderRect(app.platform.renderer, &dst);
            }
            if (enabled && app.shell.click &&
                ui_hovered(&app.shell.ui, (UiRect){dst.x, dst.y, dst.w, dst.h})) {
                app.workspace.map->view.tile = i + 1;
                if (app.workspace.map->view.tool == SELECT ||
                    app.workspace.map->view.tool == ERASE) {
                    app.workspace.map->view.tool = BRUSH;
                }
            }
        }
        y += ((total + columns - 1) / columns) * 37;
    }
    app.workspace.map->view.tiles_view.content =
        y + app.workspace.map->view.tiles_view.offset - app.shell.panel_top + 8;
    app.workspace.map->view.tiles_view.viewport = fmaxf(0, app.shell.panel_area.h - 16);
    ui_scroll_clamp(&app.workspace.map->view.tiles_view);
    ui_clip_pop(&app.shell.ui);
    ui_apply_clip(app.platform.renderer, &app.shell.ui);
    UiRect track = {app.shell.panel_area.x + app.shell.panel_area.w - 15, app.shell.panel_top + 8,
                    8, app.workspace.map->view.tiles_view.viewport};
    ui_scrollbar(&app.shell.ui, UI_TILES_SCROLL, track, &app.workspace.map->view.tiles_view,
                 UI_AXIS_Y);
    ui_draw_scrollbar(app.platform.renderer, &app.shell.ui, UI_TILES_SCROLL, track,
                      &app.workspace.map->view.tiles_view, UI_AXIS_Y);
}
