#include "app/editor_internal.h"
#include <SDL3/SDL_main.h>
#ifdef PE_EDITOR_SMOKE_TESTS
#include "editor_smoke.h"
#endif
static bool start_scripts;
static bool start_scene_tree;
static bool start_map_tools;

int main(int argc, char **argv) {
    const char *root = NULL;
    for (int i = 1; i < argc; i++) {
        if (!strcmp(argv[i], "--project") && i + 1 < argc) {
            root = argv[++i];
        } else if (!strcmp(argv[i], "--screenshot") && i + 1 < argc) {
            editor_screenshot_path = argv[++i];
        } else if (!strcmp(argv[i], "--ui") && i + 1 < argc) {
            snprintf(app.ui_path, sizeof app.ui_path, "%s", argv[++i]);
        } else if (!strcmp(argv[i], "--scripts")) {
            start_scripts = true;
        } else if (!strcmp(argv[i], "--map-tools")) {
            start_map_tools = true;
        } else if (!strcmp(argv[i], "--scene-tree")) {
            start_scene_tree = true;
        } else if (!strcmp(argv[i], "--smoke")) {
            app.smoke = true;
        } else {
            fprintf(stderr,
                    "Usage: penguin_editor [--project path] [--smoke] [--scripts] "
                    "[--scene-tree] [--map-tools] [--ui shell.yaml] [--screenshot file.bmp]\n");
            return 1;
        }
    }

    char error[256];
    if (!pe_project_resolve(root, SDL_GetBasePath(), app.root, sizeof app.root, error,
                            sizeof error)) {
        fprintf(stderr, "%s\n", error);
        return 1;
    }
    root = app.root;
    if (!pe_project_load(root, app.scene_path, sizeof app.scene_path, error, sizeof error)) {
        fprintf(stderr, "%s\n", error);
        return 1;
    }
    if (!SDL_Init(SDL_INIT_VIDEO)) {
        fprintf(stderr, "%s\n", SDL_GetError());
        return 1;
    }
    app.window = SDL_CreateWindow("PenguinEngine - Editor", 1280, 800,
                                  SDL_WINDOW_RESIZABLE | SDL_WINDOW_HIDDEN);
    app.renderer = app.window ? SDL_CreateRenderer(app.window, NULL) : NULL;
    if (!app.renderer) {
        fprintf(stderr, "%s\n", SDL_GetError());
        SDL_DestroyWindow(app.window);
        SDL_Quit();
        return 1;
    }
    SDL_SetWindowMinimumSize(app.window, 1100, 740);
    SDL_SetRenderVSync(app.renderer, 1);
    ui_init(&app.ui);
    if (!*app.ui_path) {
        snprintf(app.ui_path, sizeof app.ui_path, "%sui-layouts/shell.yaml", SDL_GetBasePath());
        FILE *check = fopen(app.ui_path, "rb");
        if (check) {
            fclose(check);
        } else {
            snprintf(app.ui_path, sizeof app.ui_path, "%s../ui-layouts/shell.yaml",
                     SDL_GetBasePath());
        }
    }
    app.shell_layout = pe_ui_layout_load(app.ui_path, "shell", error, sizeof error);
    if (!app.shell_layout) {
        fprintf(stderr, "%s\n", error);
        return 1;
    }
    app.gap = 3;
    app.tab_gap = 2;
    app.min_left = 192;
    app.min_right = 264;
    app.min_center = 320;
    editor_apply_shell_layout();
    app.left_width = 240;
    app.right_width = 280;
    app.layers_open = app.palette_open = true;
    editor_apply_shell_layout();
    UiTab left_tabs[] = {
        {UI_EXPLORER_TAB, true, 0}, {UI_SCENE_TAB, true, 0}, {UI_MAP_TAB, true, 0}};
    UiTab right_tabs[] = {
        {UI_INSPECTOR_TAB, true, 0}, {UI_INFO_TAB, true, 0}, {UI_TILES_TAB, true, 0}};
    ui_tabs_init(&app.left_tabs, left_tabs, 3, UI_EXPLORER_TAB);
    ui_tabs_init(&app.right_tabs, right_tabs, 3, UI_INSPECTOR_TAB);
    app.vertical_cursor = SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_NS_RESIZE);
    app.resize_cursor = SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_EW_RESIZE);
    app.width = 1280;
    app.height = 800;
    editor_window_init();
    app.viewport = (SDL_FRect){LEFT, TOP + TABS, 1280 - LEFT - RIGHT, 800 - TOP - TABS - BOTTOM};
    char *pref = app.smoke ? SDL_strdup(SDL_GetBasePath())
                           : SDL_GetPrefPath("PenguinEngine", "editor-sessions");
    if (!pref) {
        fprintf(stderr, "Cannot create session directory: %s\n", SDL_GetError());
        return 1;
    }
    for (int i = 0; i < 2; i++) {
        snprintf(app.session_paths[i], sizeof app.session_paths[i], "%s%08x-%llx-%d.session", pref,
                 ui_id(app.root), (unsigned long long)SDL_GetPerformanceCounter(), i);
    }
    SDL_free(pref);
    app.camera = (PeCamera){-16, -16, 1.4f};
    app.grid = true;
    app.tile = 1;
    app.running = true;
    app.last_time = SDL_GetTicksNS();
    strcpy(app.expanded[0], "assets");
    strcpy(app.expanded[1], "scenes");
    strcpy(app.expanded[2], "scripts");
    app.expanded_count = 3;
    editor_tree_refresh();
    editor_message("Welcome! Select a tile, paint a map, and press F5 to play.");
    if (!editor_activate(app.workspaces.selected, error, sizeof error)) {
        fprintf(stderr, "%s\n", error);
        return 1;
    }

    SDL_ShowWindow(app.window);
    int exit_code = 0;
#ifdef PE_EDITOR_SMOKE_TESTS
    if (app.smoke &&
        (!editor_verify_editor() || !editor_verify_script_editor() || !editor_verify_ui_layout() ||
         !editor_verify_workspace_tools() || !editor_verify_lifecycle() ||
         !editor_verify_console() || !editor_verify_window() || !editor_verify_scene_graph() ||
         !editor_verify_components() || !editor_verify_map_panels() ||
         !editor_verify_selection())) {
        app.running = false;
        exit_code = 1;
    }
#endif
#ifdef PE_EDITOR_SMOKE_TESTS
    if (app.smoke && exit_code == 0 && editor_screenshot_path) {
        editor_capture_component_dropdown();
    }
#endif
    if (start_scripts) {
        editor_tab_select(true);
    } else if (start_map_tools) {
        app.left_tabs.selected = UI_MAP_TAB;
        app.right_tabs.selected = UI_TILES_TAB;
    } else if (start_scene_tree) {
        app.left_tabs.selected = UI_SCENE_TAB;
        for (int i = 0; i < PE_ENTITIES; i++) {
            if (app.map->scene.objects[i].alive) {
                app.selected = app.map->scene.objects[i].stable_id;
                editor_inspect_entity();
                app.tool = SELECT;
                break;
            }
        }
    }

    int frames = 0;
    while (app.running) {
        SDL_Event e;
        while (SDL_PollEvent(&e)) {
            editor_event(&e);
        }
        Uint64 now = SDL_GetTicksNS();
        double elapsed = (now - app.last_time) / 1e9;
        app.last_time = now;
        if (elapsed > .1) {
            elapsed = .1;
        }
        pe_console_tick(app.console);
        editor_map_update(elapsed);
        editor_draw();
        if (app.smoke && ++frames >= 3) {
            app.running = false;
        }
        SDL_Delay(1);
    }
    pe_ui_layout_free(app.active_layout);
    app.active_layout = NULL;
    pe_ui_layout_free(app.shell_layout);
    if (app.script) {
        se_free(app.script);
        free(app.script);
    }
    if (app.map) {
        pe_history_free(&app.map->history);
        pe_assets_free(&app.map->assets);
        free(app.map);
    }
    for (int i = 0; i < 2; i++) {
        remove(app.session_paths[i]);
    }
    pe_console_free(app.console);
    SDL_DestroyCursor(app.vertical_cursor);
    SDL_DestroyCursor(app.resize_cursor);
    SDL_DestroyRenderer(app.renderer);
    SDL_DestroyWindow(app.window);
    SDL_Quit();
    return exit_code;
}
