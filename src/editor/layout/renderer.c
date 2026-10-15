#include "app/editor_internal.h"

static UiId yaml_id(const char *name);
static float yaml_button_width(const cJSON *n);
static void yaml_button(const cJSON *n, SDL_FRect area);
static int widget_state(UiId id);

static UiId yaml_id(const char *name) {
    if (!strcmp(name, "explorer_tab")) {
        return UI_EXPLORER_TAB;
    }
    if (!strcmp(name, "scene_tab")) {
        return UI_SCENE_TAB;
    }
    if (!strcmp(name, "inspector_tab")) {
        return UI_INSPECTOR_TAB;
    }
    if (!strcmp(name, "map_tab")) {
        return UI_MAP_TAB;
    }
    if (!strcmp(name, "tiles_tab")) {
        return UI_TILES_TAB;
    }
    if (!strcmp(name, "info_tab")) {
        return UI_INFO_TAB;
    }

    return ui_id(name);
}

UiIcon editor_yaml_icon(const char *name) {
    static const struct {
        const char *name;
        UiIcon icon;
    } names[] = {{"minimize", ICON_MINIMIZE},
                 {"maximize", ICON_MAXIMIZE},
                 {"restore", ICON_RESTORE},
                 {"collapse", ICON_COLLAPSE},
                 {"terminal", ICON_TERMINAL},
                 {"menu", ICON_MENU},
                 {"save", ICON_SAVE},
                 {"undo", ICON_UNDO},
                 {"redo", ICON_REDO},
                 {"play", ICON_PLAY},
                 {"stop", ICON_STOP},
                 {"import", ICON_IMPORT},
                 {"brush", ICON_BRUSH},
                 {"erase", ICON_ERASE},
                 {"rectangle", ICON_RECTANGLE},
                 {"fill", ICON_FILL},
                 {"select", ICON_SELECT},
                 {"collision", ICON_COLLISION},
                 {"grid", ICON_GRID},
                 {"refresh", ICON_REFRESH},
                 {"folder", ICON_FOLDER},
                 {"file", ICON_FILE},
                 {"image", ICON_IMAGE},
                 {"layers", ICON_LAYERS},
                 {"entity", ICON_ENTITY},
                 {"plus", ICON_PLUS},
                 {"delete", ICON_DELETE},
                 {"edit", ICON_EDIT},
                 {"up", ICON_UP},
                 {"down", ICON_DOWN},
                 {"right", ICON_RIGHT},
                 {"eye", ICON_EYE},
                 {"eye_off", ICON_EYE_OFF},
                 {"check", ICON_CHECK},
                 {"close", ICON_CLOSE}};

    for (size_t i = 0; i < sizeof names / sizeof names[0]; i++) {
        if (!strcmp(names[i].name, name)) {
            return names[i].icon;
        }
    }

    return ICON_NONE;
}

const cJSON *editor_layout_entry(UiId id) {
    if (!app.shell_layout) {
        return NULL;
    }

    const char *module = id == UI_MAP_VIEW ? "map" : "scripts";
    const cJSON *entries = pe_ui_get(app.shell_layout->root, "editors");
    for (const cJSON *e = entries ? entries->child : NULL; e; e = e->next) {
        if (!strcmp(pe_ui_string(e, "module", ""), module)) {
            return e;
        }
    }

    return NULL;
}

PeUiLayout *editor_load_editor_layout(UiId id, char *error, size_t size) {
    const cJSON *entry = editor_layout_entry(id);
    if (!entry) {
        snprintf(error, size, "Editor is not listed in shell.yaml");
        return NULL;
    }

    char path[1400], directory[1024];
    snprintf(directory, sizeof directory, "%s", app.ui_path);
    char *last = strrchr(directory, '/'), *back = strrchr(directory, '\\');
    if (back && (!last || back > last)) {
        last = back;
    }
    if (last) {
        last[1] = 0;
    } else {
        directory[0] = 0;
    }
    if (snprintf(path, sizeof path, "%s%s", directory, pe_ui_string(entry, "file", "")) >=
        (int)sizeof path) {
        return NULL;
    }

    PeUiLayout *layout = pe_ui_layout_load(path, "editor", error, size);
    if (layout && strcmp(pe_ui_string(layout->root, "id", ""), pe_ui_string(entry, "id", ""))) {
        pe_ui_layout_free(layout);
        snprintf(error, size, "%s:1:1: editor id does not match shell entry", path);
        return NULL;
    }

    return layout;
}

void editor_apply_shell_layout(void) {
    const cJSON *root = app.shell_layout->root, *layout = pe_ui_get(root, "layout"),
                *cols = pe_ui_get(layout, "columns");
    app.gap = pe_ui_get(layout, "gap")->valueint;
    app.tab_gap = pe_ui_get(layout, "tab_gap")->valueint;
    app.min_left = (float)pe_ui_get(pe_ui_get(cols, "left"), "minimum")->valuedouble;
    app.min_right = (float)pe_ui_get(pe_ui_get(cols, "right"), "minimum")->valuedouble;
    app.min_center = (float)pe_ui_get(pe_ui_get(cols, "center"), "minimum")->valuedouble;
    if (!app.loaded_view) {
        app.left_width = (float)pe_ui_get(pe_ui_get(cols, "left"), "initial")->valuedouble;
        app.right_width = (float)pe_ui_get(pe_ui_get(cols, "right"), "initial")->valuedouble;
    }

    UiTheme theme = ui_dark_theme();
    const cJSON *colors = pe_ui_get(root, "theme");
    const char *names[] = {"background", "panel", "surface", "hover", "accent", "text", "muted"};
    Uint32 *values[] = {&theme.canvas, &theme.panel, &theme.surface, &theme.hover,
                        &theme.accent, &theme.text,  &theme.muted};
    for (int i = 0; i < 7; i++) {
        const char *v = pe_ui_string(colors, names[i], NULL);
        if (v) {
            *values[i] = ((Uint32)strtoul(v + 1, NULL, 16) << 8) | 255u;
        }
    }
    ui_set_theme(&theme);
    UiTab items[UI_MAX_TABS];
    int count = 0;
    UiId selected = app.loaded_view;
    const cJSON *entries = pe_ui_get(root, "editors");
    for (const cJSON *e = entries->child; e; e = e->next) {
        items[count++] = (UiTab){
            !strcmp(pe_ui_string(e, "module", ""), "map") ? UI_MAP_VIEW : UI_SCRIPT_VIEW, true, 0};
    }

    bool found = false;
    for (int i = 0; i < count; i++) {
        if (items[i].id == selected) {
            found = true;
        }
    }
    if (!found) {
        selected = items[0].id;
    }
    ui_tabs_init(&app.workspaces, items, count, selected);
    app.workspaces.gap = (float)app.tab_gap;
}

static float yaml_button_width(const cJSON *n) {
    const char *label = pe_ui_string(n, "label", "");
    return *label ? ui_tab_content_width(editor_yaml_icon(pe_ui_string(n, "icon", "")), label) : 32;
}

static void yaml_button(const cJSON *n, SDL_FRect area) {
    const char *action = pe_ui_string(n, "action", NULL);
    editor_register_ui_hit((UiRect){area.x, area.y, area.w, area.h});
    bool enabled =
        editor_yaml_boolean(pe_ui_get(n, "enabled"), true) && editor_yaml_action_enabled(action);
    bool active = editor_yaml_boolean(pe_ui_get(n, "active"), editor_yaml_action_active(action));
    UiIcon icon = editor_yaml_icon(pe_ui_string(n, "icon", ""));
    if (action && !strcmp(action, "scene.toggle_play") && app.playing) {
        icon = ICON_STOP;
    }

    bool hover = ui_hovered(&app.ui, (UiRect){area.x, area.y, area.w, area.h});
    char buffer[256];
    const char *label = editor_yaml_text(pe_ui_get(n, "label"), buffer, sizeof buffer);
    ui_draw_button(app.renderer, area, icon, label, active, enabled, hover);
    if (hover) {
        editor_tooltip = pe_ui_string(n, "tooltip", NULL);
    }

    bool shell = false;
    for (int i = 0; i < app.shell_layout->mark_count; i++) {
        if (app.shell_layout->marks[i].value == n) {
            shell = true;
            break;
        }
    }

    UiId id = ui_id(pe_ui_string(n, "id", "")) ^ ui_id(shell                 ? "shell"
                                                       : editor_map_active() ? "map"
                                                                             : "scripts");
    if (!id) {
        id = 1;
    }
    if (ui_button(&app.ui, id, (UiRect){area.x, area.y, area.w, area.h}, enabled)) {
        editor_yaml_action(action);
    }
}

int editor_yaml_toolbar_rows(float width) {
    return editor_yaml_tool_rows(pe_ui_get(app.active_layout->root, "toolbar"), width);
}

int editor_yaml_tool_rows(const cJSON *list, float width) {
    float x = 8;
    int rows = 1;
    for (const cJSON *n = list ? list->child : NULL; n; n = n->next) {
        if (!editor_yaml_boolean(pe_ui_get(n, "visible"), true)) {
            continue;
        }
        float w = !strcmp(pe_ui_string(n, "type", ""), "separator")
                      ? 16
                      : fminf(yaml_button_width(n), fmaxf(32, width - 16));
        if (x + w > width - 8) {
            x = 8;
            rows++;
        }
        x += w + 6;
    }

    return rows;
}

void editor_draw_yaml_toolbar(SDL_FRect area) {
    const cJSON *list = pe_ui_get(app.active_layout->root, "toolbar");
    editor_draw_yaml_tool_list(list, area);
}

void editor_draw_yaml_tool_list(const cJSON *list, SDL_FRect area) {
    float x = area.x + 8, y = area.y + 6;
    for (const cJSON *n = list ? list->child : NULL; n; n = n->next) {
        if (!editor_yaml_boolean(pe_ui_get(n, "visible"), true)) {
            continue;
        }
        bool separator = !strcmp(pe_ui_string(n, "type", ""), "separator");
        float w = separator ? 16 : fminf(yaml_button_width(n), fmaxf(32, area.w - 16));
        if (x + w > area.x + area.w - 8) {
            x = area.x + 8;
            y += 32;
        }
        if (separator) {
            editor_box((SDL_FRect){x + 7, y + 4, 1, 17}, UI_BORDER);
        } else {
            yaml_button(n, (SDL_FRect){x, y, w, 25});
        }
        x += w + 6;
    }
}

void editor_draw_global_tools(void) {
    const cJSON *list = pe_ui_get(app.shell_layout->root, "toolbar");
    float x = 14;
    for (const cJSON *n = list->child; n; n = n->next) {
        if (!editor_yaml_boolean(pe_ui_get(n, "visible"), true)) {
            continue;
        }
        const char *type = pe_ui_string(n, "type", "");
        if (!strcmp(type, "editor_switcher")) {
            app.global_nav_x = (int)x;
            return;
        }
        if (!strcmp(type, "separator")) {
            editor_box((SDL_FRect){x + 8, 16, 1, 17}, UI_BORDER);
            x += 22;
        } else {
            float w = yaml_button_width(n);
            yaml_button(n, (SDL_FRect){x, 12, w, 25});
            x += w + 6;
        }
    }
    app.global_nav_x = (int)x;
}

static int widget_state(UiId id) {
    for (int i = 0; i < app.widget_count; i++) {
        if (app.widgets[i].id == id) {
            return i;
        }
    }
    if (app.widget_count == 64) {
        return -1;
    }

    int i = app.widget_count++;
    memset(&app.widgets[i], 0, sizeof app.widgets[i]);
    app.widgets[i].id = id;
    app.widgets[i].open = -1;
    return i;
}

bool editor_layout_captured(void) {
    for (int i = 0; i < app.widget_count; i++) {
        if (app.widgets[i].kind == 1 && app.widgets[i].id == app.ui.active) {
            return true;
        }
    }
    return false;
}

void editor_draw_yaml_node(const cJSON *n, SDL_FRect area, int column) {
    if (!n || area.w <= 0 || area.h <= 0 || !editor_yaml_boolean(pe_ui_get(n, "visible"), true)) {
        return;
    }

    const char *type = pe_ui_string(n, "type", "");
    UiId id = yaml_id(pe_ui_string(n, "id", ""));
    bool previous_blocked = app.ui.blocked;
    app.ui.blocked = previous_blocked || !editor_yaml_boolean(pe_ui_get(n, "enabled"), true);
    ui_clip_push(&app.ui, (UiRect){area.x, area.y, area.w, area.h});
    ui_apply_clip(app.renderer, &app.ui);
    if (!strcmp(type, "split")) {
        int state = widget_state(id);
        if (state < 0) {
            goto done;
        }
        WidgetState *stored = &app.widgets[state];
        if (stored->kind != 1) {
            stored->extent = (float)pe_ui_get(n, "initial")->valuedouble;
            stored->kind = 1;
        }
        UiAxis axis = !strcmp(pe_ui_string(n, "axis", ""), "vertical") ? UI_AXIS_Y : UI_AXIS_X;
        float min_first = (float)pe_ui_get(n, "minimum_first")->valuedouble;
        float min_second = (float)pe_ui_get(n, "minimum_second")->valuedouble;
        float gap = (float)pe_ui_get(n, "gap")->valuedouble;
        UiRect rect = {area.x, area.y, area.w, area.h};
        UiSplit split = ui_split_rects(rect, axis, stored->extent, min_first, min_second, gap);
        stored->extent = split.second_size;
        float total = (axis == UI_AXIS_Y ? area.h : area.w) - gap;
        float effective_min = fminf(min_second, split.second_size);
        ui_drag_value(
            &app.ui, id, split.grip, &stored->extent, effective_min,
            fmaxf(effective_min,
                  total - fminf(min_first, axis == UI_AXIS_Y ? split.first.h : split.first.w)),
            axis, -1);
        split = ui_split_rects(rect, axis, stored->extent, min_first, min_second, gap);
        stored->extent = split.second_size;
        editor_register_ui_hit(split.grip);
        bool highlight = ui_hovered(&app.ui, split.grip) || app.ui.active == id;
        if (highlight && !app.ui.blocked) {
            if (axis == UI_AXIS_Y) {
                app.layout_resize_y = true;
            } else {
                app.layout_resize_x = true;
            }
        }
        float cx = split.grip.x + split.grip.w * .5f;
        float cy = split.grip.y + split.grip.h * .5f;
        editor_draw_color(highlight ? UI_ACCENT : UI_MUTED);
        for (int k = -1; k <= 1; k++) {
            SDL_FRect dot = {floorf(cx + (axis == UI_AXIS_Y ? k * 6 : 0)) - 1,
                             floorf(cy + (axis == UI_AXIS_X ? k * 6 : 0)) - 1, 2, 2};
            SDL_RenderFillRect(app.renderer, &dot);
        }
        editor_draw_yaml_node(
            pe_ui_get(n, "first"),
            (SDL_FRect){split.first.x, split.first.y, split.first.w, split.first.h}, -1);
        editor_draw_yaml_node(
            pe_ui_get(n, "second"),
            (SDL_FRect){split.second.x, split.second.y, split.second.w, split.second.h}, -1);
    } else if (!strcmp(type, "tabs")) {
        int state = widget_state(id);
        if (state < 0) {
            goto done;
        }
        app.widgets[state].kind = 0;
        const cJSON *list = pe_ui_get(n, "tabs");
        int count = cJSON_GetArraySize(list);
        UiTabs temporary = {0};
        UiTabs *tabs = column == 0 ? &app.left_tabs : column == 2 ? &app.right_tabs : &temporary;
        if (column != 0 && column != 2) {
            tabs->selected = app.widgets[state].selected;
            tabs->count = app.widgets[state].count;
            tabs->scroll = app.widgets[state].scroll;
            tabs->dragging = app.widgets[state].dragging != 0;
            for (int i = 0; i < tabs->count; i++) {
                tabs->items[i] = (UiTab){app.widgets[state].order[i], true, 0};
            }
        }
        UiTab items[UI_MAX_TABS];
        int used = 0;
        for (int k = 0; k < tabs->count; k++) {
            for (const cJSON *t = list->child; t; t = t->next) {
                if (tabs->items[k].id == yaml_id(pe_ui_string(t, "id", ""))) {
                    items[used++] = (UiTab){tabs->items[k].id, true, 0};
                }
            }
        }
        for (const cJSON *t = list->child; t; t = t->next) {
            UiId key = yaml_id(pe_ui_string(t, "id", ""));
            bool found = false;
            for (int k = 0; k < used; k++) {
                if (items[k].id == key) {
                    found = true;
                }
            }
            if (!found) {
                items[used++] = (UiTab){key, true, 0};
            }
        }
        bool found = false;
        for (int k = 0; k < count; k++) {
            if (items[k].id == tabs->selected) {
                found = true;
            }
        }
        UiId selected = found ? tabs->selected : items[0].id;
        float scroll = tabs->scroll;
        bool dragging = tabs->dragging;
        ui_tabs_init(tabs, items, count, selected);
        tabs->scroll = scroll;
        tabs->dragging = dragging;
        tabs->gap = (float)app.tab_gap;
        for (const cJSON *t = list->child; t; t = t->next) {
            ui_tabs_set_width(tabs, yaml_id(pe_ui_string(t, "id", "")),
                              ui_tab_content_width(editor_yaml_icon(pe_ui_string(t, "icon", "")),
                                                   pe_ui_string(t, "title", "")) +
                                  app.tab_gap);
        }
        UiRect bar = {area.x, area.y, area.w, TABS - PANEL_GAP};
        editor_register_ui_hit(bar);
        if (column != 0 && column != 2 && app.wheel_delta && ui_contains(bar, app.mx, app.my)) {
            ui_tabs_scroll(tabs, bar.w, 112, -app.wheel_delta * 56);
            app.wheel_delta = 0;
        }
        ui_tabs_update(&app.ui, tabs, bar, 112);
        for (int k = 0; k < count; k++) {
            for (const cJSON *t = list->child; t; t = t->next) {
                if (tabs->items[k].id == yaml_id(pe_ui_string(t, "id", ""))) {
                    UiRect r = ui_tab_rect(tabs, bar, 112, k);
                    ui_draw_tab(app.renderer, (SDL_FRect){r.x, r.y, r.w, r.h},
                                editor_yaml_icon(pe_ui_string(t, "icon", "")),
                                pe_ui_string(t, "title", ""), tabs->selected == tabs->items[k].id,
                                true, ui_hovered(&app.ui, r));
                }
            }
        }
        app.widgets[state].scroll = tabs->scroll;
        app.widgets[state].dragging = tabs->dragging;
        app.widgets[state].count = count;
        app.widgets[state].selected = tabs->selected;
        for (int k = 0; k < count; k++) {
            app.widgets[state].order[k] = tabs->items[k].id;
        }
        for (const cJSON *t = list->child; t; t = t->next) {
            if (tabs->selected == yaml_id(pe_ui_string(t, "id", ""))) {
                editor_draw_yaml_node(pe_ui_get(t, "content"),
                                      (SDL_FRect){area.x, area.y + bar.h, area.w, area.h - bar.h},
                                      -1);
            }
        }
    } else if (!strcmp(type, "row") || !strcmp(type, "column") || !strcmp(type, "section")) {
        bool row = !strcmp(type, "row");
        float gap = (float)(pe_ui_get(n, "gap") ? pe_ui_get(n, "gap")->valuedouble : PANEL_GAP);
        const cJSON *children = pe_ui_get(n, "children");
        int count = 0;
        for (const cJSON *child = children->child; child; child = child->next) {
            if (editor_yaml_boolean(pe_ui_get(child, "visible"), true)) {
                count++;
            }
        }
        if (!strcmp(type, "section")) {
            int state = widget_state(id);
            if (state < 0) {
                goto done;
            }
            if (app.widgets[state].open < 0) {
                app.widgets[state].open = cJSON_IsTrue(pe_ui_get(n, "open"));
            }
            bool open = app.widgets[state].open != 0;
            ui_panel(app.renderer, area, UI_PANEL);
            ui_disclosure(&app.ui, id, (UiRect){area.x, area.y, area.w, 26}, &open, true);
            app.widgets[state].open = open;
            ui_draw_button(app.renderer, (SDL_FRect){area.x, area.y, area.w, 26},
                           open ? ICON_DOWN : ICON_RIGHT, pe_ui_string(n, "title", ""), false, true,
                           false);
            if (!open) {
                goto done;
            }
            area.y += 30;
            area.h -= 30;
        }
        float size = (fmaxf(0, (row ? area.w : area.h) - gap * (count - 1))) / (count ? count : 1);
        int i = 0;
        for (const cJSON *child = children->child; child; child = child->next) {
            if (!editor_yaml_boolean(pe_ui_get(child, "visible"), true)) {
                continue;
            }
            editor_draw_yaml_node(child,
                                  (SDL_FRect){area.x + (row ? i * (size + gap) : 0),
                                              area.y + (row ? 0 : i * (size + gap)),
                                              row ? size : area.w, row ? area.h : size},
                                  -1);
            i++;
        }
    } else if (!strcmp(type, "label")) {
        char buffer[256];
        editor_text(area.x + 8, area.y + 12,
                    editor_yaml_text(pe_ui_get(n, "label"), buffer, sizeof buffer), UI_TEXT);
    } else if (!strcmp(type, "button")) {
        yaml_button(n, (SDL_FRect){area.x, area.y, fminf(area.w, yaml_button_width(n)), 25});
    } else if (!strcmp(type, "separator")) {
        editor_box((SDL_FRect){area.x, area.y + 4, 1, fmaxf(0, area.h - 8)}, UI_BORDER);
    } else if (strcmp(type, "spacer")) {
        ui_panel(app.renderer, area, UI_PANEL);
        if (strcmp(type, "map_canvas") && strcmp(type, "script_editor")) {
            editor_register_ui_hit((UiRect){area.x, area.y, area.w, area.h});
        }
        app.panel_area = area;
        app.panel_top = area.y;
        if (!strcmp(type, "map_canvas") || !strcmp(type, "script_editor")) {
            app.viewport = area;
            app.canvas_enabled = !app.ui.blocked;
            if (!strcmp(type, "map_canvas")) {
                editor_draw_map_content();
            } else {
                editor_draw_script_content();
            }
        } else if (!strcmp(type, "file_tree")) {
            editor_draw_tree();
        } else if (!strcmp(type, "entity_tree")) {
            const cJSON *tools = pe_ui_get(n, "tools");
            if (tools) {
                editor_draw_yaml_tool_list(tools, area);
                float height = editor_yaml_tool_rows(tools, area.w) * 32.f + 6;
                app.panel_area.y += height;
                app.panel_area.h = fmaxf(0, app.panel_area.h - height);
                app.panel_top = app.panel_area.y;
            }
            editor_draw_scene_tree();
        } else if (!strcmp(type, "map_layers")) {
            editor_draw_map_panel();
        } else if (!strcmp(type, "tile_palette")) {
            editor_draw_tiles_panel();
        } else if (!strcmp(type, "inspector")) {
            editor_draw_inspector();
        } else if (!strcmp(type, "scene_info")) {
            editor_draw_info();
        } else if (!strcmp(type, "terminal")) {
            if (!app.console) {
                app.console = pe_console_new(app.root, !app.smoke);
            }
            if (app.console) {
                pe_console_draw(app.console, app.renderer, &app.ui, area, !app.ui.blocked);
            }
        } else if (!strcmp(type, "script_info")) {
            editor_draw_script_inspector();
        }
    }
done:
    app.ui.blocked = previous_blocked;
    ui_clip_pop(&app.ui);
    ui_apply_clip(app.renderer, &app.ui);
}
