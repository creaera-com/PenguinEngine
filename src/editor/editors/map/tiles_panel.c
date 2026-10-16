#include "app/editor_internal.h"

void editor_draw_tiles_panel(void) {
    float x = app.panel_area.x + 9;
    char label[128], value[64];
    bool enabled = !app.playing && !editor_edit_pending();
    float w = app.panel_area.w - 32;
    UiRect panel = {app.panel_area.x + 5, app.panel_top + 8, app.panel_area.w - 18,
                    fmaxf(0, app.panel_area.h - 16)};
    editor_box((SDL_FRect){panel.x, panel.y, panel.w, panel.h}, editor_widget_background());
    ui_scroll_clamp(&app.tiles_view);
    float y = app.panel_top + 18 - app.tiles_view.offset;
    ui_clip_push(&app.ui, panel);
    ui_apply_clip(app.renderer, &app.ui);
    editor_section(x, y, ICON_IMAGE, "TILESET");
    y += 28;
    snprintf(label, sizeof label, "%.26s", app.map->scene.tileset);
    editor_text(x, y, label, UI_TEXT);
    y += 24;
    float import_width = fminf(w, ui_tab_content_width(ICON_IMPORT, "Import PNG"));
    if (icon_button(x + w - import_width, y, import_width, ICON_IMPORT, "Import PNG",
                    "Import or replace tileset", false, enabled)) {
        editor_field_start(FIELD_IMPORT, "");
    }
    y += 32;
    snprintf(label, sizeof label, "PNG tile: %d (change)", app.map->scene.tileset_tile_size);
    if (button(x, y, w, label, false, enabled)) {
        snprintf(value, sizeof value, "%d", app.map->scene.tileset_tile_size);
        editor_field_start(FIELD_TILESIZE, value);
    }
    y += 32;
    editor_disclosure(x, y, w, "Tile palette", &app.palette_open);
    y += 32;
    if (app.palette_open) {
        int total = app.map->assets.tileset
                        ? (app.map->assets.width / app.map->assets.source_tile) *
                              (app.map->assets.height / app.map->assets.source_tile)
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
            int source_columns = app.map->assets.width / app.map->assets.source_tile;
            SDL_FRect src = {(float)(i % source_columns * app.map->assets.source_tile),
                             (float)(i / source_columns * app.map->assets.source_tile),
                             (float)app.map->assets.source_tile,
                             (float)app.map->assets.source_tile};
            SDL_RenderTexture(app.renderer, app.map->assets.tileset, &src, &dst);
            if (app.tile == i + 1) {
                editor_draw_color(UI_ACCENT);
                SDL_RenderRect(app.renderer, &dst);
            }
            if (enabled && app.click && ui_hovered(&app.ui, (UiRect){dst.x, dst.y, dst.w, dst.h})) {
                app.tile = i + 1;
                if (app.tool == SELECT || app.tool == ERASE) {
                    app.tool = BRUSH;
                }
            }
        }
        y += ((total + columns - 1) / columns) * 37;
    }
    app.tiles_view.content = y + app.tiles_view.offset - app.panel_top + 8;
    app.tiles_view.viewport = fmaxf(0, app.panel_area.h - 16);
    ui_scroll_clamp(&app.tiles_view);
    ui_clip_pop(&app.ui);
    ui_apply_clip(app.renderer, &app.ui);
    UiRect track = {app.panel_area.x + app.panel_area.w - 15, app.panel_top + 8, 8,
                    app.tiles_view.viewport};
    ui_scrollbar(&app.ui, UI_TILES_SCROLL, track, &app.tiles_view, UI_AXIS_Y);
    ui_draw_scrollbar(app.renderer, &app.ui, UI_TILES_SCROLL, track, &app.tiles_view, UI_AXIS_Y);
}
