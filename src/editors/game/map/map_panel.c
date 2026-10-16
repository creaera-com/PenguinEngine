#include "editors/game/map/internal.h"

void editor_draw_map_panel(void) {
    float x = app.shell.panel_area.x + 9;
    char label[128];
    bool enabled = !app.workspace.map->view.playing && !editor_edit_pending();
    float w = app.shell.panel_area.w - 32, half = (w - 8) / 2;
    float add_width = fminf(half, ui_tab_content_width(ICON_PLUS, "Add"));
    float delete_width = fminf(half, ui_tab_content_width(ICON_DELETE, "Delete"));
    float rename_width = fminf(half, ui_tab_content_width(ICON_EDIT, "Rename"));
    float arrow_width = fminf(32, (w - rename_width - 16) / 2);
    float actions_x = x + w - add_width - 8 - delete_width;
    float order_x = x + w - rename_width - 16 - 2 * arrow_width;
    UiRect panel = {app.shell.panel_area.x + 5, app.shell.panel_top + 8,
                    app.shell.panel_area.w - 18, fmaxf(0, app.shell.panel_area.h - 16)};
    editor_box((SDL_FRect){panel.x, panel.y, panel.w, panel.h}, editor_widget_background());
    ui_scroll_clamp(&app.workspace.map->view.map_view);
    float y = app.shell.panel_top + 18 - app.workspace.map->view.map_view.offset;
    ui_clip_push(&app.shell.ui, panel);
    ui_apply_clip(app.platform.renderer, &app.shell.ui);
    editor_disclosure(x, y, w, "Layers", &app.workspace.map->view.layers_open);
    y += 32;
    if (app.workspace.map->view.layers_open) {
        snprintf(label, sizeof label, "Map: %dx%d  tile: %d", app.workspace.map->scene.width,
                 app.workspace.map->scene.height, app.workspace.map->scene.tile_size);
        editor_text(x, y, label, UI_TEXT);
        y += 22;
        for (int l = 0; l < app.workspace.map->scene.layer_count; l++) {
            app.shell.widget_scope = ui_id("layer_row") ^ (UiId)(l + 1);
            snprintf(label, sizeof label, "%d %.20s", l + 1,
                     app.workspace.map->scene.layers[l].name);
            if (button(x, y, w - 50, label, l == app.workspace.map->view.layer, enabled)) {
                app.workspace.map->view.layer = l;
            }
            if (icon_button(x + w - 44, y, 44,
                            app.workspace.map->scene.layers[l].visible ? ICON_EYE : ICON_EYE_OFF,
                            "", "Toggle layer visibility", false, enabled) &&
                editor_begin()) {
                app.workspace.map->scene.layers[l].visible =
                    !app.workspace.map->scene.layers[l].visible;
                editor_commit();
            }
            y += 30;
        }
        app.shell.widget_scope = 0;
        if (icon_button(actions_x, y, add_width, ICON_PLUS, "Add", "Add layer", false,
                        enabled && app.workspace.map->scene.layer_count < PE_LAYERS) &&
            editor_begin()) {
            app.workspace.map->view.layer = app.workspace.map->scene.layer_count++;
            memset(&app.workspace.map->scene.layers[app.workspace.map->view.layer], 0,
                   sizeof(PeLayer));
            snprintf(app.workspace.map->scene.layers[app.workspace.map->view.layer].name, 32,
                     "Layer %d", app.workspace.map->view.layer + 1);
            app.workspace.map->scene.layers[app.workspace.map->view.layer].visible = true;
            editor_commit();
        }
        if (icon_button(x + w - delete_width, y, delete_width, ICON_DELETE,
                        delete_width >= ui_tab_content_width(ICON_DELETE, "Delete") ? "Delete" : "",
                        "Delete layer", false,
                        enabled && app.workspace.map->scene.layer_count > 1) &&
            editor_begin()) {
            memmove(
                &app.workspace.map->scene.layers[app.workspace.map->view.layer],
                &app.workspace.map->scene.layers[app.workspace.map->view.layer + 1],
                (size_t)(app.workspace.map->scene.layer_count - app.workspace.map->view.layer - 1) *
                    sizeof(PeLayer));
            app.workspace.map->scene.layer_count--;
            memset(&app.workspace.map->scene.layers[app.workspace.map->scene.layer_count], 0,
                   sizeof(PeLayer));
            if (app.workspace.map->view.layer >= app.workspace.map->scene.layer_count) {
                app.workspace.map->view.layer--;
            }
            editor_commit();
        }
        y += 32;
        if (icon_button(order_x, y, rename_width, ICON_EDIT,
                        rename_width >= ui_tab_content_width(ICON_EDIT, "Rename") ? "Rename" : "",
                        "Rename layer", false, enabled)) {
            editor_field_start(FIELD_LAYER,
                               app.workspace.map->scene.layers[app.workspace.map->view.layer].name);
        }
        if (icon_button(x + w - 2 * arrow_width - 8, y, arrow_width, ICON_UP, "", "Move layer up",
                        false, enabled && app.workspace.map->view.layer > 0) &&
            editor_begin()) {
            PeLayer temp = app.workspace.map->scene.layers[app.workspace.map->view.layer];
            app.workspace.map->scene.layers[app.workspace.map->view.layer] =
                app.workspace.map->scene.layers[app.workspace.map->view.layer - 1];
            app.workspace.map->scene.layers[--app.workspace.map->view.layer] = temp;
            editor_commit();
        }
        if (icon_button(x + w - arrow_width, y, arrow_width, ICON_DOWN, "", "Move layer down",
                        false,
                        enabled && app.workspace.map->view.layer + 1 <
                                       app.workspace.map->scene.layer_count) &&
            editor_begin()) {
            PeLayer temp = app.workspace.map->scene.layers[app.workspace.map->view.layer];
            app.workspace.map->scene.layers[app.workspace.map->view.layer] =
                app.workspace.map->scene.layers[app.workspace.map->view.layer + 1];
            app.workspace.map->scene.layers[++app.workspace.map->view.layer] = temp;
            editor_commit();
        }
        y += 40;
    }
    app.workspace.map->view.map_view.content =
        y + app.workspace.map->view.map_view.offset - app.shell.panel_top + 8;
    app.workspace.map->view.map_view.viewport = fmaxf(0, app.shell.panel_area.h - 16);
    ui_scroll_clamp(&app.workspace.map->view.map_view);
    ui_clip_pop(&app.shell.ui);
    ui_apply_clip(app.platform.renderer, &app.shell.ui);
    UiRect track = {app.shell.panel_area.x + app.shell.panel_area.w - 15, app.shell.panel_top + 8,
                    8, app.workspace.map->view.map_view.viewport};
    ui_scrollbar(&app.shell.ui, UI_MAP_SCROLL, track, &app.workspace.map->view.map_view, UI_AXIS_Y);
    ui_draw_scrollbar(app.platform.renderer, &app.shell.ui, UI_MAP_SCROLL, track,
                      &app.workspace.map->view.map_view, UI_AXIS_Y);
}
