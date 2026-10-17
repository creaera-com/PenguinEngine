#include "application/internal.h"

const char *editor_screenshot_path;

static UiRect left_split(void);
static UiRect right_split(void);
static void layout_columns(void);

static UiRect left_split(void) {
    return (UiRect){LEFT - 3, TOP, 6, (float)(app.platform.height - TOP - BOTTOM)};
}

static UiRect right_split(void) {
    return (UiRect){(float)(app.platform.width - RIGHT - 3), TOP, 6,
                    (float)(app.platform.height - TOP - BOTTOM)};
}

static void layout_columns(void) {
    ui_columns_fit((float)app.platform.width, &app.shell.left_width, &app.shell.right_width,
                   app.shell.min_left, app.shell.min_right, app.shell.min_center);
    ui_drag_value(&app.shell.ui, UI_SPLIT_LEFT, left_split(), &app.shell.left_width,
                  app.shell.min_left,
                  app.platform.width - app.shell.right_width - app.shell.min_center, UI_AXIS_X, 1);
    ui_drag_value(&app.shell.ui, UI_SPLIT_RIGHT, right_split(), &app.shell.right_width,
                  app.shell.min_right,
                  app.platform.width - app.shell.left_width - app.shell.min_center, UI_AXIS_X, -1);
}

bool editor_ui_controls_hit(float x, float y) {
    if (ui_contains(left_split(), x, y) || ui_contains(right_split(), x, y)) {
        return true;
    }
    for (int i = 0; i < app.shell.hit_count; i++) {
        if (ui_contains(app.shell.hits[i], x, y)) {
            return true;
        }
    }

    return app.workspace.backend && app.workspace.backend->controls_hit &&
           app.workspace.backend->controls_hit(x, y);
}

void editor_draw(void) {
    app.shell.needs_layout = false;
    app.shell.layout_resize_x = app.shell.layout_resize_y = false;
    pe_console_begin_frame(app.shell.console);
    app.shell.rendering = true;
    SDL_GetWindowSize(app.platform.window, &app.platform.width, &app.platform.height);
    app.shell.hit_count = 0;
    app.shell.field_seen = false;
    app.shell.canvas_enabled = false;
    const PeEditorBackend *backend = app.workspace.backend;
    bool popup = backend && backend->overlay_open && backend->overlay_open();
    if (backend && backend->begin_frame) {
        backend->begin_frame();
    }
    app.shell.ui.blocked = (app.shell.field != FIELD_NONE && !app.shell.field_inline) ||
                           app.shell.request != REQUEST_NONE || popup;
    if (!app.platform.window_collapsed) {
        layout_columns();
    }
    editor_draw_color(UI_CANVAS);
    SDL_RenderClear(app.platform.renderer);
    /* Inset panels expose the dark application background as narrow gutters. */
    editor_box((SDL_FRect){PANEL_GAP, PANEL_GAP, (float)(app.platform.width - 2 * PANEL_GAP),
                           TOP - 2 * PANEL_GAP},
               UI_PANEL);
    bool modal = (app.shell.field != FIELD_NONE && !app.shell.field_inline) ||
                 app.shell.request != REQUEST_NONE;
    bool saved_click = app.shell.click;
    if (modal) {
        app.shell.click = false;
    }
    app.shell.ui.blocked = modal || popup;
    editor_tooltip = NULL;
    editor_draw_global_tools();
    UiRect nav = {(float)app.shell.global_nav_x, 12,
                  fmaxf(0, editor_window_controls_left() - app.shell.global_nav_x - 12), 25};
    const cJSON *toolbar = pe_ui_get(app.shell.shell_layout->root, "toolbar");
    const cJSON *selector = cJSON_GetArrayItem(toolbar, cJSON_GetArraySize(toolbar) - 1);
    bool labels = cJSON_IsTrue(pe_ui_get(selector, "labels"));
    for (int i = 0; i < app.shell.workspaces.count; i++) {
        const EditorView *item = editor_view(app.shell.workspaces.items[i].id);
        const cJSON *entry = editor_layout_entry(item->id);
        float width =
            labels ? ui_tab_content_width(pe_ui_get(entry, "image")
                                              ? ICON_IMAGE
                                              : editor_yaml_icon(pe_ui_string(entry, "icon", "")),
                                          pe_ui_string(entry, "title", item->label))
                   : 32;
        ui_tabs_set_width(&app.shell.workspaces, item->id,
                          width + app.shell.tab_gap +
                              (cJSON_IsTrue(pe_ui_get(entry, "separator_before")) ? 22 : 0));
        app.shell.workspaces.gap = (float)app.shell.tab_gap;
        app.shell.workspaces.items[i].enabled =
            !editor_edit_pending() && (item->id == UI_MAP_VIEW || !editor_playing());
    }
    bool nav_blocked = app.shell.ui.blocked;
    for (int i = 0; i < app.shell.workspaces.count; i++) {
        const cJSON *entry = editor_layout_entry(app.shell.workspaces.items[i].id);
        UiRect gap = ui_tab_rect(&app.shell.workspaces, nav, 112, i);
        gap.w = 22;
        if (cJSON_IsTrue(pe_ui_get(entry, "separator_before")) && app.shell.ui.pressed &&
            ui_contains(gap, app.shell.ui.press_x, app.shell.ui.press_y)) {
            app.shell.ui.blocked = true;
        }
    }
    if (ui_tabs_update(&app.shell.ui, &app.shell.workspaces, nav, 112)) {
        editor_select_workspace(app.shell.workspaces.selected);
    }
    app.shell.ui.blocked = nav_blocked;
    ui_clip_push(&app.shell.ui, nav);
    ui_apply_clip(app.platform.renderer, &app.shell.ui);
    for (int i = 0; i < app.shell.workspaces.count; i++) {
        const EditorView *item = editor_view(app.shell.workspaces.items[i].id);
        const cJSON *entry = editor_layout_entry(item->id);
        UiRect r = ui_tab_rect(&app.shell.workspaces, nav, 112, i);
        if (cJSON_IsTrue(pe_ui_get(entry, "separator_before"))) {
            editor_box((SDL_FRect){r.x + 10, r.y + 4, 1, 17}, UI_BORDER);
            r.x += 22;
            r.w -= 22;
        }
        editor_register_ui_hit(r);
        bool hover = ui_hovered(&app.shell.ui, r);
        const char *image_path = pe_ui_string(entry, "image", NULL);
        SDL_Texture *image = image_path
                                 ? pe_ui_images_get(&app.shell.shell_images, app.platform.renderer,
                                                    app.shell.shell_layout, image_path)
                                 : NULL;
        const char *label = labels ? pe_ui_string(entry, "title", item->label) : "";
        bool selected = app.shell.workspaces.selected == item->id;
        bool enabled = app.shell.workspaces.items[i].enabled;
        if (image) {
            const cJSON *padding = pe_ui_get(entry, "image_padding");
            ui_draw_image_button(app.platform.renderer, (SDL_FRect){r.x, r.y, r.w, r.h}, image,
                                 label, padding ? (float)padding->valuedouble : 4, selected,
                                 enabled, hover,
                                 app.shell.ui.active == item->id && app.shell.ui.down,
                                 app.shell.ui.focus == item->id);
        } else {
            UiIcon fallback = editor_yaml_icon(pe_ui_string(entry, "icon", ""));
            ui_draw_button(app.platform.renderer, (SDL_FRect){r.x, r.y, r.w, r.h},
                           image_path && fallback == ICON_NONE ? ICON_IMAGE : fallback, label,
                           selected, enabled, hover);
        }
        if (hover) {
            editor_tooltip = pe_ui_string(entry, "title", item->label);
        }
    }
    ui_clip_pop(&app.shell.ui);
    ui_apply_clip(app.platform.renderer, &app.shell.ui);
    editor_window_draw_controls();
    if (app.platform.window_collapsed) {
        ui_end_frame(&app.shell.ui);
        app.shell.click = false;
        SDL_RenderPresent(app.platform.renderer);
        editor_process_deferred_ui();
        return;
    }
    if (!app.workspace.active_layout || !app.workspace.backend) {
        editor_text(LEFT + 16, TOP + 40, app.shell.status, UI_TEXT);
        ui_end_frame(&app.shell.ui);
        if (modal) {
            app.shell.ui.blocked = false;
            app.shell.click = saved_click;
            editor_draw_modal();
        }
        SDL_RenderPresent(app.platform.renderer);
        editor_process_deferred_ui();
        return;
    }

    const EditorView *view = editor_view(app.shell.workspaces.selected);
    float center_width = (float)(app.platform.width - LEFT - RIGHT - 2 * PANEL_GAP);
    int rows = editor_yaml_toolbar_rows(center_width);
    SDL_FRect tools = {LEFT + PANEL_GAP, TOP + PANEL_GAP, center_width, rows * 32.f + 6};
    app.shell.viewport =
        (SDL_FRect){tools.x, tools.y + tools.h + PANEL_GAP, tools.w,
                    app.platform.height - BOTTOM - PANEL_GAP - tools.y - tools.h - PANEL_GAP};
    editor_box(tools, UI_PANEL);
    editor_register_ui_hit((UiRect){tools.x, tools.y, tools.w, tools.h});
    ui_clip_push(&app.shell.ui, (UiRect){tools.x, tools.y, tools.w, tools.h});
    ui_apply_clip(app.platform.renderer, &app.shell.ui);
    view->draw_tools(tools);
    ui_clip_pop(&app.shell.ui);
    ui_apply_clip(app.platform.renderer, &app.shell.ui);
    const cJSON *columns = pe_ui_get(app.workspace.active_layout->root, "columns");
    editor_draw_yaml_node(pe_ui_get(columns, "center"), app.shell.viewport, 1);
    editor_draw_yaml_node(pe_ui_get(columns, "left"),
                          (SDL_FRect){PANEL_GAP, TOP + PANEL_GAP, LEFT - 2 * PANEL_GAP,
                                      app.platform.height - TOP - BOTTOM - 2 * PANEL_GAP},
                          0);
    editor_draw_yaml_node(pe_ui_get(columns, "right"),
                          (SDL_FRect){app.platform.width - RIGHT + PANEL_GAP, TOP + PANEL_GAP,
                                      RIGHT - 2 * PANEL_GAP,
                                      app.platform.height - TOP - BOTTOM - 2 * PANEL_GAP},
                          2);
    editor_box(
        (SDL_FRect){0, (float)(app.platform.height - BOTTOM), (float)app.platform.width, BOTTOM},
        UI_PANEL);
    editor_box((SDL_FRect){0, (float)(app.platform.height - BOTTOM), (float)app.platform.width, 1},
               UI_BORDER);
    float status_x = 14, status_y = (float)(app.platform.height - 18);
    const cJSON *fields = pe_ui_get(pe_ui_get(app.shell.shell_layout->root, "statusbar"), "fields");
    ui_clip_push(&app.shell.ui, (UiRect){0, (float)(app.platform.height - BOTTOM),
                                         (float)app.platform.width, BOTTOM});
    ui_apply_clip(app.platform.renderer, &app.shell.ui);
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
    ui_clip_pop(&app.shell.ui);
    ui_apply_clip(app.platform.renderer, &app.shell.ui);
    if (backend && backend->draw_overlay) {
        backend->draw_overlay();
    }
    if (editor_tooltip && !modal &&
        !(backend && backend->overlay_open && backend->overlay_open())) {
        float width = (float)strlen(editor_tooltip) * 8 + 20;
        float tx = fminf(app.shell.mx + 12, app.platform.width - width - 8),
              ty = fminf(app.shell.my + 30, app.platform.height - 28.f);
        ui_panel(app.platform.renderer, (SDL_FRect){tx, ty, width, 24}, UI_BORDER);
        editor_text(tx + 10, ty + 8, editor_tooltip, UI_TEXT);
    }

    UiRect ls = left_split(), rs = right_split();
    bool left_highlight =
        !modal && (ui_hovered(&app.shell.ui, ls) || app.shell.ui.active == UI_SPLIT_LEFT);
    bool right_highlight =
        !modal && (ui_hovered(&app.shell.ui, rs) || app.shell.ui.active == UI_SPLIT_RIGHT);
    editor_draw_split_grip(ls, left_highlight);
    editor_draw_split_grip(rs, right_highlight);
    bool resize = left_highlight || right_highlight;
    if (modal) {
        app.shell.ui.blocked = false;
        app.shell.click = saved_click;
        editor_draw_modal();
    }
    app.shell.ui.blocked = false;
    if (app.platform.resize_cursor) {
        SDL_SetCursor(app.shell.layout_resize_y && app.platform.vertical_cursor
                          ? app.platform.vertical_cursor
                      : (resize || app.shell.layout_resize_x) ? app.platform.resize_cursor
                                                              : SDL_GetDefaultCursor());
    }
    if (app.shell.field_inline && !app.shell.field_seen) {
        editor_field_stop();
    }
    ui_end_frame(&app.shell.ui);
    app.shell.click = false;
    if (editor_screenshot_path) {
        SDL_Surface *capture = SDL_RenderReadPixels(app.platform.renderer, NULL);
        if (!capture || !SDL_SaveBMP(capture, editor_screenshot_path)) {
            fprintf(stderr, "Screenshot failed: %s\n", SDL_GetError());
        }
        SDL_DestroySurface(capture);
    }
    SDL_RenderPresent(app.platform.renderer);
    editor_process_deferred_ui();
}
