#include "editors/game/map/internal.h"

void editor_draw_info(void) {
    float x = (float)(app.shell.panel_area.x + 9);
    app.shell.info_view.content = 340;
    app.shell.info_view.viewport = (float)(app.shell.panel_area.h - 16);
    ui_scroll_clamp(&app.shell.info_view);
    UiRect track = {(float)(app.shell.panel_area.x + app.shell.panel_area.w - 15),
                    app.shell.panel_top + 8, 8, app.shell.info_view.viewport};
    ui_scrollbar(&app.shell.ui, UI_INFO_SCROLL, track, &app.shell.info_view, UI_AXIS_Y);
    ui_clip_push(&app.shell.ui,
                 (UiRect){x - 4, app.shell.panel_top + 8, app.shell.panel_area.w - 18,
                          app.shell.info_view.viewport});
    ui_apply_clip(app.platform.renderer, &app.shell.ui);
    float y = app.shell.panel_top + 18 - app.shell.info_view.offset;
    editor_section(x, y, ICON_FILE, "SCENE INFO");
    char label[64];
    snprintf(label, sizeof label, "Map: %d x %d tiles", pe_map_active_instance()->scene.width,
             pe_map_active_instance()->scene.height);
    editor_text(x, y + 32, label, UI_TEXT);
    snprintf(label, sizeof label, "Tile size: %d px", pe_map_active_instance()->scene.tile_size);
    editor_text(x, y + 56, label, UI_TEXT);
    snprintf(label, sizeof label, "Layers: %d", pe_map_active_instance()->scene.layer_count);
    editor_text(x, y + 80, label, UI_TEXT);
    int count = 0;
    for (int i = 0; i < PE_ENTITIES; i++) {
        if (pe_map_active_instance()->scene.objects[i].alive) {
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
    ui_clip_pop(&app.shell.ui);
    ui_apply_clip(app.platform.renderer, &app.shell.ui);
    ui_draw_scrollbar(app.platform.renderer, &app.shell.ui, UI_INFO_SCROLL, track,
                      &app.shell.info_view, UI_AXIS_Y);
}

void editor_draw_inspector(void) {
    float x = (float)(app.shell.panel_area.x + 9);
    char label[128];
    float w = app.shell.panel_area.w - 32;
    UiRect panel = {(float)(app.shell.panel_area.x + 5), app.shell.panel_top + 8,
                    (float)(app.shell.panel_area.w - 18), (float)(app.shell.panel_area.h - 16)};
    ui_scroll_clamp(&app.shell.inspector_view);
    float y = app.shell.panel_top + 18 - app.shell.inspector_view.offset;
    ui_clip_push(&app.shell.ui, panel);
    ui_apply_clip(app.platform.renderer, &app.shell.ui);
    PeObject *o =
        pe_entity_get(&pe_map_active_instance()->scene,
                      pe_entity_find(&pe_map_active_instance()->scene, app.shell.selected));
    if (o) {
        snprintf(label, sizeof label, "ID: %u", o->stable_id);
        editor_text(x, y, label, UI_MUTED);
        editor_draw_input(FIELD_NAME, o->name, o->name, (UiRect){x, y + 14, w, 25},
                          !pe_map_active_instance()->view.playing && !editor_edit_pending());
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
    app.shell.inspector_view.content =
        y + app.shell.inspector_view.offset - app.shell.panel_top + 8;
    app.shell.inspector_view.viewport = (float)(app.shell.panel_area.h - 16);
    ui_scroll_clamp(&app.shell.inspector_view);
    ui_clip_pop(&app.shell.ui);
    ui_apply_clip(app.platform.renderer, &app.shell.ui);
    UiRect track = {(float)(app.shell.panel_area.x + app.shell.panel_area.w - 15),
                    app.shell.panel_top + 8, 8, app.shell.inspector_view.viewport};
    ui_scrollbar(&app.shell.ui, UI_INSPECTOR_SCROLL, track, &app.shell.inspector_view, UI_AXIS_Y);
    ui_draw_scrollbar(app.platform.renderer, &app.shell.ui, UI_INSPECTOR_SCROLL, track,
                      &app.shell.inspector_view, UI_AXIS_Y);
}
