#include "app/editor_internal.h"

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
    char label[128];
    float w = app.panel_area.w - 32;
    UiRect panel = {(float)(app.panel_area.x + 5), app.panel_top + 8,
                    (float)(app.panel_area.w - 18), (float)(app.panel_area.h - 16)};
    ui_scroll_clamp(&app.inspector_view);
    float y = app.panel_top + 18 - app.inspector_view.offset;
    ui_clip_push(&app.ui, panel);
    ui_apply_clip(app.renderer, &app.ui);
    PeObject *o = pe_entity_get(&app.map->scene, pe_entity_find(&app.map->scene, app.selected));
    if (o) {
        editor_draw_input(FIELD_NAME, o->name, o->name, (UiRect){x, y - 6, w, 25},
                          !app.playing && !editor_edit_pending());
        snprintf(label, sizeof label, "ID: %u", o->stable_id);
        editor_text(x, y + 29, label, UI_MUTED);
        y += 45;
        if (!(o->mask & PE_TRANSFORM)) {
            editor_text(x, y, "No Transform: no map position", UI_MUTED);
            y += 22;
        }
        y = editor_draw_components(x, y + 4, w);
    } else {
        editor_text(x, y, "Select an entity in the view.", UI_MUTED);
        y += 28;
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
