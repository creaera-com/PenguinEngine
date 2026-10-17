#include "editor_smoke.h"
#include "application/internal.h"
#include "editors/game/map/map_editor.h"
#include "editors/graphics/vector/editor.h"
#include "editors/script/script_editor.h"

static void smoke_pointer(Uint32 type, float x, float y);
static void smoke_click(float x, float y);

bool editor_verify_editor(void) {
    editor_draw();
    PeScene baseline = pe_map_active_instance()->scene;
    pe_map_active_instance()->view.tile = 4;
    pe_map_active_instance()->view.tool = BRUSH;
    float sx = app.shell.viewport.x + (2 * pe_map_active_instance()->scene.tile_size + 2 -
                                       pe_map_active_instance()->view.camera.x) *
                                          pe_map_active_instance()->view.camera.zoom;
    float sy = app.shell.viewport.y + (2 * pe_map_active_instance()->scene.tile_size + 2 -
                                       pe_map_active_instance()->view.camera.y) *
                                          pe_map_active_instance()->view.camera.zoom;
    SDL_Event e = {0};
    e.type = SDL_EVENT_MOUSE_BUTTON_DOWN;
    e.button.button = SDL_BUTTON_LEFT;
    e.button.x = sx;
    e.button.y = sy;
    editor_event(&e);
    e.type = SDL_EVENT_MOUSE_MOTION;
    e.motion.x =
        sx + pe_map_active_instance()->scene.tile_size * pe_map_active_instance()->view.camera.zoom;
    e.motion.y = sy;
    e.motion.state = SDL_BUTTON_LMASK;
    editor_event(&e);
    e.type = SDL_EVENT_MOUSE_BUTTON_UP;
    e.button.button = SDL_BUTTON_LEFT;
    e.button.x =
        sx + pe_map_active_instance()->scene.tile_size * pe_map_active_instance()->view.camera.zoom;
    e.button.y = sy;
    editor_event(&e);
    bool ok = pe_map_active_instance()->history.count == 1 &&
              pe_tile_get(&pe_map_active_instance()->scene, 0, 2, 2) == 4 &&
              pe_tile_get(&pe_map_active_instance()->scene, 0, 3, 2) == 4;
    e.type = SDL_EVENT_KEY_DOWN;
    e.key.key = SDLK_Z;
    e.key.mod = SDL_KMOD_CTRL;
    editor_event(&e);
    ok = ok && !memcmp(&pe_map_active_instance()->scene, &baseline, sizeof baseline);
    e.key.mod = SDL_KMOD_CTRL | SDL_KMOD_SHIFT;
    editor_event(&e);
    ok = ok && pe_tile_get(&pe_map_active_instance()->scene, 0, 3, 2) == 4;
    PeScene edited = pe_map_active_instance()->scene;
    e.key.key = SDLK_F5;
    e.key.mod = 0;
    editor_event(&e);
    pe_update(&pe_map_active_instance()->runtime, 1, 0, 1.f / 60);
    ok =
        ok && editor_playing() && !memcmp(&pe_map_active_instance()->scene, &edited, sizeof edited);
    editor_event(&e);
    ok = ok && !editor_playing();
    pe_map_active_instance()->scene = baseline;
    pe_history_free(&pe_map_active_instance()->history);
    pe_map_active_instance()->view.tile = 1;
    if (!ok) {
        fprintf(stderr, "Editor interaction smoke test failed\n");
    }

    return ok;
}

bool editor_verify_script_editor(void) {
    PeScene scene_before = pe_map_active_instance()->scene;
    editor_tab_select(true);
    if (!pe_script_active_instance() || pe_map_active_instance()) {
        return false;
    }

    PeText original = (*pe_script_active_instance()).document;
    pe_text_init(&(*pe_script_active_instance()).document, "");
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
    bool ok = (*pe_script_active_instance()).show_completion;
    editor_draw();
    ok = ok && (*pe_script_active_instance()).popup.w > 0;
    e.key.key = SDLK_RETURN;
    e.key.mod = 0;
    editor_event(&e);
    ok = ok && !strcmp((*pe_script_active_instance()).document.text, "const");
    e.key.key = SDLK_Z;
    e.key.mod = SDL_KMOD_CTRL;
    editor_event(&e);
    ok = ok && !strcmp((*pe_script_active_instance()).document.text, "cons");
    e.key.mod = SDL_KMOD_CTRL | SDL_KMOD_SHIFT;
    editor_event(&e);
    ok = ok && !strcmp((*pe_script_active_instance()).document.text, "const");
    e.key.key = SDLK_BACKSPACE;
    e.key.mod = 0;
    editor_event(&e);
    editor_draw();
    memset(&e, 0, sizeof e);
    e.type = SDL_EVENT_MOUSE_BUTTON_DOWN;
    e.button.button = SDL_BUTTON_LEFT;
    e.button.x = (*pe_script_active_instance()).popup.x + 8;
    e.button.y = (*pe_script_active_instance()).popup.y + 8;
    editor_event(&e);
    ok = ok && !strcmp((*pe_script_active_instance()).document.text, "const");
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
    ok = ok && !(*pe_script_active_instance()).diagnostic.valid &&
         (*pe_script_active_instance()).diagnostic.line == 1;
    editor_request(REQUEST_QUIT, NULL);
    ok = ok && app.shell.request == REQUEST_QUIT && app.runtime.running;
    app.shell.request = REQUEST_NONE;
    se_free(&(*pe_script_active_instance()));
    (*pe_script_active_instance()).document = original;
    (*pe_script_active_instance()).highlighted_revision = UINT64_MAX;
    (*pe_script_active_instance()).checked_revision = UINT64_MAX;
    (*pe_script_active_instance()).show_completion = false;
    (*pe_script_active_instance()).top_line = (*pe_script_active_instance()).left_column = 0;
    se_check(&(*pe_script_active_instance()));
    editor_tab_select(false);
    ok = ok && pe_map_active_instance() && !pe_script_active_instance() &&
         !memcmp(&scene_before, &pe_map_active_instance()->scene, sizeof scene_before);
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
    PeScene original = pe_map_active_instance()->scene;
    editor_tab_select(false);
    ui_cancel(&app.shell.ui);
    editor_draw();
    float x = app.shell.left_width, y = TOP + TABS + 90;
    smoke_pointer(SDL_EVENT_MOUSE_BUTTON_DOWN, x, y);
    smoke_pointer(SDL_EVENT_MOUSE_MOTION, x + 100, y);
    smoke_pointer(SDL_EVENT_MOUSE_BUTTON_UP, x + 100, y);
    bool ok = fabsf(app.shell.left_width - 340) < .01f && app.shell.viewport.x == 340 + PANEL_GAP;
    x = app.platform.width - app.shell.right_width;
    smoke_pointer(SDL_EVENT_MOUSE_BUTTON_DOWN, x, y);
    smoke_pointer(SDL_EVENT_MOUSE_MOTION, x - 80, y);
    smoke_pointer(SDL_EVENT_MOUSE_BUTTON_UP, x - 80, y);
    ok = ok && fabsf(app.shell.right_width - 360) < .01f;
    app.shell.left_tabs.selected = UI_MAP_TAB;
    editor_draw();
    float header_x = 30, header_y = SIDE_TOP + 20;
    smoke_pointer(SDL_EVENT_MOUSE_BUTTON_DOWN, header_x, header_y);
    smoke_pointer(SDL_EVENT_MOUSE_BUTTON_UP, header_x, header_y);
    ok = ok && !pe_map_active_instance()->view.layers_open;
    float tab_x = GLOBAL_NAV_X + 16, tab_y = 24;
    smoke_pointer(SDL_EVENT_MOUSE_BUTTON_DOWN, tab_x, tab_y);
    smoke_pointer(SDL_EVENT_MOUSE_MOTION, tab_x + 60, tab_y);
    smoke_pointer(SDL_EVENT_MOUSE_BUTTON_UP, tab_x + 60, tab_y);
    ok = ok && app.shell.workspaces.items[0].id == UI_SCRIPT_VIEW &&
         app.shell.workspaces.items[1].id == UI_MAP_VIEW && editor_map_active();
    /* Sidebar navigation is independent of workspace tabs and scene history. */
    float side_y = TOP + PANEL_GAP + 12;
    float scene_tab_x = PANEL_GAP + app.shell.left_tabs.items[0].width + 20;
    float info_tab_x =
        app.platform.width - RIGHT + PANEL_GAP + app.shell.right_tabs.items[0].width + 20;
    smoke_pointer(SDL_EVENT_MOUSE_BUTTON_DOWN, scene_tab_x, side_y);
    smoke_pointer(SDL_EVENT_MOUSE_BUTTON_UP, scene_tab_x, side_y);
    smoke_pointer(SDL_EVENT_MOUSE_BUTTON_DOWN, info_tab_x, side_y);
    smoke_pointer(SDL_EVENT_MOUSE_BUTTON_UP, info_tab_x, side_y);
    ok = ok && app.shell.left_tabs.selected == UI_SCENE_TAB &&
         app.shell.right_tabs.selected == UI_INFO_TAB && editor_map_active();
    uint32_t saved_selected = app.shell.selected;
    int saved_tool = pe_map_active_instance()->view.tool;
    for (int i = 0; i < PE_ENTITIES; i++) {
        if (!pe_map_active_instance()->scene.objects[i].alive) {
            continue;
        }
        smoke_pointer(SDL_EVENT_MOUSE_BUTTON_DOWN, 60, SIDE_TOP + 58);
        smoke_pointer(SDL_EVENT_MOUSE_BUTTON_UP, 60, SIDE_TOP + 58);
        ok = ok && app.shell.selected == pe_map_active_instance()->scene.objects[i].stable_id &&
             app.shell.right_tabs.selected == UI_INSPECTOR_TAB;
        break;
    }
    app.shell.selected = saved_selected;
    pe_map_active_instance()->view.tool = saved_tool;
    app.shell.left_tabs.selected = UI_EXPLORER_TAB;
    app.shell.right_tabs.selected = UI_INSPECTOR_TAB;
    editor_tab_select(true);
    PeText saved = (*pe_script_active_instance()).document;
    char lines[4096] = "";
    for (int i = 0; i < 100; i++) {
        strcat(lines, "// scrollable line\n");
    }
    pe_text_init(&(*pe_script_active_instance()).document, lines);
    (*pe_script_active_instance()).extent_revision =
        (*pe_script_active_instance()).highlighted_revision =
            (*pe_script_active_instance()).checked_revision = UINT64_MAX;
    (*pe_script_active_instance()).top_line = 0;
    editor_tab_select(true);
    editor_draw();
    float scroll_x = app.shell.viewport.x + app.shell.viewport.w - 8,
          scroll_y = (*pe_script_active_instance()).body.y + 8;
    smoke_pointer(SDL_EVENT_MOUSE_BUTTON_DOWN, scroll_x, scroll_y);
    smoke_pointer(SDL_EVENT_MOUSE_MOTION, scroll_x, scroll_y + 120);
    smoke_pointer(SDL_EVENT_MOUSE_BUTTON_UP, scroll_x, scroll_y + 120);
    ok = ok && (*pe_script_active_instance()).top_line > 0 &&
         !strcmp((*pe_script_active_instance()).document.text, lines) &&
         (*pe_script_active_instance()).document.count == 0;
    se_free(&(*pe_script_active_instance()));
    (*pe_script_active_instance()).document = saved;
    (*pe_script_active_instance()).extent_revision =
        (*pe_script_active_instance()).highlighted_revision =
            (*pe_script_active_instance()).checked_revision = UINT64_MAX;
    (*pe_script_active_instance()).top_line = (*pe_script_active_instance()).left_column = 0;
    se_check(&(*pe_script_active_instance()));
    editor_tab_select(false);
    ok = ok && !memcmp(&original, &pe_map_active_instance()->scene, sizeof original);
    app.shell.left_width = 240;
    app.shell.right_width = 280;
    pe_map_active_instance()->view.layers_open = pe_map_active_instance()->view.palette_open = true;
    app.shell.tree_view.offset = app.shell.inspector_view.offset = 0;
    UiTab tabs[UI_MAX_TABS];
    for (int i = 0; i < editor_view_count; i++) {
        tabs[i] = (UiTab){editor_views[i]->id, true, 0};
    }
    ui_tabs_init(&app.shell.workspaces, tabs, editor_view_count, UI_MAP_VIEW);
    ui_cancel(&app.shell.ui);
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
    PeScene original = pe_map_active_instance()->scene;
    int history_cursor = pe_map_active_instance()->history.cursor;
    uint64_t revision = 0;
    editor_draw();
    SDL_Texture *nav_image = pe_ui_images_get(&app.shell.shell_images, app.platform.renderer,
                                              app.shell.shell_layout, "icons/grid.png");
    PeUiImageEntry *shared_images = app.shell.shell_images.entries;
    bool grid = pe_map_active_instance()->view.grid;
    PeCamera camera = pe_map_active_instance()->view.camera;
    editor_tab_select(false);
    editor_draw();
    float action_y = TOP + PANEL_GAP + 18;
    smoke_click(30, 24);
    bool menu_unchanged =
        editor_map_active() && app.shell.request == REQUEST_NONE && app.shell.field == FIELD_NONE;
    smoke_click(GLOBAL_NAV_X + app.shell.workspaces.items[0].width + 16, 24);
    bool ok = nav_image && shared_images == app.shell.shell_images.entries &&
              nav_image == pe_ui_images_get(&app.shell.shell_images, app.platform.renderer,
                                            app.shell.shell_layout, "icons/grid.png") &&
              editor_yaml_action_enabled(NULL) && menu_unchanged && editor_scripts_active() &&
              !pe_map_active_instance();
    if (!pe_script_active_instance()) {
        return false;
    }
    revision = (*pe_script_active_instance()).document.revision;
    (*pe_script_active_instance()).checked_revision = UINT64_MAX;
    smoke_click(LEFT + PANEL_GAP + 8 + 16, action_y);
    ok = ok && editor_scripts_active() &&
         (*pe_script_active_instance()).checked_revision == revision;
    /* Global Undo/Redo must operate on the active script, never the scene. */
    PeText saved_document = (*pe_script_active_instance()).document;
    pe_text_init(&(*pe_script_active_instance()).document, "// toolbar test");
    pe_text_insert(&(*pe_script_active_instance()).document, "// edited\n");
    se_changed(&(*pe_script_active_instance()), app.shell.viewport, false);
    editor_draw();
    smoke_click(150, 24);
    ok = ok && (*pe_script_active_instance()).document.position == 0 &&
         !strcmp((*pe_script_active_instance()).document.text, "// toolbar test");
    smoke_click(188, 24);
    ok = ok && (*pe_script_active_instance()).document.position == 1 &&
         strstr((*pe_script_active_instance()).document.text, "edited") != NULL;
    pe_text_free(&(*pe_script_active_instance()).document);
    (*pe_script_active_instance()).document = saved_document;
    (*pe_script_active_instance()).extent_revision =
        (*pe_script_active_instance()).highlighted_revision =
            (*pe_script_active_instance()).checked_revision = UINT64_MAX;
    se_check(&(*pe_script_active_instance()));
    SDL_Event f5 = {0};
    f5.type = SDL_EVENT_KEY_DOWN;
    f5.key.key = SDLK_F5;
    editor_event(&f5);
    ok = ok && !editor_playing();
    bool document_unchanged = (*pe_script_active_instance()).document.revision == revision;
    smoke_click(GLOBAL_NAV_X + 16, 24);
    ok = ok && editor_map_active() && !pe_script_active_instance() && document_unchanged;
    /* Narrow the center until the last map actions wrap to a second row. */
    app.shell.left_width = 400;
    app.shell.right_width = app.platform.width - app.shell.left_width - app.shell.min_center;
    editor_draw();
    ok = ok && app.shell.viewport.y == TOP + 2 * PANEL_GAP + 70;
    int columns = (int)((app.shell.viewport.w - 10) / 38);
    int grid_index = editor_view(UI_MAP_VIEW)->action_count - 1;
    smoke_click(LEFT + PANEL_GAP + 8 + (grid_index % columns) * 38 + 16,
                action_y + (grid_index / columns) * 32);
    ok = ok && pe_map_active_instance()->view.grid != grid &&
         !memcmp(&original, &pe_map_active_instance()->scene, sizeof original) &&
         pe_map_active_instance()->history.cursor == history_cursor &&
         !memcmp(&camera, &pe_map_active_instance()->view.camera, sizeof camera);
    pe_map_active_instance()->view.grid = grid;
    app.shell.left_width = 240;
    app.shell.right_width = 280;
    ui_cancel(&app.shell.ui);
    editor_tab_select(false);
    editor_draw();
    if (!ok) {
        fprintf(stderr, "Workspace navigation / contextual toolbar smoke test failed\n");
    }

    return ok;
}

bool editor_verify_lifecycle(void) {
    char error[256], map_backup[1300], script_backup[1300], original_ui[1024], test_ui[1024];
    snprintf(map_backup, sizeof map_backup, "%s.baseline", app.workspace.session_paths[0]);
    snprintf(script_backup, sizeof script_backup, "%s.baseline", app.workspace.session_paths[1]);
    snprintf(original_ui, sizeof original_ui, "%s", app.shell.ui_path);
    if (strlen(app.shell.ui_path) + 7 >= sizeof test_ui) {
        return false;
    }
    memcpy(test_ui, app.shell.ui_path, strlen(app.shell.ui_path));
    strcpy(test_ui + strlen(app.shell.ui_path), ".smoke");
    UiId id = UI_MAP_VIEW;
    editor_tab_select(false);
    editor_draw();
    if (!pe_map_active_instance() ||
        !pe_session_save(map_backup, id, editor_checkpoint, &id, error, sizeof error)) {
        return false;
    }

    bool ok = editor_begin();
    if (ok) {
        pe_tile_set(&pe_map_active_instance()->scene, 0, 1, 1, 7);
        editor_commit();
    }
    pe_map_active_instance()->view.camera = (PeCamera){11, -7, 2.5f};
    PeScene edited = pe_map_active_instance()->scene;
    uint64_t map_revision = pe_map_active_instance()->history.revision;
    char original_cache[1200];
    snprintf(original_cache, sizeof original_cache, "%s", app.workspace.session_paths[0]);
    snprintf(app.workspace.session_paths[0], sizeof app.workspace.session_paths[0],
             "missing-session-directory/map.cache");
    editor_select_workspace(UI_SCRIPT_VIEW);
    ok = ok && editor_map_active() && pe_map_active_instance() && !pe_script_active_instance() &&
         pe_map_active_instance()->history.revision == map_revision;
    snprintf(app.workspace.session_paths[0], sizeof app.workspace.session_paths[0], "%s",
             original_cache);
    editor_select_workspace(UI_SCRIPT_VIEW);
    editor_draw();
    if (!pe_script_active_instance() || pe_map_active_instance()) {
        return false;
    }
    ok = ok && app.workspace.session_dirty[0];
    id = UI_SCRIPT_VIEW;
    if (!pe_session_save(script_backup, id, editor_checkpoint, &id, error, sizeof error)) {
        return false;
    }
    pe_text_insert(&pe_script_active_instance()->document, "// unsaved lifecycle test\n");
    se_changed(pe_script_active_instance(), app.shell.viewport, false);
    uint64_t script_revision = pe_script_active_instance()->document.revision;
    int script_position = pe_script_active_instance()->document.position;
    editor_select_workspace(UI_MAP_VIEW);
    editor_draw();
    if (!pe_map_active_instance() || pe_script_active_instance()) {
        return false;
    }
    ok = ok && app.workspace.session_dirty[1] &&
         !memcmp(&edited, &pe_map_active_instance()->scene, sizeof edited) &&
         pe_map_active_instance()->history.revision == map_revision &&
         pe_map_active_instance()->view.camera.x == 11 &&
         pe_map_active_instance()->view.camera.zoom == 2.5f;
    editor_undo_active(false);
    ok = ok && pe_tile_get(&pe_map_active_instance()->scene, 0, 1, 1) != 7;
    editor_undo_active(true);
    ok = ok && pe_tile_get(&pe_map_active_instance()->scene, 0, 1, 1) == 7;
    /* Corrupt a target checkpoint: the source must be restored, never discarded. */
    FILE *f = fopen(app.workspace.session_paths[1], "r+b");
    if (!f) {
        return false;
    }
    fseek(f, -1, SEEK_END);
    int byte = fgetc(f);
    fseek(f, -1, SEEK_END);
    fputc(byte ^ 255, f);
    fclose(f);
    editor_select_workspace(UI_SCRIPT_VIEW);
    ok = ok && editor_map_active() && pe_map_active_instance() && !pe_script_active_instance() &&
         pe_map_active_instance()->history.revision == map_revision;
    f = fopen(app.workspace.session_paths[1], "r+b");
    if (!f) {
        return false;
    }
    fseek(f, -1, SEEK_END);
    fputc(byte, f);
    fclose(f);
    editor_select_workspace(UI_SCRIPT_VIEW);
    editor_draw();
    if (!pe_script_active_instance() || pe_map_active_instance()) {
        return false;
    }
    ok = ok && pe_script_active_instance()->document.revision == script_revision &&
         pe_script_active_instance()->document.position == script_position;
    editor_undo_active(false);
    ok = ok && pe_script_active_instance()->document.position == script_position - 1;
    editor_undo_active(true);
    ok = ok && pe_script_active_instance()->document.position == script_position;
    editor_request(REQUEST_QUIT, NULL);
    ok = ok && app.shell.request == REQUEST_QUIT && app.runtime.running;
    app.shell.request = REQUEST_NONE;
    editor_select_workspace(UI_MAP_VIEW);
    editor_draw();
    /* A valid reload changes layout/theme and reinstantiates the same dirty document. */
    cJSON *root = cJSON_Duplicate(app.shell.shell_layout->root, true);
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
    snprintf(app.shell.ui_path, sizeof app.shell.ui_path, "%s", test_ui);
    ok = editor_reload_layout() && ok;
    ok = ok && UI_ACCENT == 0xb08968ffu && app.shell.gap == 2 && pe_map_active_instance() &&
         pe_map_active_instance()->history.revision == map_revision;
    MapPayload *before = pe_map_active_instance();
    f = fopen(test_ui, "wb");
    if (!f) {
        return false;
    }
    fputs("broken: [", f);
    fclose(f);
    ok = ok && !editor_reload_layout() && pe_map_active_instance() == before &&
         UI_ACCENT == 0xb08968ffu;
    snprintf(app.shell.ui_path, sizeof app.shell.ui_path, "%s", original_ui);
    ok = editor_reload_layout() && ok;
    remove(test_ui);
    /* Restore the test baseline without writing any project files. */
    editor_select_workspace(UI_SCRIPT_VIEW);
    if (!pe_script_active_instance()) {
        return false;
    }
    se_free(pe_script_active_instance());
    memset(pe_script_active_instance(), 0, sizeof *pe_script_active_instance());
    id = UI_SCRIPT_VIEW;
    ok = pe_session_load(script_backup, id, editor_checkpoint, &id, error, sizeof error) && ok;
    se_check(pe_script_active_instance());
    editor_select_workspace(UI_MAP_VIEW);
    if (!pe_map_active_instance()) {
        return false;
    }
    pe_assets_free(&pe_map_active_instance()->assets);
    pe_history_free(&pe_map_active_instance()->history);
    memset(pe_map_active_instance(), 0, sizeof *pe_map_active_instance());
    id = UI_MAP_VIEW;
    ok = pe_session_load(map_backup, id, editor_checkpoint, &id, error, sizeof error) && ok;
    editor_assets_sync();
    remove(map_backup);
    remove(script_backup);
    app.workspace.session_dirty[0] = editor_map_dirty();
    app.shell.mx = app.shell.my = 0;
    ui_cancel(&app.shell.ui);
    editor_draw();
    if (!ok) {
        fprintf(stderr, "Editor lifecycle / checkpoint / YAML reload smoke failed\n");
    }

    return ok;
}

bool editor_verify_console(void) {
    editor_tab_select(false);
    editor_draw();
    if (!app.shell.console) {
        return false;
    }
    PeConsole *service = app.shell.console;
    PeTerminal *terminal = pe_console_terminal(service);
    PeScene baseline = pe_map_active_instance()->scene;
    SDL_FRect body = pe_console_area(service);
    float old_height = body.h;
    int state = -1;
    for (int i = 0; i < app.shell.widget_count; i++) {
        if (app.shell.widgets[i].kind == 1) {
            state = i;
            break;
        }
    }
    if (state < 0) {
        return false;
    }
    float extent = app.shell.widgets[state].extent;
    float x = app.shell.viewport.x + app.shell.viewport.w * .5f,
          y = app.shell.viewport.y + app.shell.viewport.h + 4;
    smoke_pointer(SDL_EVENT_MOUSE_BUTTON_DOWN, x, y);
    editor_draw();
    smoke_pointer(SDL_EVENT_MOUSE_MOTION, x, y - 32);
    editor_draw();
    smoke_pointer(SDL_EVENT_MOUSE_BUTTON_UP, x, y - 32);
    editor_draw();
    bool ok = app.shell.widgets[state].extent > extent + 20 &&
              pe_console_area(service).h > old_height + 20;
    ok = ok && editor_reload_layout();
    editor_draw();
    ok = ok && app.shell.console == service && pe_console_terminal(service) == terminal;
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
         !memcmp(&baseline, &pe_map_active_instance()->scene, sizeof baseline);
    pe_terminal_consume(terminal, length);
    pe_terminal_feed(terminal, "shared-session", 14);
    editor_tab_select(true);
    editor_draw();
    ok = ok && app.shell.console == service && pe_console_terminal(app.shell.console) == terminal &&
         !pe_map_active_instance() && pe_script_active_instance();
    char *copy = pe_terminal_copy(terminal, 0, 0, 0, 13);
    ok = ok && copy && !strcmp(copy, "shared-session");
    free(copy);
    size_t script_length = pe_script_active_instance()->document.length;
    body = pe_console_area(service);
    smoke_click(body.x + 10, body.y + 8);
    editor_draw();
    event.type = SDL_EVENT_TEXT_INPUT;
    event.text.text = "x";
    editor_event(&event);
    ok = ok && pe_script_active_instance()->document.length == script_length;
    pe_terminal_output(terminal, &length);
    pe_terminal_consume(terminal, length);
    editor_tab_select(false);
    editor_draw();
    state = -1;
    for (int i = 0; i < app.shell.widget_count; i++) {
        if (app.shell.widgets[i].kind == 1) {
            state = i;
            break;
        }
    }
    ok = ok && state >= 0 && app.shell.widgets[state].extent > extent + 20;
    if (state >= 0) {
        app.shell.widgets[state].extent = extent;
    }
    body = pe_console_area(service);
    smoke_click(app.shell.viewport.x + 8, TOP + 8);
    ui_cancel(&app.shell.ui);
    const char *reset = "\033[2J\033[H";
    pe_terminal_feed(terminal, reset, strlen(reset));
    app.shell.mx = app.shell.my = 0;
    editor_draw();
    if (!ok) {
        fprintf(stderr, "Console / split layout / focus / shared session smoke failed\n");
    }
    return ok;
}

bool editor_verify_window(void) {
    editor_draw();
    int width = app.platform.width, height = app.platform.height;
    float controls = editor_window_controls_left();
    bool ok = cJSON_GetArraySize(pe_ui_get(app.shell.shell_layout->root, "window_controls")) == 3;
    ok = ok && editor_window_hit_test(width - 1, height - 1) == SDL_HITTEST_RESIZE_BOTTOMRIGHT;
    ok = ok && editor_window_hit_test((int)controls - 20, 20) == SDL_HITTEST_DRAGGABLE;
    ok = ok && editor_window_hit_test((int)controls + 16, 20) == SDL_HITTEST_NORMAL;
    ok = ok && editor_window_hit_test(20, 20) == SDL_HITTEST_NORMAL;
    editor_window_action("window.collapse");
    editor_draw();
    SDL_SyncWindow(app.platform.window);
    int collapsed_width, collapsed_height;
    SDL_GetWindowSize(app.platform.window, &collapsed_width, &collapsed_height);
    ok = ok && app.platform.window_collapsed && collapsed_height == TOP;
    ok = ok && editor_window_hit_test(collapsed_width / 2, TOP - 1) == SDL_HITTEST_DRAGGABLE;
    editor_window_action("window.collapse");
    editor_draw();
    SDL_SyncWindow(app.platform.window);
    int restored_width, restored_height;
    SDL_GetWindowSize(app.platform.window, &restored_width, &restored_height);
    ok = ok && !app.platform.window_collapsed && restored_width == width &&
         restored_height == height;
    ok = ok && !editor_window_action("window.unknown");
    smoke_pointer(SDL_EVENT_MOUSE_MOTION, 500, 400);
    if (!ok) {
        fprintf(stderr, "Window chrome smoke failed\n");
    }
    return ok;
}

bool editor_verify_scene_graph(void) {
    PeScene original = pe_map_active_instance()->scene;
    pe_history_free(&pe_map_active_instance()->history);
    pe_scene_init(&pe_map_active_instance()->scene, 16, 16, 16);
    snprintf(pe_map_active_instance()->scene.tileset, PE_PATH, "%s", original.tileset);
    pe_map_active_instance()->scene.tileset_tile_size = original.tileset_tile_size;
    pe_map_active_instance()->collapsed_count = 0;
    app.shell.left_tabs.selected = UI_SCENE_TAB;
    app.shell.selected = 0;
    editor_draw();
    bool ok = !editor_yaml_action_enabled("scene.add_child");
    float tools_y = TOP + TABS + 18.f;
    float tools_x = LEFT - PANEL_GAP - 8 - (4 * 32 + 3 * 6);
    smoke_click(tools_x + 16, tools_y);
    uint32_t root = app.shell.selected;
    ok = ok && root != 0 && pe_map_active_instance()->history.count == 1;
    smoke_click(tools_x + 92, tools_y);
    uint32_t child = app.shell.selected;
    PeObject *o = pe_entity_get(&pe_map_active_instance()->scene,
                                pe_entity_find(&pe_map_active_instance()->scene, child));
    ok = ok && o && o->parent_id == root;
    smoke_click(tools_x + 54, tools_y);
    uint32_t sibling = app.shell.selected;
    o = pe_entity_get(&pe_map_active_instance()->scene,
                      pe_entity_find(&pe_map_active_instance()->scene, sibling));
    ok = ok && o && o->parent_id == root;
    float first_y = TOP + TABS + 48.f;
    smoke_click(80, first_y + 36);
    ok = ok && app.shell.selected == child;
    smoke_click(tools_x + 92, tools_y);
    uint32_t leaf = app.shell.selected;
    o = pe_entity_get(&pe_map_active_instance()->scene,
                      pe_entity_find(&pe_map_active_instance()->scene, leaf));
    ok = ok && o && o->parent_id == child;
    editor_draw();
    ok = ok && app.shell.scene_view.content == 4 * 26;
    smoke_click(23, first_y + 10);
    editor_draw();
    ok = ok && app.shell.scene_view.content == 26;
    smoke_click(23, first_y + 10);
    editor_draw();
    ok = ok && app.shell.scene_view.content == 4 * 26;
    /* Drag the root in world space and verify inherited motion of the leaf. */
    smoke_click(65, first_y + 10);
    float before_x = 0, before_y = 0;
    ok = ok && pe_entity_world_position(&pe_map_active_instance()->scene,
                                        pe_entity_find(&pe_map_active_instance()->scene, leaf),
                                        &before_x, &before_y);
    float sx = app.shell.viewport.x + (66 - pe_map_active_instance()->view.camera.x) *
                                          pe_map_active_instance()->view.camera.zoom;
    float sy = app.shell.viewport.y + (66 - pe_map_active_instance()->view.camera.y) *
                                          pe_map_active_instance()->view.camera.zoom;
    smoke_pointer(SDL_EVENT_MOUSE_BUTTON_DOWN, sx, sy);
    smoke_pointer(SDL_EVENT_MOUSE_MOTION, sx + 32 * pe_map_active_instance()->view.camera.zoom,
                  sy + 16 * pe_map_active_instance()->view.camera.zoom);
    smoke_pointer(SDL_EVENT_MOUSE_BUTTON_UP, sx + 32 * pe_map_active_instance()->view.camera.zoom,
                  sy + 16 * pe_map_active_instance()->view.camera.zoom);
    float x = 0, y = 0;
    ok = ok &&
         pe_entity_world_position(&pe_map_active_instance()->scene,
                                  pe_entity_find(&pe_map_active_instance()->scene, leaf), &x, &y);
    ok = ok && fabsf(x - before_x - 32) < .01f && fabsf(y - before_y - 16) < .01f;
    editor_undo_active(false);
    ok = ok &&
         pe_entity_world_position(&pe_map_active_instance()->scene,
                                  pe_entity_find(&pe_map_active_instance()->scene, leaf), &x, &y);
    ok = ok && fabsf(x - before_x) < .01f;
    editor_undo_active(true);
    smoke_click(tools_x + 130, tools_y);
    ok = ok && !pe_entity_valid(&pe_map_active_instance()->scene,
                                pe_entity_find(&pe_map_active_instance()->scene, leaf));
    editor_undo_active(false);
    ok = ok && pe_entity_valid(&pe_map_active_instance()->scene,
                               pe_entity_find(&pe_map_active_instance()->scene, leaf));
    editor_draw();
    /* Collapse the child and persist both hierarchy and tree state across F6 and unload. */
    smoke_click(35, first_y + 36);
    editor_draw();
    ok = ok && pe_map_active_instance()->collapsed_count == 1 &&
         pe_map_active_instance()->collapsed[0] == child;
    ok = ok && editor_reload_layout();
    editor_tab_select(true);
    ok = ok && pe_map_active_instance() == NULL;
    editor_tab_select(false);
    ok = ok && pe_map_active_instance() && pe_map_active_instance()->collapsed_count == 1 &&
         pe_map_active_instance()->collapsed[0] == child;
    if (pe_map_active_instance()) {
        o = pe_entity_get(&pe_map_active_instance()->scene,
                          pe_entity_find(&pe_map_active_instance()->scene, leaf));
        ok = ok && o && o->parent_id == child;
        pe_history_free(&pe_map_active_instance()->history);
        pe_map_active_instance()->scene = original;
        pe_map_active_instance()->collapsed_count = 0;
    }
    app.shell.selected = 0;
    app.shell.left_tabs.selected = UI_EXPLORER_TAB;
    pe_map_active_instance()->view.tool = BRUSH;
    editor_selection_sync();
    if (!ok) {
        fprintf(stderr, "Scene graph smoke failed\n");
    }
    return ok;
}

bool editor_verify_components(void) {
    PeScene original = pe_map_active_instance()->scene;
    pe_history_free(&pe_map_active_instance()->history);
    pe_scene_init(&pe_map_active_instance()->scene, 16, 16, 16);
    snprintf(pe_map_active_instance()->scene.tileset, PE_PATH, "%s", original.tileset);
    pe_map_active_instance()->scene.tileset_tile_size = original.tileset_tile_size;
    PeEntity entity = pe_entity_create(&pe_map_active_instance()->scene, 0);
    uint32_t id = pe_map_active_instance()->scene.objects[entity.index].stable_id;
    app.shell.selected = id;
    memset(pe_map_active_instance()->component_ids, 0,
           sizeof pe_map_active_instance()->component_ids);
    memset(pe_map_active_instance()->component_closed, 0,
           sizeof pe_map_active_instance()->component_closed);
    editor_inspect_entity();
    editor_draw();
    float x = app.platform.width - RIGHT + PANEL_GAP + 9;
    float top = TOP + TABS + 67.f + 22;
    float remove_x = x + (RIGHT - 2 * PANEL_GAP - 32) - 11;
    float content_before = app.shell.inspector_view.content;
    smoke_click(x + 90, top + 12);
    bool ok = pe_map_active_instance()->component_add_open &&
              app.shell.inspector_view.content == content_before;
    SDL_Event key = {0};
    key.type = SDL_EVENT_KEY_DOWN;
    key.key.key = SDLK_ESCAPE;
    editor_event(&key);
    editor_draw();
    ok = ok && !pe_map_active_instance()->component_add_open;
    PeScene before_popup = pe_map_active_instance()->scene;
    smoke_click(x + 90, top + 12);
    smoke_click(app.shell.viewport.x + 40, app.shell.viewport.y + 40);
    ok = ok && !pe_map_active_instance()->component_add_open && app.shell.selected == id &&
         !memcmp(&before_popup, &pe_map_active_instance()->scene, sizeof before_popup);
    smoke_click(x + 90, top + 12);
    key.key.key = SDLK_DOWN;
    editor_event(&key);
    key.key.key = SDLK_RETURN;
    editor_event(&key);
    editor_draw();
    ok = ok &&
         pe_map_active_instance()->scene.objects[entity.index].mask == (PE_TRANSFORM | PE_SPRITE);
    editor_undo_active(false);
    editor_draw();
    smoke_click(x + 90, top + 12);
    /* Empty entity: the fourth choice is Player, which also adds Transform + Body. */
    smoke_click(x + 90, top + 33 + 3 * 28 + 12);
    PeObject *o = pe_entity_get(&pe_map_active_instance()->scene, entity);
    ok = ok && o && o->mask == (PE_TRANSFORM | PE_BODY | PE_PLAYER) &&
         pe_map_active_instance()->history.count == 1;
    top -= 22;
    smoke_click(x + 90, top + 12);
    smoke_click(x + 90, top + 33 + 28 + 12);
    ok = ok && o->mask == 15 && !pe_map_active_instance()->component_add_open;
    smoke_click(x + 90, top + 12);
    smoke_click(x + 90, top + 33 + 12);
    ok = ok && pe_map_active_instance()->component_add_open && o->mask == 15;
    key.key.key = SDLK_ESCAPE;
    editor_event(&key);
    editor_draw();
    float transform = top + 33, sprite = transform + 164, body = sprite + 77, player = body + 106;
    smoke_click(x + 80, transform + 12);
    ok = ok && (pe_map_active_instance()->component_closed[entity.index] & PE_TRANSFORM);
    smoke_click(x + 80, transform + 12);
    ok = ok && !(pe_map_active_instance()->component_closed[entity.index] & PE_TRANSFORM);
    uint64_t revision = pe_map_active_instance()->history.revision;
    smoke_click(remove_x, transform + 10);
    smoke_click(remove_x, body + 10);
    ok = ok && o->mask == 15 && pe_map_active_instance()->history.revision == revision;
    smoke_click(remove_x, sprite + 10);
    ok = ok && !(o->mask & PE_SPRITE);
    editor_undo_active(false);
    ok = ok && (o->mask & PE_SPRITE);
    editor_draw();
    smoke_click(x + 20, transform + 34 + 12);
    ok = ok && app.shell.field == FIELD_NONE;
    smoke_click(x + 180, transform + 34 + 12);
    ok = ok && app.shell.field == FIELD_X;
    ok = ok && app.shell.field_inline && app.shell.ui.focus == app.shell.field_id;
    SDL_Event text = {0};
    text.type = SDL_EVENT_TEXT_INPUT;
    text.text.text = "-12.5";
    editor_event(&text);
    ok = ok && !strcmp(app.shell.input, "-12.5") && o->x == 0;
    key.key.key = SDLK_LEFT;
    editor_event(&key);
    key.key.key = SDLK_DELETE;
    editor_event(&key);
    text.text.text = "25";
    editor_event(&text);
    ok = ok && !strcmp(app.shell.input, "-12.25") && o->x == 0;
    key.key.key = SDLK_RETURN;
    editor_event(&key);
    ok = ok && app.shell.field == FIELD_NONE && o->x == -12.25f;
    editor_undo_active(false);
    ok = ok && o->x == 0;
    editor_draw();
    smoke_click(x + 180, transform + 34 + 12);
    text.text.text = "99";
    editor_event(&text);
    key.key.key = SDLK_ESCAPE;
    editor_event(&key);
    ok = ok && app.shell.field == FIELD_NONE && o->x == 0;
    editor_draw();
    smoke_click(x + 180, transform + 34 + 12);
    text.text.text = "8";
    editor_event(&text);
    smoke_click(x + 20, sprite + 34 + 12);
    ok = ok && app.shell.field == FIELD_NONE && o->x == 8;
    editor_undo_active(false);
    editor_draw();
    smoke_click(x + 20, sprite + 34 + 12);
    ok = ok && app.shell.field == FIELD_NONE;
    smoke_click(x + 180, sprite + 34 + 12);
    ok = ok && app.shell.field == FIELD_COLOR;
    uint32_t color = o->color;
    strcpy(app.shell.input, "nothex");
    editor_field_apply();
    ok = ok && app.shell.field == FIELD_COLOR && app.shell.field_inline &&
         app.shell.field_invalid && o->color == color;
    smoke_click(app.shell.viewport.x + 40, app.shell.viewport.y + 40);
    ok = ok && app.shell.field == FIELD_COLOR && app.shell.selected == id && o->color == color;
    strcpy(app.shell.input, "#12345678");
    editor_field_apply();
    ok = ok && app.shell.field == FIELD_NONE && o->color == 0x12345678;
    editor_undo_active(false);
    ok = ok && o->color == color;
    editor_undo_active(true);
    editor_draw();
    smoke_click(x + 180, body + 34 + 12);
    ok = ok && app.shell.field == FIELD_VX;
    strcpy(app.shell.input, "25.5");
    editor_field_apply();
    ok = ok && app.shell.field == FIELD_NONE && o->vx == 25.5f;
    editor_draw();
    smoke_click(remove_x, player + 10);
    ok = ok && !(o->mask & PE_PLAYER);
    smoke_click(remove_x, body + 10);
    ok = ok && !(o->mask & PE_BODY);
    editor_undo_active(false);
    editor_undo_active(false);
    ok = ok && o->mask == 15 && o->vx == 25.5f;
    editor_draw();
    smoke_click(x + 80, sprite + 12);
    ok = ok && (pe_map_active_instance()->component_closed[entity.index] & PE_SPRITE);
    smoke_click(x + 90, TOP + TABS + 44);
    ok = ok && app.shell.field == FIELD_NAME && app.shell.field_inline;
    text.text.text = "Hero";
    editor_event(&text);
    key.key.key = SDLK_RETURN;
    editor_event(&key);
    ok = ok && !strcmp(o->name, "Hero") && o->stable_id == id;
    editor_undo_active(false);
    ok = ok && !strcmp(o->name, "Entity 1");
    editor_undo_active(true);
    editor_draw();
    smoke_click(x + 90, TOP + TABS + 44);
    text.text.text = "   ";
    editor_event(&text);
    editor_field_apply();
    ok = ok && app.shell.field == FIELD_NAME && app.shell.field_invalid && !strcmp(o->name, "Hero");
    key.key.key = SDLK_ESCAPE;
    editor_event(&key);
    ok = ok && editor_reload_layout();
    editor_tab_select(true);
    editor_tab_select(false);
    o = pe_entity_get(&pe_map_active_instance()->scene,
                      pe_entity_find(&pe_map_active_instance()->scene, id));
    ok = ok && o && !strcmp(o->name, "Hero");
    ok = ok && o && o->mask == 15 && o->color == 0x12345678 && o->vx == 25.5f &&
         (pe_map_active_instance()->component_closed[entity.index] & PE_SPRITE);
    editor_play_toggle();
    ok = ok && !editor_component_change(PE_PLAYER, false);
    editor_play_toggle();
    pe_history_free(&pe_map_active_instance()->history);
    pe_map_active_instance()->scene = original;
    memset(pe_map_active_instance()->component_ids, 0,
           sizeof pe_map_active_instance()->component_ids);
    memset(pe_map_active_instance()->component_closed, 0,
           sizeof pe_map_active_instance()->component_closed);
    pe_map_active_instance()->component_add_open = false;
    app.shell.selected = 0;
    pe_map_active_instance()->view.layers_open = pe_map_active_instance()->view.palette_open = true;
    app.shell.inspector_view.offset = 0;
    editor_selection_sync();
    if (!ok) {
        fprintf(stderr, "Component inspector smoke failed\n");
    }
    return ok;
}

static bool smoke_panel_tab(bool right, UiId id) {
    UiTabs *tabs = right ? &app.shell.right_tabs : &app.shell.left_tabs;
    UiRect bar = {right ? app.platform.width - RIGHT + PANEL_GAP : PANEL_GAP, TOP + PANEL_GAP,
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
    PeScene original = pe_map_active_instance()->scene;
    pe_history_free(&pe_map_active_instance()->history);
    pe_map_active_instance()->view.layers_open = pe_map_active_instance()->view.palette_open = true;
    editor_draw();
    bool ok = smoke_panel_tab(false, UI_MAP_TAB) && smoke_panel_tab(true, UI_TILES_TAB);
    float x = PANEL_GAP + 9, w = LEFT - 2 * PANEL_GAP - 32;
    float right_x = app.platform.width - RIGHT + PANEL_GAP + 9;
    float first = TOP + TABS + 72.f;
    int before = pe_map_active_instance()->scene.layer_count;
    float half = (w - 8) / 2;
    float add_width = fminf(half, ui_tab_content_width(ICON_PLUS, "Add"));
    float delete_width = fminf(half, ui_tab_content_width(ICON_DELETE, "Delete"));
    float rename_width = fminf(half, ui_tab_content_width(ICON_EDIT, "Rename"));
    float arrow_width = fminf(32, (w - rename_width - 16) / 2);
    smoke_click(x + w - delete_width - 8 - add_width / 2, first + before * 30 + 12);
    ok = ok && pe_map_active_instance()->scene.layer_count == before + 1;
    float actions = first + pe_map_active_instance()->scene.layer_count * 30;
    smoke_click(x + w - 16 - 2 * arrow_width - rename_width / 2, actions + 32 + 12);
    ok = ok && app.shell.field == FIELD_LAYER;
    strcpy(app.shell.input, "Decoration");
    editor_field_apply();
    ok = ok &&
         !strcmp(pe_map_active_instance()->scene.layers[pe_map_active_instance()->view.layer].name,
                 "Decoration");
    editor_undo_active(false);
    ok = ok &&
         strcmp(pe_map_active_instance()->scene.layers[pe_map_active_instance()->view.layer].name,
                "Decoration");
    editor_undo_active(true);
    editor_draw();
    smoke_click(x + w - 22, first + pe_map_active_instance()->view.layer * 30 + 12);
    ok =
        ok && !pe_map_active_instance()->scene.layers[pe_map_active_instance()->view.layer].visible;
    editor_undo_active(false);
    ok = ok && pe_map_active_instance()->scene.layers[pe_map_active_instance()->view.layer].visible;
    editor_draw();
    smoke_click(x + w - 8 - 1.5f * arrow_width, actions + 44);
    ok = ok && pe_map_active_instance()->view.layer == before - 1 &&
         !strcmp(pe_map_active_instance()->scene.layers[pe_map_active_instance()->view.layer].name,
                 "Decoration");
    editor_undo_active(false);
    pe_map_active_instance()->view.layer = before;
    editor_draw();
    smoke_click(x + w - delete_width / 2, actions + 12);
    ok = ok && pe_map_active_instance()->scene.layer_count == before;
    editor_undo_active(false);
    pe_map_active_instance()->view.layer = before;
    editor_draw();
    /* Import and source size belong to Tiles, independently of the selected entity. */
    smoke_click(app.platform.width - PANEL_GAP - 48, TOP + TABS + 82);
    ok = ok && app.shell.field == FIELD_IMPORT;
    editor_field_stop();
    editor_draw();
    smoke_click(right_x + 70, TOP + TABS + 114);
    ok = ok && app.shell.field == FIELD_TILESIZE;
    editor_field_stop();
    editor_draw();
    int old_tile = pe_map_active_instance()->view.tile;
    smoke_click(right_x + 37 + 16, TOP + TABS + 182);
    ok = ok && pe_map_active_instance()->view.tile == 2;
    ok = ok && editor_reload_layout() && app.shell.left_tabs.selected == UI_MAP_TAB &&
         app.shell.right_tabs.selected == UI_TILES_TAB;
    editor_tab_select(true);
    editor_tab_select(false);
    ok = ok && app.shell.left_tabs.selected == UI_MAP_TAB &&
         app.shell.right_tabs.selected == UI_TILES_TAB &&
         pe_map_active_instance()->scene.layer_count == before + 1 &&
         !strcmp(pe_map_active_instance()->scene.layers[pe_map_active_instance()->view.layer].name,
                 "Decoration");
    editor_undo_active(false);
    ok = ok &&
         strcmp(pe_map_active_instance()->scene.layers[pe_map_active_instance()->view.layer].name,
                "Decoration");
    pe_history_free(&pe_map_active_instance()->history);
    pe_map_active_instance()->scene = original;
    pe_map_active_instance()->view.layer = 0;
    pe_map_active_instance()->view.tile = old_tile;
    app.shell.left_tabs.selected = UI_EXPLORER_TAB;
    app.shell.right_tabs.selected = UI_INSPECTOR_TAB;
    pe_map_active_instance()->view.map_view = pe_map_active_instance()->view.tiles_view =
        (UiScroll){0};
    editor_selection_sync();
    if (!ok) {
        fprintf(stderr, "Map/Tiles panel smoke failed\n");
    }
    return ok;
}

bool editor_verify_selection(void) {
    PeScene original = pe_map_active_instance()->scene;
    PeCamera camera = pe_map_active_instance()->view.camera;
    pe_history_free(&pe_map_active_instance()->history);
    PeEntity parent = pe_entity_create(&pe_map_active_instance()->scene, PE_TRANSFORM | PE_SPRITE);
    PeEntity child = pe_entity_create(&pe_map_active_instance()->scene, PE_TRANSFORM | PE_SPRITE);
    PeObject *p = pe_entity_get(&pe_map_active_instance()->scene, parent);
    PeObject *o = pe_entity_get(&pe_map_active_instance()->scene, child);
    if (!p || !o) {
        return false;
    }
    p->x = p->y = 2000;
    o->x = 12;
    o->y = 14;
    bool ok = pe_entity_set_parent(&pe_map_active_instance()->scene, child, parent, false);
    app.shell.selected = 0;
    app.shell.left_tabs.selected = UI_SCENE_TAB;
    app.shell.right_tabs.selected = UI_INFO_TAB;
    editor_draw();
    PeScene unchanged = pe_map_active_instance()->scene;
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
    ok = ok && app.shell.selected == o->stable_id &&
         app.shell.right_tabs.selected == UI_INSPECTOR_TAB;
    ok = ok && pe_entity_world_position(&pe_map_active_instance()->scene, child, &wx, &wy);
    float sx = app.shell.viewport.x + (wx - pe_map_active_instance()->view.camera.x) *
                                          pe_map_active_instance()->view.camera.zoom;
    float sy = app.shell.viewport.y + (wy - pe_map_active_instance()->view.camera.y) *
                                          pe_map_active_instance()->view.camera.zoom;
    ok = ok && editor_inside(sx, sy, app.shell.viewport) &&
         !memcmp(&unchanged, &pe_map_active_instance()->scene, sizeof unchanged);
    PeCamera focused = pe_map_active_instance()->view.camera;
    editor_inspect_entity();
    editor_draw();
    ok = ok && !memcmp(&focused, &pe_map_active_instance()->view.camera, sizeof focused);
    smoke_pointer(SDL_EVENT_MOUSE_BUTTON_DOWN, sx + 2, sy + 2);
    smoke_pointer(SDL_EVENT_MOUSE_MOTION, sx + 2 + 20 * pe_map_active_instance()->view.camera.zoom,
                  sy + 2);
    smoke_pointer(SDL_EVENT_MOUSE_BUTTON_UP,
                  sx + 2 + 20 * pe_map_active_instance()->view.camera.zoom, sy + 2);
    ok = ok && !memcmp(&focused, &pe_map_active_instance()->view.camera, sizeof focused);
    float moved_x = 0, moved_y = 0;
    ok = ok &&
         pe_entity_world_position(&pe_map_active_instance()->scene, child, &moved_x, &moved_y) &&
         fabsf(moved_x - wx - 20) < .01f;
    editor_field_start(FIELD_X, "5000");
    editor_field_apply();
    editor_draw();
    ok = ok && pe_entity_world_position(&pe_map_active_instance()->scene, child, &wx, &wy);
    sx = app.shell.viewport.x + (wx - pe_map_active_instance()->view.camera.x) *
                                    pe_map_active_instance()->view.camera.zoom;
    sy = app.shell.viewport.y + (wy - pe_map_active_instance()->view.camera.y) *
                                    pe_map_active_instance()->view.camera.zoom;
    ok = ok && editor_inside(sx, sy, app.shell.viewport);
    pe_history_free(&pe_map_active_instance()->history);
    pe_map_active_instance()->scene = original;
    pe_map_active_instance()->view.camera = camera;
    pe_map_active_instance()->reveal_selected = false;
    app.shell.selected = 0;
    app.shell.left_tabs.selected = UI_EXPLORER_TAB;
    app.shell.right_tabs.selected = UI_INSPECTOR_TAB;
    if (!ok) {
        fprintf(stderr, "Entity selection focus smoke failed\n");
    }
    return ok;
}

void editor_capture_component_dropdown(void) {
    PeScene scene = pe_map_active_instance()->scene;
    PeCamera camera = pe_map_active_instance()->view.camera;
    uint32_t selected = app.shell.selected;
    UiId left = app.shell.left_tabs.selected, right = app.shell.right_tabs.selected;
    Tool tool = pe_map_active_instance()->view.tool;
    for (uint32_t i = 0; i < PE_ENTITIES; i++) {
        PeObject *o = &pe_map_active_instance()->scene.objects[i];
        if (!o->alive) {
            continue;
        }
        PeEntity entity = {i, o->generation};
        /* Show one selectable option without changing the real scene or its history. */
        if (o->mask & PE_PLAYER) {
            pe_entity_remove_component(&pe_map_active_instance()->scene, entity, PE_PLAYER);
        } else if (o->mask & PE_BODY) {
            pe_entity_remove_component(&pe_map_active_instance()->scene, entity, PE_BODY);
        }
        app.shell.selected = o->stable_id;
        pe_map_active_instance()->view.tool = SELECT;
        app.shell.left_tabs.selected = UI_SCENE_TAB;
        editor_inspect_entity();
        editor_draw();
        SDL_FRect anchor = pe_map_active_instance()->component_anchor;
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
        pe_entity_set_name(&pe_map_active_instance()->scene, entity, "Hero");
        editor_draw();
        smoke_click(app.platform.width - RIGHT + PANEL_GAP + 100, TOP + TABS + 44);
        SDL_Event end_name = {0};
        end_name.type = SDL_EVENT_KEY_DOWN;
        end_name.key.key = SDLK_END;
        editor_event(&end_name);
        if (snprintf(path, sizeof path, "%s.name.bmp", original_path) < (int)sizeof path) {
            editor_screenshot_path = path;
            editor_draw();
            editor_screenshot_path = original_path;
        }
        editor_field_stop();
        break;
    }
    pe_map_active_instance()->scene = scene;
    pe_map_active_instance()->view.camera = camera;
    app.shell.selected = selected;
    pe_map_active_instance()->view.tool = tool;
    app.shell.left_tabs.selected = left;
    app.shell.right_tabs.selected = right;
    pe_map_active_instance()->reveal_selected = false;
    pe_map_active_instance()->component_add_open = false;
}
