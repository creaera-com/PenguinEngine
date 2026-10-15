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
    float header_x = app.width - app.right_width + 30, header_y = SIDE_TOP + 20;
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
    app.layers_open = app.entity_open = app.palette_open = true;
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
    bool ok = cJSON_GetArraySize(pe_ui_get(app.shell_layout->root, "window_controls")) == 4;
    ok = ok && editor_window_hit_test(width - 1, height - 1) == SDL_HITTEST_RESIZE_BOTTOMRIGHT;
    ok = ok && editor_window_hit_test((int)controls - 20, 20) == SDL_HITTEST_DRAGGABLE;
    ok = ok && editor_window_hit_test((int)controls + 16, 20) == SDL_HITTEST_NORMAL;
    ok = ok && editor_window_hit_test(20, 20) == SDL_HITTEST_NORMAL;
    smoke_click(controls + 16, 24);
    editor_draw();
    SDL_SyncWindow(app.window);
    int collapsed_width, collapsed_height;
    SDL_GetWindowSize(app.window, &collapsed_width, &collapsed_height);
    ok = ok && app.window_collapsed && collapsed_height == TOP;
    ok = ok && editor_window_hit_test(collapsed_width / 2, TOP - 1) == SDL_HITTEST_DRAGGABLE;
    smoke_click(editor_window_controls_left() + 16, 24);
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
