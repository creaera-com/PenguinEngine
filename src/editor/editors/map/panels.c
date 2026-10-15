#include "app/editor_internal.h"

void editor_draw_scene_tree(void) {
    editor_section(app.panel_area.x + 11, app.panel_top + 18, ICON_ENTITY, "SCENE ENTITIES");
    int count = 0;
    for (int i = 0; i < PE_ENTITIES; i++) {
        if (app.map->scene.objects[i].alive) {
            count++;
        }
    }
    app.scene_view.content = count ? count * 26.f : 26;
    app.scene_view.viewport = (float)(app.panel_area.h - 46);
    ui_scroll_clamp(&app.scene_view);
    UiRect track = {app.panel_area.x + app.panel_area.w - 15, app.panel_top + 46, 8,
                    app.scene_view.viewport};
    ui_scrollbar(&app.ui, UI_SCENE_SCROLL, track, &app.scene_view, UI_AXIS_Y);
    ui_clip_push(&app.ui, (UiRect){app.panel_area.x, app.panel_top + 46, app.panel_area.w - 18,
                                   app.scene_view.viewport});
    ui_apply_clip(app.renderer, &app.ui);
    float y = app.panel_top + 46 - app.scene_view.offset;
    if (!count) {
        editor_text(app.panel_area.x + 11, y + 8, "No entities in scene.", UI_MUTED);
    }
    for (int i = 0; i < PE_ENTITIES; i++) {
        const PeObject *o = &app.map->scene.objects[i];
        if (!o->alive) {
            continue;
        }
        char label[40];
        snprintf(label, sizeof label, "%s %u", o->mask & PE_PLAYER ? "Player" : "Entity",
                 o->stable_id);
        app.widget_scope = ui_id("scene_entity") ^ o->stable_id;
        if (icon_button(app.panel_area.x + 9, y, app.panel_area.w - 32, ICON_ENTITY, label,
                        "Select entity", app.selected == o->stable_id,
                        (app.map || app.script) && !app.playing && !editor_edit_pending())) {
            app.selected = o->stable_id;
            app.tool = SELECT;
            app.right_tabs.selected = UI_INSPECTOR_TAB;
            editor_tab_select(false);
        }
        y += 26;
    }
    app.widget_scope = 0;
    ui_clip_pop(&app.ui);
    ui_apply_clip(app.renderer, &app.ui);
    ui_draw_scrollbar(app.renderer, &app.ui, UI_SCENE_SCROLL, track, &app.scene_view, UI_AXIS_Y);
}

void editor_draw_info(void) {
    float x = (float)(app.panel_area.x + 9);
    app.info_view.content = 340;
    app.info_view.viewport = (float)(app.panel_area.h - 16);
    ui_scroll_clamp(&app.info_view);
    UiRect track = {(float)(app.panel_area.x + app.panel_area.w - 15), app.panel_top + 8, 8,
                    app.info_view.viewport};
    ui_scrollbar(&app.ui, UI_INFO_SCROLL, track, &app.info_view, UI_AXIS_Y);
    ui_clip_push(&app.ui,
                 (UiRect){x - 4, app.panel_top + 8, app.panel_area.w - 18, app.info_view.viewport});
    ui_apply_clip(app.renderer, &app.ui);
    float y = app.panel_top + 18 - app.info_view.offset;
    editor_section(x, y, ICON_FILE, "SCENE INFO");
    char label[64];
    snprintf(label, sizeof label, "Map: %d x %d tiles", app.map->scene.width,
             app.map->scene.height);
    editor_text(x, y + 32, label, UI_TEXT);
    snprintf(label, sizeof label, "Tile size: %d px", app.map->scene.tile_size);
    editor_text(x, y + 56, label, UI_TEXT);
    snprintf(label, sizeof label, "Layers: %d", app.map->scene.layer_count);
    editor_text(x, y + 80, label, UI_TEXT);
    int count = 0;
    for (int i = 0; i < PE_ENTITIES; i++) {
        if (app.map->scene.objects[i].alive) {
            count++;
        }
    }
    snprintf(label, sizeof label, "Entities: %d / %d", count, PE_ENTITIES);
    editor_text(x, y + 104, label, UI_TEXT);
    editor_section(x, y + 148, ICON_EDIT, "SHORTCUTS");
    const char *help[] = {"Ctrl+S  Save",    "Ctrl+Z  Undo",     "F5      Play / Stop",
                          "MMB     Pan map", "Wheel   Zoom map", "B/E/R/F/V  Map tools"};
    for (int i = 0; i < 6; i++) {
        editor_text(x, y + 180 + i * 24, help[i], UI_MUTED);
    }
    ui_clip_pop(&app.ui);
    ui_apply_clip(app.renderer, &app.ui);
    ui_draw_scrollbar(app.renderer, &app.ui, UI_INFO_SCROLL, track, &app.info_view, UI_AXIS_Y);
}

void editor_draw_inspector(void) {
    float x = (float)(app.panel_area.x + 9);
    char label[128], value[64];
    bool enabled = !app.playing;
    float w = app.panel_area.w - 32, half = (w - 8) / 2;
    UiRect panel = {(float)(app.panel_area.x + 5), app.panel_top + 8,
                    (float)(app.panel_area.w - 18), (float)(app.panel_area.h - 16)};
    ui_scroll_clamp(&app.inspector_view);
    float y = app.panel_top + 18 - app.inspector_view.offset;
    ui_clip_push(&app.ui, panel);
    ui_apply_clip(app.renderer, &app.ui);
    editor_disclosure(x, y, w, "Tilemap / Layers", &app.layers_open);
    y += 32;
    if (app.layers_open) {
        snprintf(label, sizeof label, "Map: %dx%d  tile: %d", app.map->scene.width,
                 app.map->scene.height, app.map->scene.tile_size);
        editor_text(x, y, label, UI_TEXT);
        y += 22;
        snprintf(label, sizeof label, "PNG tile: %d (change)", app.map->scene.tileset_tile_size);
        if (button(x, y, w, label, false, enabled)) {
            snprintf(value, sizeof value, "%d", app.map->scene.tileset_tile_size);
            editor_field_start(FIELD_TILESIZE, value);
        }
        y += 32;
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
        if (icon_button(x, y, half, ICON_PLUS, "Add layer", NULL, false,
                        enabled && app.map->scene.layer_count < PE_LAYERS) &&
            editor_begin()) {
            app.layer = app.map->scene.layer_count++;
            memset(&app.map->scene.layers[app.layer], 0, sizeof(PeLayer));
            snprintf(app.map->scene.layers[app.layer].name, 32, "Layer %d", app.layer + 1);
            app.map->scene.layers[app.layer].visible = true;
            editor_commit();
        }
        if (icon_button(x + half + 8, y, half, ICON_DELETE, "Delete", "Delete layer", false,
                        enabled && app.map->scene.layer_count > 1) &&
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
        if (icon_button(x, y, half, ICON_EDIT, "Rename", NULL, false, enabled)) {
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
    editor_disclosure(x, y, w, "Entity", &app.entity_open);
    y += 32;
    if (app.entity_open) {
        if (icon_button(x, y, half, ICON_PLUS, "New", "Create entity", false, enabled) &&
            editor_begin()) {
            PeEntity e = pe_entity_create(&app.map->scene, PE_TRANSFORM | PE_SPRITE | PE_BODY);
            PeObject *o = pe_entity_get(&app.map->scene, e);
            if (o) {
                o->x = 64;
                o->y = 64;
                app.selected = o->stable_id;
                app.tool = SELECT;
            } else {
                editor_message("Entity limit reached");
            }
            editor_commit();
        }
        PeObject *o = pe_entity_get(&app.map->scene, pe_entity_find(&app.map->scene, app.selected));
        if (icon_button(x + half + 8, y, half, ICON_DELETE, "Delete", "Delete entity", false,
                        enabled && o) &&
            editor_begin()) {
            pe_entity_destroy(&app.map->scene, pe_entity_find(&app.map->scene, app.selected));
            app.selected = 0;
            editor_commit();
            o = NULL;
        }
        y += 34;
        if (o) {
            snprintf(label, sizeof label, "Entity #%u", o->stable_id);
            editor_text(x, y, label, UI_TEXT);
            y += 20;
            float values[] = {o->x, o->y, o->w, o->h};
            const char *names[] = {"X", "Y", "Width", "Height"};
            Field fields[] = {FIELD_X, FIELD_Y, FIELD_W, FIELD_H};
            for (int i = 0; i < 4; i++) {
                app.widget_scope = ui_id("entity_field") ^ o->stable_id * 31u ^ (UiId)i;
                snprintf(label, sizeof label, "%s: %.2f", names[i], values[i]);
                if (button(x, y, w, label, false, enabled)) {
                    snprintf(value, sizeof value, "%.2f", values[i]);
                    editor_field_start(fields[i], value);
                }
                y += 29;
            }
            app.widget_scope = 0;
            if (button(x, y, w, o->mask & PE_PLAYER ? "Player control: ON" : "Player control: OFF",
                       false, enabled) &&
                editor_begin()) {
                o->mask ^= PE_PLAYER;
                editor_commit();
            }
            y += 35;
        } else {
            editor_text(x, y, "Select an entity in the view.", UI_MUTED);
            y += 28;
        }
    }
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
    app.inspector_view.content = y + app.inspector_view.offset - app.panel_top + 8;
    app.inspector_view.viewport = (float)(app.panel_area.h - 16);
    ui_scroll_clamp(&app.inspector_view);
    ui_clip_pop(&app.ui);
    ui_apply_clip(app.renderer, &app.ui);
    UiRect track = {(float)(app.panel_area.x + app.panel_area.w - 15), app.panel_top + 8, 8,
                    app.inspector_view.viewport};
    ui_scrollbar(&app.ui, UI_INSPECTOR_SCROLL, track, &app.inspector_view, UI_AXIS_Y);
    ui_draw_scrollbar(app.renderer, &app.ui, UI_INSPECTOR_SCROLL, track, &app.inspector_view,
                      UI_AXIS_Y);
}
