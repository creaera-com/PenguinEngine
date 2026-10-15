#include "app/editor_internal.h"
const char *editor_screenshot_path;

static UiRect left_split(void);
static UiRect right_split(void);
static void layout_columns(void);

static UiRect left_split(void) {
    return (UiRect){LEFT - 3, TOP, 6, (float)(app.height - TOP - BOTTOM)};
}

static UiRect right_split(void) {
    return (UiRect){(float)(app.width - RIGHT - 3), TOP, 6, (float)(app.height - TOP - BOTTOM)};
}

static void layout_columns(void) {
    ui_columns_fit((float)app.width, &app.left_width, &app.right_width, app.min_left, app.min_right,
                   app.min_center);
    ui_drag_value(&app.ui, UI_SPLIT_LEFT, left_split(), &app.left_width, app.min_left,
                  app.width - app.right_width - app.min_center, UI_AXIS_X, 1);
    ui_drag_value(&app.ui, UI_SPLIT_RIGHT, right_split(), &app.right_width, app.min_right,
                  app.width - app.left_width - app.min_center, UI_AXIS_X, -1);
}

bool editor_ui_controls_hit(float x, float y) {
    if (ui_contains(left_split(), x, y) || ui_contains(right_split(), x, y)) {
        return true;
    }
    for (int i = 0; i < app.hit_count; i++) {
        if (ui_contains(app.hits[i], x, y)) {
            return true;
        }
    }

    return app.script && se_scroll_hit(app.script, x, y, app.viewport);
}

void editor_draw(void) {
    app.needs_layout = false;
    app.layout_resize_x = app.layout_resize_y = false;
    pe_console_begin_frame(app.console);
    app.rendering = true;
    SDL_GetWindowSize(app.window, &app.width, &app.height);
    app.hit_count = 0;
    app.canvas_enabled = false;
    app.ui.blocked = app.field != FIELD_NONE || app.request != REQUEST_NONE;
    if (!app.window_collapsed) {
        layout_columns();
    }
    editor_draw_color(UI_CANVAS);
    SDL_RenderClear(app.renderer);
    /* Inset panels expose the dark application background as narrow gutters. */
    editor_box(
        (SDL_FRect){PANEL_GAP, PANEL_GAP, (float)(app.width - 2 * PANEL_GAP), TOP - 2 * PANEL_GAP},
        UI_PANEL);
    bool modal = app.field != FIELD_NONE || app.request != REQUEST_NONE;
    bool saved_click = app.click;
    if (modal) {
        app.click = false;
    }
    app.ui.blocked = modal;
    editor_tooltip = NULL;
    editor_draw_global_tools();
    UiRect nav = {(float)app.global_nav_x, 12,
                  fmaxf(0, editor_window_controls_left() - app.global_nav_x - 12), 25};
    const cJSON *toolbar = pe_ui_get(app.shell_layout->root, "toolbar");
    const cJSON *selector = cJSON_GetArrayItem(toolbar, cJSON_GetArraySize(toolbar) - 1);
    bool labels = cJSON_IsTrue(pe_ui_get(selector, "labels"));
    for (int i = 0; i < app.workspaces.count; i++) {
        const EditorView *item = editor_view(app.workspaces.items[i].id);
        const cJSON *entry = editor_layout_entry(item->id);
        float width = labels
                          ? ui_tab_content_width(editor_yaml_icon(pe_ui_string(entry, "icon", "")),
                                                 pe_ui_string(entry, "title", item->label))
                          : 32;
        ui_tabs_set_width(&app.workspaces, item->id, width + app.tab_gap);
        app.workspaces.gap = (float)app.tab_gap;
        app.workspaces.items[i].enabled =
            !editor_edit_pending() && (item->id == UI_MAP_VIEW || !app.playing);
    }
    if (ui_tabs_update(&app.ui, &app.workspaces, nav, 112)) {
        editor_select_workspace(app.workspaces.selected);
    }
    ui_clip_push(&app.ui, nav);
    ui_apply_clip(app.renderer, &app.ui);
    for (int i = 0; i < app.workspaces.count; i++) {
        const EditorView *item = editor_view(app.workspaces.items[i].id);
        const cJSON *entry = editor_layout_entry(item->id);
        UiRect r = ui_tab_rect(&app.workspaces, nav, 112, i);
        editor_register_ui_hit(r);
        bool hover = ui_hovered(&app.ui, r);
        ui_draw_button(app.renderer, (SDL_FRect){r.x, r.y, r.w, r.h},
                       editor_yaml_icon(pe_ui_string(entry, "icon", "")),
                       labels ? pe_ui_string(entry, "title", item->label) : "",
                       app.workspaces.selected == item->id, app.workspaces.items[i].enabled, hover);
        if (hover) {
            editor_tooltip = pe_ui_string(entry, "title", item->label);
        }
    }
    ui_clip_pop(&app.ui);
    ui_apply_clip(app.renderer, &app.ui);
    editor_window_draw_controls();
    if (app.window_collapsed) {
        ui_end_frame(&app.ui);
        app.click = false;
        SDL_RenderPresent(app.renderer);
        editor_process_deferred_ui();
        return;
    }
    if (!app.active_layout || (!app.map && !app.script)) {
        editor_text(LEFT + 16, TOP + 40, app.status, UI_TEXT);
        ui_end_frame(&app.ui);
        if (modal) {
            app.ui.blocked = false;
            app.click = saved_click;
            editor_draw_modal();
        }
        SDL_RenderPresent(app.renderer);
        editor_process_deferred_ui();
        return;
    }

    const EditorView *view = editor_view(app.workspaces.selected);
    float center_width = (float)(app.width - LEFT - RIGHT - 2 * PANEL_GAP);
    int rows = editor_yaml_toolbar_rows(center_width);
    SDL_FRect tools = {LEFT + PANEL_GAP, TOP + PANEL_GAP, center_width, rows * 32.f + 6};
    app.viewport = (SDL_FRect){tools.x, tools.y + tools.h + PANEL_GAP, tools.w,
                               app.height - BOTTOM - PANEL_GAP - tools.y - tools.h - PANEL_GAP};
    editor_box(tools, UI_PANEL);
    editor_register_ui_hit((UiRect){tools.x, tools.y, tools.w, tools.h});
    ui_clip_push(&app.ui, (UiRect){tools.x, tools.y, tools.w, tools.h});
    ui_apply_clip(app.renderer, &app.ui);
    view->draw_tools(tools);
    ui_clip_pop(&app.ui);
    ui_apply_clip(app.renderer, &app.ui);
    const cJSON *columns = pe_ui_get(app.active_layout->root, "columns");
    editor_draw_yaml_node(pe_ui_get(columns, "center"), app.viewport, 1);
    editor_draw_yaml_node(pe_ui_get(columns, "left"),
                          (SDL_FRect){PANEL_GAP, TOP + PANEL_GAP, LEFT - 2 * PANEL_GAP,
                                      app.height - TOP - BOTTOM - 2 * PANEL_GAP},
                          0);
    editor_draw_yaml_node(pe_ui_get(columns, "right"),
                          (SDL_FRect){app.width - RIGHT + PANEL_GAP, TOP + PANEL_GAP,
                                      RIGHT - 2 * PANEL_GAP,
                                      app.height - TOP - BOTTOM - 2 * PANEL_GAP},
                          2);
    editor_box((SDL_FRect){0, (float)(app.height - BOTTOM), (float)app.width, BOTTOM}, UI_PANEL);
    editor_box((SDL_FRect){0, (float)(app.height - BOTTOM), (float)app.width, 1}, UI_BORDER);
    float status_x = 14, status_y = (float)(app.height - 18);
    const cJSON *fields = pe_ui_get(pe_ui_get(app.shell_layout->root, "statusbar"), "fields");
    ui_clip_push(&app.ui, (UiRect){0, (float)(app.height - BOTTOM), (float)app.width, BOTTOM});
    ui_apply_clip(app.renderer, &app.ui);
    for (const cJSON *field = fields->child; field; field = field->next) {
        char buffer[256];
        const char *value = editor_yaml_text(field, buffer, sizeof buffer);
        const char *binding = pe_ui_string(field, "binding", "");
        Uint32 color = !strcmp(binding, "editor.mode")      ? UI_ACCENT
                       : !strcmp(binding, "editor.message") ? UI_TEXT
                                                            : UI_MUTED;
        if (!strcmp(binding, "document.save_state") && !strcmp(value, "Modified")) {
            color = UI_ACCENT;
        }
        editor_text(status_x, status_y, value, color);
        status_x += ui_text_width(value) + 16;
        if (field->next) {
            editor_text(status_x, status_y, "|", UI_DISABLED);
            status_x += 16;
        }
    }
    ui_clip_pop(&app.ui);
    ui_apply_clip(app.renderer, &app.ui);
    if (editor_tooltip && !modal) {
        float width = (float)strlen(editor_tooltip) * 8 + 20;
        float tx = fminf(app.mx + 12, app.width - width - 8),
              ty = fminf(app.my + 30, app.height - 28.f);
        ui_panel(app.renderer, (SDL_FRect){tx, ty, width, 24}, UI_BORDER);
        editor_text(tx + 10, ty + 8, editor_tooltip, UI_TEXT);
    }

    UiRect ls = left_split(), rs = right_split();
    bool left_highlight = !modal && (ui_hovered(&app.ui, ls) || app.ui.active == UI_SPLIT_LEFT);
    bool right_highlight = !modal && (ui_hovered(&app.ui, rs) || app.ui.active == UI_SPLIT_RIGHT);
    editor_draw_split_grip(ls, left_highlight);
    editor_draw_split_grip(rs, right_highlight);
    bool resize = left_highlight || right_highlight;
    if (modal) {
        app.ui.blocked = false;
        app.click = saved_click;
        editor_draw_modal();
    }
    app.ui.blocked = false;
    if (app.resize_cursor) {
        SDL_SetCursor(app.layout_resize_y && app.vertical_cursor ? app.vertical_cursor
                      : (resize || app.layout_resize_x)          ? app.resize_cursor
                                                                 : SDL_GetDefaultCursor());
    }
    ui_end_frame(&app.ui);
    app.click = false;
    if (editor_screenshot_path) {
        SDL_Surface *capture = SDL_RenderReadPixels(app.renderer, NULL);
        if (!capture || !SDL_SaveBMP(capture, editor_screenshot_path)) {
            fprintf(stderr, "Screenshot failed: %s\n", SDL_GetError());
        }
        SDL_DestroySurface(capture);
    }
    SDL_RenderPresent(app.renderer);
    editor_process_deferred_ui();
}
