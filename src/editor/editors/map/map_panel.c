#include "app/editor_internal.h"

void editor_draw_map_panel(void) {
    float x = app.panel_area.x + 9;
    char label[128];
    bool enabled = !app.playing && !editor_edit_pending();
    float w = app.panel_area.w - 32, half = (w - 8) / 2;
    UiRect panel = {app.panel_area.x + 5, app.panel_top + 8, app.panel_area.w - 18,
                    fmaxf(0, app.panel_area.h - 16)};
    editor_box((SDL_FRect){panel.x, panel.y, panel.w, panel.h}, editor_widget_background());
    ui_scroll_clamp(&app.map_view);
    float y = app.panel_top + 18 - app.map_view.offset;
    ui_clip_push(&app.ui, panel);
    ui_apply_clip(app.renderer, &app.ui);
    editor_disclosure(x, y, w, "Layers", &app.layers_open);
    y += 32;
    if (app.layers_open) {
        snprintf(label, sizeof label, "Map: %dx%d  tile: %d", app.map->scene.width,
                 app.map->scene.height, app.map->scene.tile_size);
        editor_text(x, y, label, UI_TEXT);
        y += 22;
        for (int l = 0; l < app.map->scene.layer_count; l++) {
            app.widget_scope = ui_id("layer_row") ^ (UiId)(l + 1);
            snprintf(label, sizeof label, "%d %.20s", l + 1, app.map->scene.layers[l].name);
            if (button(x, y, w - 50, label, l == app.layer, enabled)) {
                app.layer = l;
            }
            if (icon_button(x + w - 44, y, 44,
                            app.map->scene.layers[l].visible ? ICON_EYE : ICON_EYE_OFF, "",
                            "Toggle layer visibility", false, enabled) &&
                editor_begin()) {
                app.map->scene.layers[l].visible = !app.map->scene.layers[l].visible;
                editor_commit();
            }
            y += 30;
        }
        app.widget_scope = 0;
        if (icon_button(x, y, half, ICON_PLUS, half >= 80 ? "Add" : "", "Add layer", false,
                        enabled && app.map->scene.layer_count < PE_LAYERS) &&
            editor_begin()) {
            app.layer = app.map->scene.layer_count++;
            memset(&app.map->scene.layers[app.layer], 0, sizeof(PeLayer));
            snprintf(app.map->scene.layers[app.layer].name, 32, "Layer %d", app.layer + 1);
            app.map->scene.layers[app.layer].visible = true;
            editor_commit();
        }
        if (icon_button(x + half + 8, y, half, ICON_DELETE, half >= 96 ? "Delete" : "",
                        "Delete layer", false, enabled && app.map->scene.layer_count > 1) &&
            editor_begin()) {
            memmove(&app.map->scene.layers[app.layer], &app.map->scene.layers[app.layer + 1],
                    (size_t)(app.map->scene.layer_count - app.layer - 1) * sizeof(PeLayer));
            app.map->scene.layer_count--;
            memset(&app.map->scene.layers[app.map->scene.layer_count], 0, sizeof(PeLayer));
            if (app.layer >= app.map->scene.layer_count) {
                app.layer--;
            }
            editor_commit();
        }
        y += 32;
        if (icon_button(x, y, half, ICON_EDIT, half >= 96 ? "Rename" : "", "Rename layer", false,
                        enabled)) {
            editor_field_start(FIELD_LAYER, app.map->scene.layers[app.layer].name);
        }
        if (icon_button(x + half + 8, y, (half - 8) / 2, ICON_UP, "", "Move layer up", false,
                        enabled && app.layer > 0) &&
            editor_begin()) {
            PeLayer temp = app.map->scene.layers[app.layer];
            app.map->scene.layers[app.layer] = app.map->scene.layers[app.layer - 1];
            app.map->scene.layers[--app.layer] = temp;
            editor_commit();
        }
        if (icon_button(x + half + 8 + (half + 8) / 2, y, (half - 8) / 2, ICON_DOWN, "",
                        "Move layer down", false,
                        enabled && app.layer + 1 < app.map->scene.layer_count) &&
            editor_begin()) {
            PeLayer temp = app.map->scene.layers[app.layer];
            app.map->scene.layers[app.layer] = app.map->scene.layers[app.layer + 1];
            app.map->scene.layers[++app.layer] = temp;
            editor_commit();
        }
        y += 40;
    }
    app.map_view.content = y + app.map_view.offset - app.panel_top + 8;
    app.map_view.viewport = fmaxf(0, app.panel_area.h - 16);
    ui_scroll_clamp(&app.map_view);
    ui_clip_pop(&app.ui);
    ui_apply_clip(app.renderer, &app.ui);
    UiRect track = {app.panel_area.x + app.panel_area.w - 15, app.panel_top + 8, 8,
                    app.map_view.viewport};
    ui_scrollbar(&app.ui, UI_MAP_SCROLL, track, &app.map_view, UI_AXIS_Y);
    ui_draw_scrollbar(app.renderer, &app.ui, UI_MAP_SCROLL, track, &app.map_view, UI_AXIS_Y);
}
