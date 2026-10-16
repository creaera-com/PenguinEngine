#include "application/internal.h"

static UiId yaml_id(const char *name);
static float yaml_button_width(const cJSON *n);

static int widget_state(UiId id);
static UiRect tool_panel_areas[64];
static int tool_panel_count;

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
    if (!app.shell.shell_layout) {
        return NULL;
    }

    const char *module = id == UI_MAP_VIEW ? "map" : id == UI_SCRIPT_VIEW ? "scripts" : "vector";
    const cJSON *entries = pe_ui_get(app.shell.shell_layout->root, "editors");
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
    snprintf(directory, sizeof directory, "%s", app.shell.ui_path);
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
    pe_ui_images_free(&app.shell.shell_images);
    const cJSON *root = app.shell.shell_layout->root, *layout = pe_ui_get(root, "layout"),
                *cols = pe_ui_get(layout, "columns");
    app.shell.gap = pe_ui_get(layout, "gap")->valueint;
    app.shell.tab_gap = pe_ui_get(layout, "tab_gap")->valueint;
    app.shell.min_left = (float)pe_ui_get(pe_ui_get(cols, "left"), "minimum")->valuedouble;
    app.shell.min_right = (float)pe_ui_get(pe_ui_get(cols, "right"), "minimum")->valuedouble;
    app.shell.min_center = (float)pe_ui_get(pe_ui_get(cols, "center"), "minimum")->valuedouble;
    if (!app.workspace.loaded_view) {
        app.shell.left_width = (float)pe_ui_get(pe_ui_get(cols, "left"), "initial")->valuedouble;
        app.shell.right_width = (float)pe_ui_get(pe_ui_get(cols, "right"), "initial")->valuedouble;
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
    UiId selected = app.workspace.loaded_view;
    const cJSON *entries = pe_ui_get(root, "editors");
    for (const cJSON *e = entries->child; e; e = e->next) {
        items[count++] =
            (UiTab){!strcmp(pe_ui_string(e, "module", ""), "map")       ? UI_MAP_VIEW
                    : !strcmp(pe_ui_string(e, "module", ""), "scripts") ? UI_SCRIPT_VIEW
                                                                        : UI_VECTOR_VIEW,
                    true, 0};
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
    ui_tabs_init(&app.shell.workspaces, items, count, selected);
    app.shell.workspaces.gap = (float)app.shell.tab_gap;
}

static float yaml_button_width(const cJSON *n) {
    const char *label = pe_ui_string(n, "label", "");
    return *label ? ui_tab_content_width(pe_ui_get(n, "image")
                                             ? ICON_IMAGE
                                             : editor_yaml_icon(pe_ui_string(n, "icon", "")),
                                         label)
                  : 32;
}

void editor_draw_yaml_button(const cJSON *n, SDL_FRect area) {
    const char *action = pe_ui_string(n, "action", NULL);
    editor_register_ui_hit((UiRect){area.x, area.y, area.w, area.h});
    bool enabled =
        editor_yaml_boolean(pe_ui_get(n, "enabled"), true) && editor_yaml_action_enabled(action);
    bool active = editor_yaml_boolean(pe_ui_get(n, "active"), editor_yaml_action_active(action));
    UiIcon icon = editor_yaml_icon(pe_ui_string(n, "icon", ""));
    if (action && !strcmp(action, "scene.toggle_play") && editor_playing()) {
        icon = ICON_STOP;
    }

    if (action && !strcmp(action, "window.maximize") &&
        (SDL_GetWindowFlags(app.platform.window) & SDL_WINDOW_MAXIMIZED)) {
        icon = ICON_RESTORE;
    }
    if (action && !strcmp(action, "window.collapse") && app.platform.window_collapsed) {
        icon = ICON_DOWN;
    }
    bool shell = false;
    for (int i = 0; i < app.shell.shell_layout->mark_count; i++) {
        if (app.shell.shell_layout->marks[i].value == n) {
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

    bool hover = ui_hovered(&app.shell.ui, (UiRect){area.x, area.y, area.w, area.h});
    char buffer[256];
    const char *label = editor_yaml_text(pe_ui_get(n, "label"), buffer, sizeof buffer);
    const char *image_path = pe_ui_string(n, "image", NULL);
    SDL_Texture *image =
        image_path ? pe_ui_images_get(shell ? &app.shell.shell_images : &app.shell.editor_images,
                                      app.platform.renderer,
                                      shell ? app.shell.shell_layout : app.workspace.active_layout,
                                      image_path)
                   : NULL;
    if (image) {
        const cJSON *padding = pe_ui_get(n, "image_padding");
        ui_draw_image_button(app.platform.renderer, area, image, label,
                             padding ? (float)padding->valuedouble : 4, active, enabled, hover,
                             app.shell.ui.active == id && app.shell.ui.down,
                             app.shell.ui.focus == id);
    } else {
        ui_draw_button(app.platform.renderer, area,
                       image_path && icon == ICON_NONE ? ICON_IMAGE : icon, label, active, enabled,
                       hover);
    }
    if (hover) {
        editor_tooltip = pe_ui_string(n, "tooltip", NULL);
    }

    if (ui_button(&app.shell.ui, id, (UiRect){area.x, area.y, area.w, area.h}, enabled)) {
        editor_yaml_action(action);
    }
}

int editor_yaml_toolbar_rows(float width) {
    return editor_yaml_tool_rows(pe_ui_get(app.workspace.active_layout->root, "toolbar"), width);
}

int editor_yaml_tool_rows(const cJSON *list, float width) {
    float x = 8;
    int rows = 1;
    for (const cJSON *n = list ? list->child : NULL; n; n = n->next) {
        if ((!strcmp(pe_ui_string(n, "type", ""), "vector_preview") ||
             !strcmp(pe_ui_string(n, "type", ""), "vector_grid"))) {
            break;
        }
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
    const cJSON *list = pe_ui_get(app.workspace.active_layout->root, "toolbar");
    editor_draw_yaml_tool_list(list, area, false);
}

void editor_draw_yaml_tool_list(const cJSON *list, SDL_FRect area, bool right_aligned) {
    const cJSON *start = list ? list->child : NULL;
    float available = fmaxf(32, area.w - 16), y = area.y + 6;
    while (start && strcmp(pe_ui_string(start, "type", ""), "vector_preview") &&
           strcmp(pe_ui_string(start, "type", ""), "vector_grid")) {
        float used = 0;
        const cJSON *end = start;
        for (; end; end = end->next) {
            if ((!strcmp(pe_ui_string(end, "type", ""), "vector_preview") ||
                 !strcmp(pe_ui_string(end, "type", ""), "vector_grid"))) {
                break;
            }
            if (!editor_yaml_boolean(pe_ui_get(end, "visible"), true)) {
                continue;
            }
            float w = !strcmp(pe_ui_string(end, "type", ""), "separator")
                          ? 16
                          : fminf(yaml_button_width(end), available);
            float next = used + (used > 0 ? 6 : 0) + w;
            if (used > 0 && next > available) {
                break;
            }
            used = next;
        }
        float x = area.x + (right_aligned ? area.w - 8 - used : 8);
        for (const cJSON *n = start; n != end; n = n->next) {
            if (!editor_yaml_boolean(pe_ui_get(n, "visible"), true)) {
                continue;
            }
            bool separator = !strcmp(pe_ui_string(n, "type", ""), "separator");
            float w = separator ? 16 : fminf(yaml_button_width(n), available);
            if (separator) {
                editor_box((SDL_FRect){x + 7, y + 4, 1, 17}, UI_BORDER);
            } else {
                editor_draw_yaml_button(n, (SDL_FRect){x, y, w, 25});
            }
            x += w + 6;
        }
        start = end;
        y += 32;
    }
}

void editor_draw_global_tools(void) {
    tool_panel_count = 0;
    const cJSON *list = pe_ui_get(app.shell.shell_layout->root, "toolbar");
    float x = 14;
    for (const cJSON *n = list->child; n; n = n->next) {
        if (!editor_yaml_boolean(pe_ui_get(n, "visible"), true)) {
            continue;
        }
        const char *type = pe_ui_string(n, "type", "");
        if (!strcmp(type, "editor_switcher")) {
            app.shell.global_nav_x = (int)x;
            return;
        }
        if (!strcmp(type, "separator")) {
            editor_box((SDL_FRect){x + 8, 16, 1, 17}, UI_BORDER);
            x += 22;
        } else {
            float w = yaml_button_width(n);
            editor_draw_yaml_button(n, (SDL_FRect){x, 12, w, 25});
            x += w + 6;
        }
    }
    app.shell.global_nav_x = (int)x;
}

static int widget_state(UiId id) {
    for (int i = 0; i < app.shell.widget_count; i++) {
        if (app.shell.widgets[i].id == id) {
            return i;
        }
    }
    if (app.shell.widget_count == 64) {
        return -1;
    }

    int i = app.shell.widget_count++;
    memset(&app.shell.widgets[i], 0, sizeof app.shell.widgets[i]);
    app.shell.widgets[i].id = id;
    app.shell.widgets[i].open = -1;
    return i;
}

bool editor_layout_captured(void) {
    for (int i = 0; i < app.shell.widget_count; i++) {
        if (app.shell.widgets[i].kind != 0 && app.shell.widgets[i].id == app.shell.ui.active) {
            return true;
        }
    }
    return false;
}

bool editor_layout_scrolls_at(float x, float y) {
    for (int i = 0; i < tool_panel_count; i++) {
        if (ui_contains(tool_panel_areas[i], x, y)) {
            return true;
        }
    }
    return false;
}

static const cJSON *tool_content(const cJSON *section) {
    const cJSON *children = pe_ui_get(section, "children");
    for (const cJSON *child = children->child; child; child = child->next) {
        if ((!strcmp(pe_ui_string(child, "type", ""), "vector_preview") ||
             !strcmp(pe_ui_string(child, "type", ""), "vector_grid")) &&
            editor_yaml_boolean(pe_ui_get(child, "visible"), true)) {
            return child;
        }
    }
    return NULL;
}

static float tool_content_height(const cJSON *section) {
    const cJSON *content = tool_content(section);
    return content ? (float)pe_ui_get(content, "height")->valuedouble + 12 : 0;
}

static void draw_tool_panel(const cJSON *n, SDL_FRect area, UiId id) {
    ui_panel(app.platform.renderer, area, UI_PANEL);
    int state = widget_state(id);
    if (state < 0) {
        return;
    }
    app.shell.widgets[state].kind = 2;
    UiRect clip = {area.x + 6, area.y + 6, fmaxf(0, area.w - 24), fmaxf(0, area.h - 12)};
    if (tool_panel_count < 64) {
        tool_panel_areas[tool_panel_count++] = (UiRect){area.x, area.y, area.w, area.h};
    }
    editor_register_ui_hit((UiRect){area.x, area.y, area.w, area.h});
    const cJSON *sections = pe_ui_get(n, "children");
    float content = 0;
    for (const cJSON *s = sections->child; s; s = s->next) {
        if (!editor_yaml_boolean(pe_ui_get(s, "visible"), true)) {
            continue;
        }
        int group = widget_state(yaml_id(pe_ui_string(s, "id", "")));
        if (group < 0) {
            continue;
        }
        if (app.shell.widgets[group].open < 0) {
            app.shell.widgets[group].open = cJSON_IsTrue(pe_ui_get(s, "open"));
        }
        content += 33 + 8;
        if (app.shell.widgets[group].open) {
            content += editor_yaml_tool_rows(pe_ui_get(s, "children"), clip.w) * 32 + 6 +
                       tool_content_height(s);
        }
    }
    UiScroll scroll = {app.shell.widgets[state].scroll, content, clip.h};
    if (!app.shell.ui.blocked && !editor_edit_pending() && app.shell.wheel_delta &&
        ui_contains((UiRect){area.x, area.y, area.w, area.h}, app.shell.mx, app.shell.my)) {
        ui_scroll_by(&scroll, -app.shell.wheel_delta * 60);
        app.shell.wheel_delta = 0;
    }
    ui_scroll_clamp(&scroll);
    UiRect track = {area.x + area.w - 14, clip.y, 8, clip.h};
    ui_scrollbar(&app.shell.ui, id, track, &scroll, UI_AXIS_Y);
    app.shell.widgets[state].scroll = scroll.offset;
    ui_clip_push(&app.shell.ui, clip);
    ui_apply_clip(app.platform.renderer, &app.shell.ui);
    float y = clip.y - scroll.offset;
    for (const cJSON *s = sections->child; s; s = s->next) {
        if (!editor_yaml_boolean(pe_ui_get(s, "visible"), true)) {
            continue;
        }
        UiId group_id = yaml_id(pe_ui_string(s, "id", ""));
        int group = widget_state(group_id);
        if (group < 0) {
            continue;
        }
        bool open = app.shell.widgets[group].open != 0;
        bool enabled = editor_yaml_boolean(pe_ui_get(s, "enabled"), true);
        bool blocked = app.shell.ui.blocked;
        app.shell.ui.blocked = blocked || !enabled;
        UiRect header = {clip.x, y, clip.w, 33};
        editor_register_ui_hit(header);
        ui_disclosure(&app.shell.ui, group_id, header, &open, !editor_edit_pending());
        app.shell.widgets[group].open = open;
        float height = 33;
        if (open) {
            height += editor_yaml_tool_rows(pe_ui_get(s, "children"), clip.w) * 32 + 6 +
                      tool_content_height(s);
        }
        editor_box((SDL_FRect){clip.x, y, clip.w, height}, editor_widget_background());
        if (ui_hovered(&app.shell.ui, header)) {
            editor_box((SDL_FRect){clip.x, y, clip.w, 33}, UI_HOVER);
        }
        ui_icon(app.platform.renderer, open ? ICON_DOWN : ICON_RIGHT, clip.x + 9, y + 10, 12,
                UI_MUTED);
        ui_icon(app.platform.renderer, editor_yaml_icon(pe_ui_string(s, "icon", "")), clip.x + 27,
                y + 8, 16, UI_ACCENT);
        editor_text(clip.x + 51, y + 12, pe_ui_string(s, "title", ""), UI_TEXT);
        if (open) {
            editor_draw_yaml_tool_list(pe_ui_get(s, "children"),
                                       (SDL_FRect){clip.x, y + 33, clip.w, height - 33}, true);
            const cJSON *content_widget = tool_content(s);
            if (content_widget) {
                float top =
                    y + 33 + editor_yaml_tool_rows(pe_ui_get(s, "children"), clip.w) * 32 + 12;
                float content_widget_height =
                    (float)pe_ui_get(content_widget, "height")->valuedouble;
                if (top + content_widget_height > clip.y && top < clip.y + clip.h) {
                    editor_draw_yaml_node(
                        content_widget,
                        (SDL_FRect){clip.x + 8, top, fmaxf(0, clip.w - 16), content_widget_height},
                        -1);
                }
            }
        }
        app.shell.ui.blocked = blocked;
        y += height + 8;
    }
    ui_clip_pop(&app.shell.ui);
    ui_apply_clip(app.platform.renderer, &app.shell.ui);
    ui_draw_scrollbar(app.platform.renderer, &app.shell.ui, id, track, &scroll, UI_AXIS_Y);
}

void editor_draw_yaml_node(const cJSON *n, SDL_FRect area, int column) {
    if (!n || area.w <= 0 || area.h <= 0 || !editor_yaml_boolean(pe_ui_get(n, "visible"), true)) {
        return;
    }

    const char *type = pe_ui_string(n, "type", "");
    UiId id = yaml_id(pe_ui_string(n, "id", ""));
    bool previous_blocked = app.shell.ui.blocked;
    app.shell.ui.blocked = previous_blocked || !editor_yaml_boolean(pe_ui_get(n, "enabled"), true);
    ui_clip_push(&app.shell.ui, (UiRect){area.x, area.y, area.w, area.h});
    ui_apply_clip(app.platform.renderer, &app.shell.ui);
    if (!strcmp(type, "tool_panel")) {
        draw_tool_panel(n, area, id);
    } else if (!strcmp(type, "split")) {
        int state = widget_state(id);
        if (state < 0) {
            goto done;
        }
        WidgetState *stored = &app.shell.widgets[state];
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
            &app.shell.ui, id, split.grip, &stored->extent, effective_min,
            fmaxf(effective_min,
                  total - fminf(min_first, axis == UI_AXIS_Y ? split.first.h : split.first.w)),
            axis, -1);
        split = ui_split_rects(rect, axis, stored->extent, min_first, min_second, gap);
        stored->extent = split.second_size;
        editor_register_ui_hit(split.grip);
        bool highlight = ui_hovered(&app.shell.ui, split.grip) || app.shell.ui.active == id;
        if (highlight && !app.shell.ui.blocked) {
            if (axis == UI_AXIS_Y) {
                app.shell.layout_resize_y = true;
            } else {
                app.shell.layout_resize_x = true;
            }
        }
        float cx = split.grip.x + split.grip.w * .5f;
        float cy = split.grip.y + split.grip.h * .5f;
        editor_draw_color(highlight ? UI_ACCENT : UI_MUTED);
        for (int k = -1; k <= 1; k++) {
            SDL_FRect dot = {floorf(cx + (axis == UI_AXIS_Y ? k * 6 : 0)) - 1,
                             floorf(cy + (axis == UI_AXIS_X ? k * 6 : 0)) - 1, 2, 2};
            SDL_RenderFillRect(app.platform.renderer, &dot);
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
        app.shell.widgets[state].kind = 0;
        const cJSON *list = pe_ui_get(n, "tabs");
        int count = cJSON_GetArraySize(list);
        UiTabs temporary = {0};
        UiTabs *tabs = column == 0   ? &app.shell.left_tabs
                       : column == 2 ? &app.shell.right_tabs
                                     : &temporary;
        if (column != 0 && column != 2) {
            tabs->selected = app.shell.widgets[state].selected;
            tabs->count = app.shell.widgets[state].count;
            tabs->scroll = app.shell.widgets[state].scroll;
            tabs->dragging = app.shell.widgets[state].dragging != 0;
            for (int i = 0; i < tabs->count; i++) {
                tabs->items[i] = (UiTab){app.shell.widgets[state].order[i], true, 0};
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
        tabs->gap = (float)app.shell.tab_gap;
        for (const cJSON *t = list->child; t; t = t->next) {
            ui_tabs_set_width(tabs, yaml_id(pe_ui_string(t, "id", "")),
                              ui_tab_content_width(editor_yaml_icon(pe_ui_string(t, "icon", "")),
                                                   pe_ui_string(t, "title", "")) +
                                  app.shell.tab_gap);
        }
        UiRect bar = {area.x, area.y, area.w, TABS - PANEL_GAP};
        editor_register_ui_hit(bar);
        if (column != 0 && column != 2 && app.shell.wheel_delta &&
            ui_contains(bar, app.shell.mx, app.shell.my)) {
            ui_tabs_scroll(tabs, bar.w, 112, -app.shell.wheel_delta * 56);
            app.shell.wheel_delta = 0;
        }
        ui_tabs_update(&app.shell.ui, tabs, bar, 112);
        for (int k = 0; k < count; k++) {
            for (const cJSON *t = list->child; t; t = t->next) {
                if (tabs->items[k].id == yaml_id(pe_ui_string(t, "id", ""))) {
                    UiRect r = ui_tab_rect(tabs, bar, 112, k);
                    ui_draw_tab(app.platform.renderer, (SDL_FRect){r.x, r.y, r.w, r.h},
                                editor_yaml_icon(pe_ui_string(t, "icon", "")),
                                pe_ui_string(t, "title", ""), tabs->selected == tabs->items[k].id,
                                true, ui_hovered(&app.shell.ui, r));
                }
            }
        }
        app.shell.widgets[state].scroll = tabs->scroll;
        app.shell.widgets[state].dragging = tabs->dragging;
        app.shell.widgets[state].count = count;
        app.shell.widgets[state].selected = tabs->selected;
        for (int k = 0; k < count; k++) {
            app.shell.widgets[state].order[k] = tabs->items[k].id;
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
            if (app.shell.widgets[state].open < 0) {
                app.shell.widgets[state].open = cJSON_IsTrue(pe_ui_get(n, "open"));
            }
            bool open = app.shell.widgets[state].open != 0;
            ui_panel(app.platform.renderer, area, UI_PANEL);
            ui_disclosure(&app.shell.ui, id, (UiRect){area.x, area.y, area.w, 26}, &open, true);
            app.shell.widgets[state].open = open;
            ui_draw_button(app.platform.renderer, (SDL_FRect){area.x, area.y, area.w, 26},
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
        editor_draw_yaml_button(
            n, (SDL_FRect){area.x, area.y, fminf(area.w, yaml_button_width(n)), 25});
    } else if (!strcmp(type, "separator")) {
        editor_box((SDL_FRect){area.x, area.y + 4, 1, fmaxf(0, area.h - 8)}, UI_BORDER);
    } else if (strcmp(type, "spacer")) {
        ui_panel(app.platform.renderer, area, UI_PANEL);
        if (strcmp(type, "map_canvas") && strcmp(type, "script_editor") &&
            strcmp(type, "vector_canvas")) {
            editor_register_ui_hit((UiRect){area.x, area.y, area.w, area.h});
        }
        app.shell.panel_area = area;
        app.shell.panel_top = area.y;
        if (!strcmp(type, "map_canvas") || !strcmp(type, "script_editor") ||
            !strcmp(type, "vector_canvas")) {
            app.shell.viewport = area;
            app.shell.canvas_enabled = !app.shell.ui.blocked;
            if (!strcmp(type, "map_canvas")) {
                editor_draw_map_content();
            } else if (!strcmp(type, "vector_canvas")) {
                vector_draw_canvas();
            } else {
                editor_draw_script_content();
            }
        } else if (!strcmp(type, "file_tree")) {
            editor_draw_tree();
        } else if (!strcmp(type, "entity_tree")) {
            const cJSON *tools = pe_ui_get(n, "tools");
            if (tools) {
                editor_draw_yaml_tool_list(tools, area, true);
                float height = editor_yaml_tool_rows(tools, area.w) * 32.f + 6;
                app.shell.panel_area.y += height;
                app.shell.panel_area.h = fmaxf(0, app.shell.panel_area.h - height);
                app.shell.panel_top = app.shell.panel_area.y;
            }
            editor_draw_scene_tree();
        } else if (!strcmp(type, "vector_tree")) {
            vector_draw_tree();
        } else if (!strcmp(type, "vector_grid")) {
            vector_draw_grid_settings(area);
        } else if (!strcmp(type, "vector_preview")) {
            vector_draw_preview(area);
        } else if (!strcmp(type, "vector_inspector")) {
            vector_draw_inspector();
        } else if (!strcmp(type, "map_layers")) {
            editor_draw_map_panel();
        } else if (!strcmp(type, "tile_palette")) {
            editor_draw_tiles_panel();
        } else if (!strcmp(type, "inspector")) {
            editor_draw_inspector();
        } else if (!strcmp(type, "scene_info")) {
            editor_draw_info();
        } else if (!strcmp(type, "terminal")) {
            if (!app.shell.console) {
                app.shell.console = pe_console_new(app.project.root, !app.runtime.smoke);
            }
            if (app.shell.console) {
                pe_console_draw(app.shell.console, app.platform.renderer, &app.shell.ui, area,
                                !app.shell.ui.blocked);
            }
        } else if (!strcmp(type, "script_info")) {
            editor_draw_script_inspector();
        }
    }
done:
    app.shell.ui.blocked = previous_blocked;
    ui_clip_pop(&app.shell.ui);
    ui_apply_clip(app.platform.renderer, &app.shell.ui);
}
