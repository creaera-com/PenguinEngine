#include "editor_smoke.h"
#include "app/editor_internal.h"

static void smoke_pointer(Uint32 type, float x, float y);
static void smoke_click(float x, float y);

bool editor_verify_editor(void) {
    editor_draw();
    PeScene baseline = app.map->scene;
    app.tile = 4;
    app.tool = BRUSH;
    float sx = app.viewport.x + (2 * app.map->scene.tile_size + 2 - app.camera.x) * app.camera.zoom;
    float sy = app.viewport.y + (2 * app.map->scene.tile_size + 2 - app.camera.y) * app.camera.zoom;
    SDL_Event e = {0};
    e.type = SDL_EVENT_MOUSE_BUTTON_DOWN;
    e.button.button = SDL_BUTTON_LEFT;
    e.button.x = sx;
    e.button.y = sy;
    editor_event(&e);
    e.type = SDL_EVENT_MOUSE_MOTION;
    e.motion.x = sx + app.map->scene.tile_size * app.camera.zoom;
    e.motion.y = sy;
    e.motion.state = SDL_BUTTON_LMASK;
    editor_event(&e);
    e.type = SDL_EVENT_MOUSE_BUTTON_UP;
    e.button.button = SDL_BUTTON_LEFT;
    e.button.x = sx + app.map->scene.tile_size * app.camera.zoom;
    e.button.y = sy;
    editor_event(&e);
    bool ok = app.map->history.count == 1 && pe_tile_get(&app.map->scene, 0, 2, 2) == 4 &&
              pe_tile_get(&app.map->scene, 0, 3, 2) == 4;
    e.type = SDL_EVENT_KEY_DOWN;
    e.key.key = SDLK_Z;
    e.key.mod = SDL_KMOD_CTRL;
    editor_event(&e);
    ok = ok && !memcmp(&app.map->scene, &baseline, sizeof baseline);
    e.key.mod = SDL_KMOD_CTRL | SDL_KMOD_SHIFT;
    editor_event(&e);
    ok = ok && pe_tile_get(&app.map->scene, 0, 3, 2) == 4;
    PeScene edited = app.map->scene;
    e.key.key = SDLK_F5;
    e.key.mod = 0;
    editor_event(&e);
    pe_update(&app.map->runtime, 1, 0, 1.f / 60);
    ok = ok && app.playing && !memcmp(&app.map->scene, &edited, sizeof edited);
    editor_event(&e);
    ok = ok && !app.playing;
    app.map->scene = baseline;
    pe_history_free(&app.map->history);
    app.tile = 1;
    if (!ok) {
        fprintf(stderr, "Editor interaction smoke test failed\n");
    }

    return ok;
}

bool editor_verify_script_editor(void) {
    PeScene scene_before = app.map->scene;
    editor_tab_select(true);
    if (!app.script || app.map) {
        return false;
    }

    PeText original = (*app.script).document;
    pe_text_init(&(*app.script).document, "");
    editor_tab_select(true);
    SDL_Event e = {0};
    e.type = SDL_EVENT_TEXT_INPUT;
    e.text.text = "cons";
    editor_event(&e);
    memset(&e, 0, sizeof e);
    e.type = SDL_EVENT_KEY_DOWN;
    e.key.key = SDLK_SPACE;
    e.key.mod = SDL_KMOD_CTRL;
    editor_event(&e);
    bool ok = (*app.script).show_completion;
    editor_draw();
    ok = ok && (*app.script).popup.w > 0;
    e.key.key = SDLK_RETURN;
    e.key.mod = 0;
    editor_event(&e);
    ok = ok && !strcmp((*app.script).document.text, "const");
    e.key.key = SDLK_Z;
    e.key.mod = SDL_KMOD_CTRL;
    editor_event(&e);
    ok = ok && !strcmp((*app.script).document.text, "cons");
    e.key.mod = SDL_KMOD_CTRL | SDL_KMOD_SHIFT;
    editor_event(&e);
    ok = ok && !strcmp((*app.script).document.text, "const");
    e.key.key = SDLK_BACKSPACE;
    e.key.mod = 0;
    editor_event(&e);
    editor_draw();
    memset(&e, 0, sizeof e);
    e.type = SDL_EVENT_MOUSE_BUTTON_DOWN;
    e.button.button = SDL_BUTTON_LEFT;
    e.button.x = (*app.script).popup.x + 8;
    e.button.y = (*app.script).popup.y + 8;
    editor_event(&e);
    ok = ok && !strcmp((*app.script).document.text, "const");
    e.type = SDL_EVENT_MOUSE_BUTTON_UP;
    editor_event(&e);
    memset(&e, 0, sizeof e);
    e.type = SDL_EVENT_TEXT_INPUT;
    e.text.text = " value = ;\n";
    editor_event(&e);
    memset(&e, 0, sizeof e);
    e.type = SDL_EVENT_KEY_DOWN;
    e.key.key = SDLK_F7;
    editor_event(&e);
    ok = ok && !(*app.script).diagnostic.valid && (*app.script).diagnostic.line == 1;
    editor_request(REQUEST_QUIT, NULL);
    ok = ok && app.request == REQUEST_QUIT && app.running;
    app.request = REQUEST_NONE;
    se_free(&(*app.script));
    (*app.script).document = original;
    (*app.script).highlighted_revision = UINT64_MAX;
    (*app.script).checked_revision = UINT64_MAX;
    (*app.script).show_completion = false;
    (*app.script).top_line = (*app.script).left_column = 0;
    se_check(&(*app.script));
    editor_tab_select(false);
    ok = ok && app.map && !app.script &&
         !memcmp(&scene_before, &app.map->scene, sizeof scene_before);
    if (!ok) {
        fprintf(stderr, "Script editor interaction smoke test failed\n");
    }

    return ok;
}

static void smoke_pointer(Uint32 type, float x, float y) {
    SDL_Event e = {0};
    e.type = type;
    if (type == SDL_EVENT_MOUSE_MOTION) {
        e.motion.x = x;
        e.motion.y = y;
        e.motion.state = SDL_BUTTON_LMASK;
    } else {
        e.button.button = SDL_BUTTON_LEFT;
        e.button.x = x;
        e.button.y = y;
    }
    editor_event(&e);
    editor_draw();
}

bool editor_verify_ui_layout(void) {
    PeScene original = app.map->scene;
    editor_tab_select(false);
    ui_cancel(&app.ui);
    editor_draw();
    float x = app.left_width, y = TOP + TABS + 90;
    smoke_pointer(SDL_EVENT_MOUSE_BUTTON_DOWN, x, y);
    smoke_pointer(SDL_EVENT_MOUSE_MOTION, x + 100, y);
    smoke_pointer(SDL_EVENT_MOUSE_BUTTON_UP, x + 100, y);
    bool ok = fabsf(app.left_width - 340) < .01f && app.viewport.x == 340 + PANEL_GAP;
    x = app.width - app.right_width;
    smoke_pointer(SDL_EVENT_MOUSE_BUTTON_DOWN, x, y);
    smoke_pointer(SDL_EVENT_MOUSE_MOTION, x - 80, y);
    smoke_pointer(SDL_EVENT_MOUSE_BUTTON_UP, x - 80, y);
    ok = ok && fabsf(app.right_width - 360) < .01f;
    app.left_tabs.selected = UI_MAP_TAB;
    editor_draw();
    float header_x = 30, header_y = SIDE_TOP + 20;
    smoke_pointer(SDL_EVENT_MOUSE_BUTTON_DOWN, header_x, header_y);
    smoke_pointer(SDL_EVENT_MOUSE_BUTTON_UP, header_x, header_y);
    ok = ok && !app.layers_open;
    float tab_x = GLOBAL_NAV_X + 16, tab_y = 24;
    smoke_pointer(SDL_EVENT_MOUSE_BUTTON_DOWN, tab_x, tab_y);
    smoke_pointer(SDL_EVENT_MOUSE_MOTION, tab_x + 60, tab_y);
    smoke_pointer(SDL_EVENT_MOUSE_BUTTON_UP, tab_x + 60, tab_y);
    ok = ok && app.workspaces.items[0].id == UI_SCRIPT_VIEW &&
         app.workspaces.items[1].id == UI_MAP_VIEW && editor_map_active();
    /* Sidebar navigation is independent of workspace tabs and scene history. */
    float side_y = TOP + PANEL_GAP + 12;
    float scene_tab_x = PANEL_GAP + app.left_tabs.items[0].width + 20;
    float info_tab_x = app.width - RIGHT + PANEL_GAP + app.right_tabs.items[0].width + 20;
    smoke_pointer(SDL_EVENT_MOUSE_BUTTON_DOWN, scene_tab_x, side_y);
    smoke_pointer(SDL_EVENT_MOUSE_BUTTON_UP, scene_tab_x, side_y);
    smoke_pointer(SDL_EVENT_MOUSE_BUTTON_DOWN, info_tab_x, side_y);
    smoke_pointer(SDL_EVENT_MOUSE_BUTTON_UP, info_tab_x, side_y);
    ok = ok && app.left_tabs.selected == UI_SCENE_TAB && app.right_tabs.selected == UI_INFO_TAB &&
         editor_map_active();
    uint32_t saved_selected = app.selected;
    int saved_tool = app.tool;
    for (int i = 0; i < PE_ENTITIES; i++) {
        if (!app.map->scene.objects[i].alive) {
            continue;
        }
        smoke_pointer(SDL_EVENT_MOUSE_BUTTON_DOWN, 60, SIDE_TOP + 58);
        smoke_pointer(SDL_EVENT_MOUSE_BUTTON_UP, 60, SIDE_TOP + 58);
        ok = ok && app.selected == app.map->scene.objects[i].stable_id &&
             app.right_tabs.selected == UI_INSPECTOR_TAB;
        break;
    }
    app.selected = saved_selected;
    app.tool = saved_tool;
    app.left_tabs.selected = UI_EXPLORER_TAB;
    app.right_tabs.selected = UI_INSPECTOR_TAB;
    editor_tab_select(true);
    PeText saved = (*app.script).document;
    char lines[4096] = "";
    for (int i = 0; i < 100; i++) {
        strcat(lines, "// scrollable line\n");
    }
    pe_text_init(&(*app.script).document, lines);
    (*app.script).extent_revision = (*app.script).highlighted_revision =
        (*app.script).checked_revision = UINT64_MAX;
    (*app.script).top_line = 0;
    editor_tab_select(true);
    editor_draw();
    float scroll_x = app.viewport.x + app.viewport.w - 8, scroll_y = (*app.script).body.y + 8;
    smoke_pointer(SDL_EVENT_MOUSE_BUTTON_DOWN, scroll_x, scroll_y);
    smoke_pointer(SDL_EVENT_MOUSE_MOTION, scroll_x, scroll_y + 120);
    smoke_pointer(SDL_EVENT_MOUSE_BUTTON_UP, scroll_x, scroll_y + 120);
    ok = ok && (*app.script).top_line > 0 && !strcmp((*app.script).document.text, lines) &&
         (*app.script).document.count == 0;
    se_free(&(*app.script));
    (*app.script).document = saved;
    (*app.script).extent_revision = (*app.script).highlighted_revision =
        (*app.script).checked_revision = UINT64_MAX;
    (*app.script).top_line = (*app.script).left_column = 0;
    se_check(&(*app.script));
    editor_tab_select(false);
    ok = ok && !memcmp(&original, &app.map->scene, sizeof original);
    app.left_width = 240;
    app.right_width = 280;
    app.layers_open = app.palette_open = true;
    app.tree_view.offset = app.inspector_view.offset = 0;
    UiTab tabs[UI_MAX_TABS];
    for (int i = 0; i < editor_view_count; i++) {
        tabs[i] = (UiTab){editor_views[i].id, true, 0};
    }
    ui_tabs_init(&app.workspaces, tabs, editor_view_count, UI_MAP_VIEW);
    ui_cancel(&app.ui);
    editor_tab_select(false);
    editor_draw();
    if (!ok) {
        fprintf(stderr, "Shared UI layout / capture smoke test failed\n");
    }

    return ok;
}

static void smoke_click(float x, float y) {
    smoke_pointer(SDL_EVENT_MOUSE_BUTTON_DOWN, x, y);
    smoke_pointer(SDL_EVENT_MOUSE_BUTTON_UP, x, y);
}

bool editor_verify_workspace_tools(void) {
    PeScene original = app.map->scene;
    int history_cursor = app.map->history.cursor;
    uint64_t revision = 0;
    bool grid = app.grid;
    PeCamera camera = app.camera;
    editor_tab_select(false);
    editor_draw();
    float action_y = TOP + PANEL_GAP + 18;
    smoke_click(30, 24);
    bool menu_unchanged =
        editor_map_active() && app.request == REQUEST_NONE && app.field == FIELD_NONE;
    smoke_click(GLOBAL_NAV_X + app.workspaces.items[0].width + 16, 24);
    bool ok = menu_unchanged && editor_scripts_active() && !app.map;
    if (!app.script) {
        return false;
    }
    revision = (*app.script).document.revision;
    (*app.script).checked_revision = UINT64_MAX;
    smoke_click(LEFT + PANEL_GAP + 8 + 16, action_y);
    ok = ok && editor_scripts_active() && (*app.script).checked_revision == revision;
    /* Global Undo/Redo must operate on the active script, never the scene. */
    PeText saved_document = (*app.script).document;
    pe_text_init(&(*app.script).document, "// toolbar test");
    pe_text_insert(&(*app.script).document, "// edited\n");
    se_changed(&(*app.script), app.viewport, false);
    editor_draw();
    smoke_click(150, 24);
    ok = ok && (*app.script).document.position == 0 &&
         !strcmp((*app.script).document.text, "// toolbar test");
    smoke_click(188, 24);
    ok = ok && (*app.script).document.position == 1 &&
         strstr((*app.script).document.text, "edited") != NULL;
    pe_text_free(&(*app.script).document);
    (*app.script).document = saved_document;
    (*app.script).extent_revision = (*app.script).highlighted_revision =
        (*app.script).checked_revision = UINT64_MAX;
    se_check(&(*app.script));
    SDL_Event f5 = {0};
    f5.type = SDL_EVENT_KEY_DOWN;
    f5.key.key = SDLK_F5;
    editor_event(&f5);
    ok = ok && !app.playing;
    bool document_unchanged = (*app.script).document.revision == revision;
    smoke_click(GLOBAL_NAV_X + 16, 24);
    ok = ok && editor_map_active() && !app.script && document_unchanged;
    /* Narrow the center until the last map actions wrap to a second row. */
    app.left_width = 400;
    app.right_width = app.width - app.left_width - app.min_center;
    editor_draw();
    ok = ok && app.viewport.y == TOP + 2 * PANEL_GAP + 70;
    int columns = (int)((app.viewport.w - 10) / 38);
    int grid_index = editor_view(UI_MAP_VIEW)->action_count - 1;
    smoke_click(LEFT + PANEL_GAP + 8 + (grid_index % columns) * 38 + 16,
                action_y + (grid_index / columns) * 32);
    ok = ok && app.grid != grid && !memcmp(&original, &app.map->scene, sizeof original) &&
         app.map->history.cursor == history_cursor && !memcmp(&camera, &app.camera, sizeof camera);
    app.grid = grid;
    app.left_width = 240;
    app.right_width = 280;
    ui_cancel(&app.ui);
    editor_tab_select(false);
    editor_draw();
    if (!ok) {
        fprintf(stderr, "Workspace navigation / contextual toolbar smoke test failed\n");
    }

    return ok;
}

bool editor_verify_lifecycle(void) {
    char error[256], map_backup[1300], script_backup[1300], original_ui[1024], test_ui[1024];
    snprintf(map_backup, sizeof map_backup, "%s.baseline", app.session_paths[0]);
    snprintf(script_backup, sizeof script_backup, "%s.baseline", app.session_paths[1]);
    snprintf(original_ui, sizeof original_ui, "%s", app.ui_path);
    if (strlen(app.ui_path) + 7 >= sizeof test_ui) {
        return false;
    }
    memcpy(test_ui, app.ui_path, strlen(app.ui_path));
    strcpy(test_ui + strlen(app.ui_path), ".smoke");
    UiId id = UI_MAP_VIEW;
    editor_tab_select(false);
    editor_draw();
    if (!app.map || !pe_session_save(map_backup, id, editor_checkpoint, &id, error, sizeof error)) {
        return false;
    }

    bool ok = editor_begin();
    if (ok) {
        pe_tile_set(&app.map->scene, 0, 1, 1, 7);
        editor_commit();
    }
    app.camera = (PeCamera){11, -7, 2.5f};
    PeScene edited = app.map->scene;
    uint64_t map_revision = app.map->history.revision;
    char original_cache[1200];
    snprintf(original_cache, sizeof original_cache, "%s", app.session_paths[0]);
    snprintf(app.session_paths[0], sizeof app.session_paths[0],
             "missing-session-directory/map.cache");
    editor_select_workspace(UI_SCRIPT_VIEW);
    ok = ok && editor_map_active() && app.map && !app.script &&
         app.map->history.revision == map_revision;
    snprintf(app.session_paths[0], sizeof app.session_paths[0], "%s", original_cache);
    editor_select_workspace(UI_SCRIPT_VIEW);
    editor_draw();
    if (!app.script || app.map) {
        return false;
    }
    ok = ok && app.session_dirty[0];
    id = UI_SCRIPT_VIEW;
    if (!pe_session_save(script_backup, id, editor_checkpoint, &id, error, sizeof error)) {
        return false;
    }
    pe_text_insert(&app.script->document, "// unsaved lifecycle test\n");
    se_changed(app.script, app.viewport, false);
    uint64_t script_revision = app.script->document.revision;
    int script_position = app.script->document.position;
    editor_select_workspace(UI_MAP_VIEW);
    editor_draw();
    if (!app.map || app.script) {
        return false;
    }
    ok = ok && app.session_dirty[1] && !memcmp(&edited, &app.map->scene, sizeof edited) &&
         app.map->history.revision == map_revision && app.camera.x == 11 && app.camera.zoom == 2.5f;
    editor_undo_active(false);
    ok = ok && pe_tile_get(&app.map->scene, 0, 1, 1) != 7;
    editor_undo_active(true);
    ok = ok && pe_tile_get(&app.map->scene, 0, 1, 1) == 7;
    /* Corrupt a target checkpoint: the source must be restored, never discarded. */
    FILE *f = fopen(app.session_paths[1], "r+b");
    if (!f) {
        return false;
    }
    fseek(f, -1, SEEK_END);
    int byte = fgetc(f);
    fseek(f, -1, SEEK_END);
    fputc(byte ^ 255, f);
    fclose(f);
    editor_select_workspace(UI_SCRIPT_VIEW);
    ok = ok && editor_map_active() && app.map && !app.script &&
         app.map->history.revision == map_revision;
    f = fopen(app.session_paths[1], "r+b");
    if (!f) {
        return false;
    }
    fseek(f, -1, SEEK_END);
    fputc(byte, f);
    fclose(f);
    editor_select_workspace(UI_SCRIPT_VIEW);
    editor_draw();
    if (!app.script || app.map) {
        return false;
    }
    ok = ok && app.script->document.revision == script_revision &&
         app.script->document.position == script_position;
    editor_undo_active(false);
    ok = ok && app.script->document.position == script_position - 1;
    editor_undo_active(true);
    ok = ok && app.script->document.position == script_position;
    editor_request(REQUEST_QUIT, NULL);
    ok = ok && app.request == REQUEST_QUIT && app.running;
    app.request = REQUEST_NONE;
    editor_select_workspace(UI_MAP_VIEW);
    editor_draw();
    /* A valid reload changes layout/theme and reinstantiates the same dirty document. */
    cJSON *root = cJSON_Duplicate(app.shell_layout->root, true);
    cJSON_ReplaceItemInObjectCaseSensitive((cJSON *)pe_ui_get(root, "theme"), "accent",
                                           cJSON_CreateString("#B08968"));
    cJSON_ReplaceItemInObjectCaseSensitive((cJSON *)pe_ui_get(root, "layout"), "gap",
                                           cJSON_CreateNumber(2));
    char *json = cJSON_PrintUnformatted(root);
    cJSON_Delete(root);
    f = fopen(test_ui, "wb");
    if (!f) {
        free(json);
        return false;
    }
    fwrite(json, 1, strlen(json), f);
    fclose(f);
    free(json);
    snprintf(app.ui_path, sizeof app.ui_path, "%s", test_ui);
    ok = editor_reload_layout() && ok;
    ok = ok && UI_ACCENT == 0xb08968ffu && app.gap == 2 && app.map &&
         app.map->history.revision == map_revision;
    MapPayload *before = app.map;
    f = fopen(test_ui, "wb");
    if (!f) {
        return false;
    }
    fputs("broken: [", f);
    fclose(f);
    ok = ok && !editor_reload_layout() && app.map == before && UI_ACCENT == 0xb08968ffu;
    snprintf(app.ui_path, sizeof app.ui_path, "%s", original_ui);
    ok = editor_reload_layout() && ok;
    remove(test_ui);
    /* Restore the test baseline without writing any project files. */
    editor_select_workspace(UI_SCRIPT_VIEW);
    if (!app.script) {
        return false;
    }
    se_free(app.script);
    memset(app.script, 0, sizeof *app.script);
    id = UI_SCRIPT_VIEW;
    ok = pe_session_load(script_backup, id, editor_checkpoint, &id, error, sizeof error) && ok;
    se_check(app.script);
    editor_select_workspace(UI_MAP_VIEW);
    if (!app.map) {
        return false;
    }
    pe_assets_free(&app.map->assets);
    pe_history_free(&app.map->history);
    memset(app.map, 0, sizeof *app.map);
    id = UI_MAP_VIEW;
    ok = pe_session_load(map_backup, id, editor_checkpoint, &id, error, sizeof error) && ok;
    editor_assets_sync();
    remove(map_backup);
    remove(script_backup);
    app.session_dirty[0] = editor_map_dirty();
    app.mx = app.my = 0;
    ui_cancel(&app.ui);
    editor_draw();
    if (!ok) {
        fprintf(stderr, "Editor lifecycle / checkpoint / YAML reload smoke failed\n");
    }

    return ok;
}

bool editor_verify_console(void) {
    editor_tab_select(false);
    editor_draw();
    if (!app.console) {
        return false;
    }
    PeConsole *service = app.console;
    PeTerminal *terminal = pe_console_terminal(service);
    PeScene baseline = app.map->scene;
    SDL_FRect body = pe_console_area(service);
    float old_height = body.h;
    int state = -1;
    for (int i = 0; i < app.widget_count; i++) {
        if (app.widgets[i].kind == 1) {
            state = i;
            break;
        }
    }
    if (state < 0) {
        return false;
    }
    float extent = app.widgets[state].extent;
    float x = app.viewport.x + app.viewport.w * .5f, y = app.viewport.y + app.viewport.h + 4;
    smoke_pointer(SDL_EVENT_MOUSE_BUTTON_DOWN, x, y);
    editor_draw();
    smoke_pointer(SDL_EVENT_MOUSE_MOTION, x, y - 32);
    editor_draw();
    smoke_pointer(SDL_EVENT_MOUSE_BUTTON_UP, x, y - 32);
    editor_draw();
    bool ok =
        app.widgets[state].extent > extent + 20 && pe_console_area(service).h > old_height + 20;
    ok = ok && editor_reload_layout();
    editor_draw();
    ok = ok && app.console == service && pe_console_terminal(service) == terminal;
    body = pe_console_area(service);
    smoke_click(body.x + 10, body.y + 8);
    editor_draw();
    ok = ok && pe_console_focused(service);
    SDL_Event event = {0};
    event.type = SDL_EVENT_TEXT_INPUT;
    event.text.text = "echo penguin";
    editor_event(&event);
    event.type = SDL_EVENT_KEY_DOWN;
    event.key.key = SDLK_C;
    event.key.mod = SDL_KMOD_CTRL;
    editor_event(&event);
    size_t length;
    const char *output = pe_terminal_output(terminal, &length);
    ok = ok && length == 13 && !memcmp(output, "echo penguin\x03", 13) &&
         !memcmp(&baseline, &app.map->scene, sizeof baseline);
    pe_terminal_consume(terminal, length);
    pe_terminal_feed(terminal, "shared-session", 14);
    editor_tab_select(true);
    editor_draw();
    ok = ok && app.console == service && pe_console_terminal(app.console) == terminal && !app.map &&
         app.script;
    char *copy = pe_terminal_copy(terminal, 0, 0, 0, 13);
    ok = ok && copy && !strcmp(copy, "shared-session");
    free(copy);
    size_t script_length = app.script->document.length;
    body = pe_console_area(service);
    smoke_click(body.x + 10, body.y + 8);
    editor_draw();
    event.type = SDL_EVENT_TEXT_INPUT;
    event.text.text = "x";
    editor_event(&event);
    ok = ok && app.script->document.length == script_length;
    pe_terminal_output(terminal, &length);
    pe_terminal_consume(terminal, length);
    editor_tab_select(false);
    editor_draw();
    state = -1;
    for (int i = 0; i < app.widget_count; i++) {
        if (app.widgets[i].kind == 1) {
            state = i;
            break;
        }
    }
    ok = ok && state >= 0 && app.widgets[state].extent > extent + 20;
    if (state >= 0) {
        app.widgets[state].extent = extent;
    }
    body = pe_console_area(service);
    smoke_click(app.viewport.x + 8, TOP + 8);
    ui_cancel(&app.ui);
    const char *reset = "\033[2J\033[H";
    pe_terminal_feed(terminal, reset, strlen(reset));
    app.mx = app.my = 0;
    editor_draw();
    if (!ok) {
        fprintf(stderr, "Console / split layout / focus / shared session smoke failed\n");
    }
    return ok;
}

bool editor_verify_window(void) {
    editor_draw();
    int width = app.width, height = app.height;
    float controls = editor_window_controls_left();
    bool ok = cJSON_GetArraySize(pe_ui_get(app.shell_layout->root, "window_controls")) == 3;
    ok = ok && editor_window_hit_test(width - 1, height - 1) == SDL_HITTEST_RESIZE_BOTTOMRIGHT;
    ok = ok && editor_window_hit_test((int)controls - 20, 20) == SDL_HITTEST_DRAGGABLE;
    ok = ok && editor_window_hit_test((int)controls + 16, 20) == SDL_HITTEST_NORMAL;
    ok = ok && editor_window_hit_test(20, 20) == SDL_HITTEST_NORMAL;
    editor_window_action("window.collapse");
    editor_draw();
    SDL_SyncWindow(app.window);
    int collapsed_width, collapsed_height;
    SDL_GetWindowSize(app.window, &collapsed_width, &collapsed_height);
    ok = ok && app.window_collapsed && collapsed_height == TOP;
    ok = ok && editor_window_hit_test(collapsed_width / 2, TOP - 1) == SDL_HITTEST_DRAGGABLE;
    editor_window_action("window.collapse");
    editor_draw();
    SDL_SyncWindow(app.window);
    int restored_width, restored_height;
    SDL_GetWindowSize(app.window, &restored_width, &restored_height);
    ok = ok && !app.window_collapsed && restored_width == width && restored_height == height;
    ok = ok && !editor_window_action("window.unknown");
    smoke_pointer(SDL_EVENT_MOUSE_MOTION, 500, 400);
    if (!ok) {
        fprintf(stderr, "Window chrome smoke failed\n");
    }
    return ok;
}

bool editor_verify_scene_graph(void) {
    PeScene original = app.map->scene;
    pe_history_free(&app.map->history);
    pe_scene_init(&app.map->scene, 16, 16, 16);
    snprintf(app.map->scene.tileset, PE_PATH, "%s", original.tileset);
    app.map->scene.tileset_tile_size = original.tileset_tile_size;
    app.map->collapsed_count = 0;
    app.left_tabs.selected = UI_SCENE_TAB;
    app.selected = 0;
    editor_draw();
    bool ok = !editor_yaml_action_enabled("scene.add_child");
    float tools_y = TOP + TABS + 18.f;
    float tools_x = LEFT - PANEL_GAP - 8 - (4 * 32 + 3 * 6);
    smoke_click(tools_x + 16, tools_y);
    uint32_t root = app.selected;
    ok = ok && root != 0 && app.map->history.count == 1;
    smoke_click(tools_x + 92, tools_y);
    uint32_t child = app.selected;
    PeObject *o = pe_entity_get(&app.map->scene, pe_entity_find(&app.map->scene, child));
    ok = ok && o && o->parent_id == root;
    smoke_click(tools_x + 54, tools_y);
    uint32_t sibling = app.selected;
    o = pe_entity_get(&app.map->scene, pe_entity_find(&app.map->scene, sibling));
    ok = ok && o && o->parent_id == root;
    float first_y = TOP + TABS + 48.f;
    smoke_click(80, first_y + 36);
    ok = ok && app.selected == child;
    smoke_click(tools_x + 92, tools_y);
    uint32_t leaf = app.selected;
    o = pe_entity_get(&app.map->scene, pe_entity_find(&app.map->scene, leaf));
    ok = ok && o && o->parent_id == child;
    editor_draw();
    ok = ok && app.scene_view.content == 4 * 26;
    smoke_click(23, first_y + 10);
    editor_draw();
    ok = ok && app.scene_view.content == 26;
    smoke_click(23, first_y + 10);
    editor_draw();
    ok = ok && app.scene_view.content == 4 * 26;
    /* Drag the root in world space and verify inherited motion of the leaf. */
    smoke_click(65, first_y + 10);
    float before_x = 0, before_y = 0;
    ok = ok && pe_entity_world_position(&app.map->scene, pe_entity_find(&app.map->scene, leaf),
                                        &before_x, &before_y);
    float sx = app.viewport.x + (66 - app.camera.x) * app.camera.zoom;
    float sy = app.viewport.y + (66 - app.camera.y) * app.camera.zoom;
    smoke_pointer(SDL_EVENT_MOUSE_BUTTON_DOWN, sx, sy);
    smoke_pointer(SDL_EVENT_MOUSE_MOTION, sx + 32 * app.camera.zoom, sy + 16 * app.camera.zoom);
    smoke_pointer(SDL_EVENT_MOUSE_BUTTON_UP, sx + 32 * app.camera.zoom, sy + 16 * app.camera.zoom);
    float x = 0, y = 0;
    ok = ok &&
         pe_entity_world_position(&app.map->scene, pe_entity_find(&app.map->scene, leaf), &x, &y);
    ok = ok && fabsf(x - before_x - 32) < .01f && fabsf(y - before_y - 16) < .01f;
    editor_undo_active(false);
    ok = ok &&
         pe_entity_world_position(&app.map->scene, pe_entity_find(&app.map->scene, leaf), &x, &y);
    ok = ok && fabsf(x - before_x) < .01f;
    editor_undo_active(true);
    smoke_click(tools_x + 130, tools_y);
    ok = ok && !pe_entity_valid(&app.map->scene, pe_entity_find(&app.map->scene, leaf));
    editor_undo_active(false);
    ok = ok && pe_entity_valid(&app.map->scene, pe_entity_find(&app.map->scene, leaf));
    editor_draw();
    /* Collapse the child and persist both hierarchy and tree state across F6 and unload. */
    smoke_click(35, first_y + 36);
    editor_draw();
    ok = ok && app.map->collapsed_count == 1 && app.map->collapsed[0] == child;
    ok = ok && editor_reload_layout();
    editor_tab_select(true);
    ok = ok && app.map == NULL;
    editor_tab_select(false);
    ok = ok && app.map && app.map->collapsed_count == 1 && app.map->collapsed[0] == child;
    if (app.map) {
        o = pe_entity_get(&app.map->scene, pe_entity_find(&app.map->scene, leaf));
        ok = ok && o && o->parent_id == child;
        pe_history_free(&app.map->history);
        app.map->scene = original;
        app.map->collapsed_count = 0;
    }
    app.selected = 0;
    app.left_tabs.selected = UI_EXPLORER_TAB;
    app.tool = BRUSH;
    editor_selection_sync();
    if (!ok) {
        fprintf(stderr, "Scene graph smoke failed\n");
    }
    return ok;
}

bool editor_verify_components(void) {
    PeScene original = app.map->scene;
    pe_history_free(&app.map->history);
    pe_scene_init(&app.map->scene, 16, 16, 16);
    snprintf(app.map->scene.tileset, PE_PATH, "%s", original.tileset);
    app.map->scene.tileset_tile_size = original.tileset_tile_size;
    PeEntity entity = pe_entity_create(&app.map->scene, 0);
    uint32_t id = app.map->scene.objects[entity.index].stable_id;
    app.selected = id;
    memset(app.map->component_ids, 0, sizeof app.map->component_ids);
    memset(app.map->component_closed, 0, sizeof app.map->component_closed);
    editor_inspect_entity();
    editor_draw();
    float x = app.width - RIGHT + PANEL_GAP + 9;
    float top = TOP + TABS + 42.f + 22;
    float remove_x = x + (RIGHT - 2 * PANEL_GAP - 32) - 18;
    float content_before = app.inspector_view.content;
    smoke_click(x + 90, top + 12);
    bool ok = app.map->component_add_open && app.inspector_view.content == content_before;
    SDL_Event key = {0};
    key.type = SDL_EVENT_KEY_DOWN;
    key.key.key = SDLK_ESCAPE;
    editor_event(&key);
    editor_draw();
    ok = ok && !app.map->component_add_open;
    PeScene before_popup = app.map->scene;
    smoke_click(x + 90, top + 12);
    smoke_click(app.viewport.x + 40, app.viewport.y + 40);
    ok = ok && !app.map->component_add_open && app.selected == id &&
         !memcmp(&before_popup, &app.map->scene, sizeof before_popup);
    smoke_click(x + 90, top + 12);
    key.key.key = SDLK_DOWN;
    editor_event(&key);
    key.key.key = SDLK_RETURN;
    editor_event(&key);
    editor_draw();
    ok = ok && app.map->scene.objects[entity.index].mask == (PE_TRANSFORM | PE_SPRITE);
    editor_undo_active(false);
    editor_draw();
    smoke_click(x + 90, top + 12);
    /* Empty entity: the fourth choice is Player, which also adds Transform + Body. */
    smoke_click(x + 90, top + 33 + 3 * 28 + 12);
    PeObject *o = pe_entity_get(&app.map->scene, entity);
    ok = ok && o && o->mask == (PE_TRANSFORM | PE_BODY | PE_PLAYER) && app.map->history.count == 1;
    top -= 22;
    smoke_click(x + 90, top + 12);
    smoke_click(x + 90, top + 33 + 28 + 12);
    ok = ok && o->mask == 15 && !app.map->component_add_open;
    smoke_click(x + 90, top + 12);
    smoke_click(x + 90, top + 33 + 12);
    ok = ok && app.map->component_add_open && o->mask == 15;
    key.key.key = SDLK_ESCAPE;
    editor_event(&key);
    editor_draw();
    float transform = top + 33, sprite = transform + 164, body = sprite + 77, player = body + 106;
    smoke_click(x + 80, transform + 12);
    ok = ok && (app.map->component_closed[entity.index] & PE_TRANSFORM);
    smoke_click(x + 80, transform + 12);
    ok = ok && !(app.map->component_closed[entity.index] & PE_TRANSFORM);
    uint64_t revision = app.map->history.revision;
    smoke_click(remove_x, transform + 12);
    smoke_click(remove_x, body + 12);
    ok = ok && o->mask == 15 && app.map->history.revision == revision;
    smoke_click(remove_x, sprite + 12);
    ok = ok && !(o->mask & PE_SPRITE);
    editor_undo_active(false);
    ok = ok && (o->mask & PE_SPRITE);
    editor_draw();
    smoke_click(x + 20, transform + 34 + 12);
    ok = ok && app.field == FIELD_NONE;
    smoke_click(x + 180, transform + 34 + 12);
    ok = ok && app.field == FIELD_X;
    ok = ok && app.field_inline && app.ui.focus == app.field_id;
    SDL_Event text = {0};
    text.type = SDL_EVENT_TEXT_INPUT;
    text.text.text = "-12.5";
    editor_event(&text);
    ok = ok && !strcmp(app.input, "-12.5") && o->x == 0;
    key.key.key = SDLK_LEFT;
    editor_event(&key);
    key.key.key = SDLK_DELETE;
    editor_event(&key);
    text.text.text = "25";
    editor_event(&text);
    ok = ok && !strcmp(app.input, "-12.25") && o->x == 0;
    key.key.key = SDLK_RETURN;
    editor_event(&key);
    ok = ok && app.field == FIELD_NONE && o->x == -12.25f;
    editor_undo_active(false);
    ok = ok && o->x == 0;
    editor_draw();
    smoke_click(x + 180, transform + 34 + 12);
    text.text.text = "99";
    editor_event(&text);
    key.key.key = SDLK_ESCAPE;
    editor_event(&key);
    ok = ok && app.field == FIELD_NONE && o->x == 0;
    editor_draw();
    smoke_click(x + 180, transform + 34 + 12);
    text.text.text = "8";
    editor_event(&text);
    smoke_click(x + 20, sprite + 34 + 12);
    ok = ok && app.field == FIELD_NONE && o->x == 8;
    editor_undo_active(false);
    editor_draw();
    smoke_click(x + 20, sprite + 34 + 12);
    ok = ok && app.field == FIELD_NONE;
    smoke_click(x + 180, sprite + 34 + 12);
    ok = ok && app.field == FIELD_COLOR;
    uint32_t color = o->color;
    strcpy(app.input, "nothex");
    editor_field_apply();
    ok = ok && app.field == FIELD_COLOR && app.field_inline && app.field_invalid &&
         o->color == color;
    smoke_click(app.viewport.x + 40, app.viewport.y + 40);
    ok = ok && app.field == FIELD_COLOR && app.selected == id && o->color == color;
    strcpy(app.input, "#12345678");
    editor_field_apply();
    ok = ok && app.field == FIELD_NONE && o->color == 0x12345678;
    editor_undo_active(false);
    ok = ok && o->color == color;
    editor_undo_active(true);
    editor_draw();
    smoke_click(x + 180, body + 34 + 12);
    ok = ok && app.field == FIELD_VX;
    strcpy(app.input, "25.5");
    editor_field_apply();
    ok = ok && app.field == FIELD_NONE && o->vx == 25.5f;
    editor_draw();
    smoke_click(remove_x, player + 12);
    ok = ok && !(o->mask & PE_PLAYER);
    smoke_click(remove_x, body + 12);
    ok = ok && !(o->mask & PE_BODY);
    editor_undo_active(false);
    editor_undo_active(false);
    ok = ok && o->mask == 15 && o->vx == 25.5f;
    editor_draw();
    smoke_click(x + 80, sprite + 12);
    ok = ok && (app.map->component_closed[entity.index] & PE_SPRITE);
    ok = ok && editor_reload_layout();
    editor_tab_select(true);
    editor_tab_select(false);
    o = pe_entity_get(&app.map->scene, pe_entity_find(&app.map->scene, id));
    ok = ok && o && o->mask == 15 && o->color == 0x12345678 && o->vx == 25.5f &&
         (app.map->component_closed[entity.index] & PE_SPRITE);
    editor_play_toggle();
    ok = ok && !editor_component_change(PE_PLAYER, false);
    editor_play_toggle();
    pe_history_free(&app.map->history);
    app.map->scene = original;
    memset(app.map->component_ids, 0, sizeof app.map->component_ids);
    memset(app.map->component_closed, 0, sizeof app.map->component_closed);
    app.map->component_add_open = false;
    app.selected = 0;
    app.layers_open = app.palette_open = true;
    app.inspector_view.offset = 0;
    editor_selection_sync();
    if (!ok) {
        fprintf(stderr, "Component inspector smoke failed\n");
    }
    return ok;
}

static bool smoke_panel_tab(bool right, UiId id) {
    UiTabs *tabs = right ? &app.right_tabs : &app.left_tabs;
    UiRect bar = {right ? app.width - RIGHT + PANEL_GAP : PANEL_GAP, TOP + PANEL_GAP,
                  (right ? RIGHT : LEFT) - 2 * PANEL_GAP, TABS - PANEL_GAP};
    for (int i = 0; i < tabs->count; i++) {
        if (tabs->items[i].id == id) {
            UiRect tab = ui_tab_rect(tabs, bar, 112, i);
            smoke_click(tab.x + tab.w * .5f, tab.y + 12);
            return tabs->selected == id;
        }
    }
    return false;
}

bool editor_verify_map_panels(void) {
    PeScene original = app.map->scene;
    pe_history_free(&app.map->history);
    app.layers_open = app.palette_open = true;
    editor_draw();
    bool ok = smoke_panel_tab(false, UI_MAP_TAB) && smoke_panel_tab(true, UI_TILES_TAB);
    float x = PANEL_GAP + 9, w = LEFT - 2 * PANEL_GAP - 32;
    float right_x = app.width - RIGHT + PANEL_GAP + 9;
    float first = TOP + TABS + 72.f;
    int before = app.map->scene.layer_count;
    float half = (w - 8) / 2;
    float add_width = fminf(half, ui_tab_content_width(ICON_PLUS, "Add"));
    float delete_width = fminf(half, ui_tab_content_width(ICON_DELETE, "Delete"));
    float rename_width = fminf(half, ui_tab_content_width(ICON_EDIT, "Rename"));
    float arrow_width = fminf(32, (w - rename_width - 16) / 2);
    smoke_click(x + w - delete_width - 8 - add_width / 2, first + before * 30 + 12);
    ok = ok && app.map->scene.layer_count == before + 1;
    float actions = first + app.map->scene.layer_count * 30;
    smoke_click(x + w - 16 - 2 * arrow_width - rename_width / 2, actions + 32 + 12);
    ok = ok && app.field == FIELD_LAYER;
    strcpy(app.input, "Decoration");
    editor_field_apply();
    ok = ok && !strcmp(app.map->scene.layers[app.layer].name, "Decoration");
    editor_undo_active(false);
    ok = ok && strcmp(app.map->scene.layers[app.layer].name, "Decoration");
    editor_undo_active(true);
    editor_draw();
    smoke_click(x + w - 22, first + app.layer * 30 + 12);
    ok = ok && !app.map->scene.layers[app.layer].visible;
    editor_undo_active(false);
    ok = ok && app.map->scene.layers[app.layer].visible;
    editor_draw();
    smoke_click(x + w - 8 - 1.5f * arrow_width, actions + 44);
    ok = ok && app.layer == before - 1 &&
         !strcmp(app.map->scene.layers[app.layer].name, "Decoration");
    editor_undo_active(false);
    app.layer = before;
    editor_draw();
    smoke_click(x + w - delete_width / 2, actions + 12);
    ok = ok && app.map->scene.layer_count == before;
    editor_undo_active(false);
    app.layer = before;
    editor_draw();
    /* Import and source size belong to Tiles, independently of the selected entity. */
    smoke_click(app.width - PANEL_GAP - 48, TOP + TABS + 82);
    ok = ok && app.field == FIELD_IMPORT;
    editor_field_stop();
    editor_draw();
    smoke_click(right_x + 70, TOP + TABS + 114);
    ok = ok && app.field == FIELD_TILESIZE;
    editor_field_stop();
    editor_draw();
    int old_tile = app.tile;
    smoke_click(right_x + 37 + 16, TOP + TABS + 182);
    ok = ok && app.tile == 2;
    ok = ok && editor_reload_layout() && app.left_tabs.selected == UI_MAP_TAB &&
         app.right_tabs.selected == UI_TILES_TAB;
    editor_tab_select(true);
    editor_tab_select(false);
    ok = ok && app.left_tabs.selected == UI_MAP_TAB && app.right_tabs.selected == UI_TILES_TAB &&
         app.map->scene.layer_count == before + 1 &&
         !strcmp(app.map->scene.layers[app.layer].name, "Decoration");
    editor_undo_active(false);
    ok = ok && strcmp(app.map->scene.layers[app.layer].name, "Decoration");
    pe_history_free(&app.map->history);
    app.map->scene = original;
    app.layer = 0;
    app.tile = old_tile;
    app.left_tabs.selected = UI_EXPLORER_TAB;
    app.right_tabs.selected = UI_INSPECTOR_TAB;
    app.map_view = app.tiles_view = (UiScroll){0};
    editor_selection_sync();
    if (!ok) {
        fprintf(stderr, "Map/Tiles panel smoke failed\n");
    }
    return ok;
}

bool editor_verify_selection(void) {
    PeScene original = app.map->scene;
    PeCamera camera = app.camera;
    pe_history_free(&app.map->history);
    PeEntity parent = pe_entity_create(&app.map->scene, PE_TRANSFORM | PE_SPRITE);
    PeEntity child = pe_entity_create(&app.map->scene, PE_TRANSFORM | PE_SPRITE);
    PeObject *p = pe_entity_get(&app.map->scene, parent);
    PeObject *o = pe_entity_get(&app.map->scene, child);
    if (!p || !o) {
        return false;
    }
    p->x = p->y = 2000;
    o->x = 12;
    o->y = 14;
    bool ok = pe_entity_set_parent(&app.map->scene, child, parent, false);
    app.selected = 0;
    app.left_tabs.selected = UI_SCENE_TAB;
    app.right_tabs.selected = UI_INFO_TAB;
    editor_draw();
    PeScene unchanged = app.map->scene;
    int roots = 0;
    for (int i = 0; i < PE_ENTITIES; i++) {
        if (original.objects[i].alive && !original.objects[i].parent_id) {
            roots++;
        }
    }
    float row_y = TOP + TABS + 48.f + (roots + 1) * 26;
    smoke_click(85, row_y + 12);
    editor_draw();
    float wx = 0, wy = 0;
    ok = ok && app.selected == o->stable_id && app.right_tabs.selected == UI_INSPECTOR_TAB;
    ok = ok && pe_entity_world_position(&app.map->scene, child, &wx, &wy);
    float sx = app.viewport.x + (wx - app.camera.x) * app.camera.zoom;
    float sy = app.viewport.y + (wy - app.camera.y) * app.camera.zoom;
    ok = ok && editor_inside(sx, sy, app.viewport) &&
         !memcmp(&unchanged, &app.map->scene, sizeof unchanged);
    PeCamera focused = app.camera;
    editor_inspect_entity();
    editor_draw();
    ok = ok && !memcmp(&focused, &app.camera, sizeof focused);
    smoke_pointer(SDL_EVENT_MOUSE_BUTTON_DOWN, sx + 2, sy + 2);
    smoke_pointer(SDL_EVENT_MOUSE_MOTION, sx + 2 + 20 * app.camera.zoom, sy + 2);
    smoke_pointer(SDL_EVENT_MOUSE_BUTTON_UP, sx + 2 + 20 * app.camera.zoom, sy + 2);
    ok = ok && !memcmp(&focused, &app.camera, sizeof focused);
    float moved_x = 0, moved_y = 0;
    ok = ok && pe_entity_world_position(&app.map->scene, child, &moved_x, &moved_y) &&
         fabsf(moved_x - wx - 20) < .01f;
    editor_field_start(FIELD_X, "5000");
    editor_field_apply();
    editor_draw();
    ok = ok && pe_entity_world_position(&app.map->scene, child, &wx, &wy);
    sx = app.viewport.x + (wx - app.camera.x) * app.camera.zoom;
    sy = app.viewport.y + (wy - app.camera.y) * app.camera.zoom;
    ok = ok && editor_inside(sx, sy, app.viewport);
    pe_history_free(&app.map->history);
    app.map->scene = original;
    app.camera = camera;
    app.map->reveal_selected = false;
    app.selected = 0;
    app.left_tabs.selected = UI_EXPLORER_TAB;
    app.right_tabs.selected = UI_INSPECTOR_TAB;
    if (!ok) {
        fprintf(stderr, "Entity selection focus smoke failed\n");
    }
    return ok;
}

void editor_capture_component_dropdown(void) {
    PeScene scene = app.map->scene;
    PeCamera camera = app.camera;
    uint32_t selected = app.selected;
    UiId left = app.left_tabs.selected, right = app.right_tabs.selected;
    Tool tool = app.tool;
    for (uint32_t i = 0; i < PE_ENTITIES; i++) {
        PeObject *o = &app.map->scene.objects[i];
        if (!o->alive) {
            continue;
        }
        PeEntity entity = {i, o->generation};
        /* Show one selectable option without changing the real scene or its history. */
        if (o->mask & PE_PLAYER) {
            pe_entity_remove_component(&app.map->scene, entity, PE_PLAYER);
        } else if (o->mask & PE_BODY) {
            pe_entity_remove_component(&app.map->scene, entity, PE_BODY);
        }
        app.selected = o->stable_id;
        app.tool = SELECT;
        app.left_tabs.selected = UI_SCENE_TAB;
        editor_inspect_entity();
        editor_draw();
        SDL_FRect anchor = app.map->component_anchor;
        smoke_click(anchor.x + anchor.w * .5f, anchor.y + 12);
        char path[1200];
        const char *original_path = editor_screenshot_path;
        if (snprintf(path, sizeof path, "%s.dropdown.bmp", original_path) < (int)sizeof path) {
            editor_screenshot_path = path;
            editor_draw();
            editor_screenshot_path = original_path;
        }
        SDL_Event escape = {0};
        escape.type = SDL_EVENT_KEY_DOWN;
        escape.key.key = SDLK_ESCAPE;
        editor_event(&escape);
        editor_draw();
        if (o->mask & PE_TRANSFORM) {
            smoke_click(anchor.x + anchor.w - 40, anchor.y + 33 + 34 + 12);
            SDL_Event end = {0};
            end.type = SDL_EVENT_KEY_DOWN;
            end.key.key = SDLK_END;
            editor_event(&end);
            if (snprintf(path, sizeof path, "%s.input.bmp", original_path) < (int)sizeof path) {
                editor_screenshot_path = path;
                editor_draw();
                editor_screenshot_path = original_path;
            }
            editor_field_stop();
        }
        break;
    }
    app.map->scene = scene;
    app.camera = camera;
    app.selected = selected;
    app.tool = tool;
    app.left_tabs.selected = left;
    app.right_tabs.selected = right;
    app.map->reveal_selected = false;
    app.map->component_add_open = false;
}
