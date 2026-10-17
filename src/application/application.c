#include "application/application.h"
#include "application/internal.h"
#ifdef PE_EDITOR_SMOKE_TESTS
#include "editor_smoke.h"
#endif
static bool start_scripts;
static bool start_vector, vector_preview;
static bool start_scene_tree;
static bool start_map_tools;

static bool parse_arguments(int argc, char **argv, const char **root) {
    for (int i = 1; i < argc; i++) {
        if (!strcmp(argv[i], "--project") && i + 1 < argc) {
            *root = argv[++i];
        } else if (!strcmp(argv[i], "--screenshot") && i + 1 < argc) {
            editor_screenshot_path = argv[++i];
        } else if (!strcmp(argv[i], "--ui") && i + 1 < argc) {
            snprintf(app.shell.ui_path, sizeof app.shell.ui_path, "%s", argv[++i]);
        } else if (!strcmp(argv[i], "--vector") || !strcmp(argv[i], "--vector-demo")) {
            start_vector = true;
            vector_preview = !strcmp(argv[i], "--vector-demo");
        } else if (!strcmp(argv[i], "--scripts")) {
            start_scripts = true;
        } else if (!strcmp(argv[i], "--map-tools")) {
            start_map_tools = true;
        } else if (!strcmp(argv[i], "--scene-tree")) {
            start_scene_tree = true;
        } else if (!strcmp(argv[i], "--smoke")) {
            app.runtime.smoke = true;
        } else {
            fprintf(stderr, "Usage: penguin_editor [--project path] [--smoke] [--scripts] "
                            "[--scene-tree] [--map-tools] [--vector | --vector-demo] [--ui "
                            "shell.yaml] [--screenshot file.bmp]\n");
            return false;
        }
    }
    return true;
}

static bool load_project(const char *root) {
    char error[256];
    if (!pe_project_resolve(root, SDL_GetBasePath(), app.project.root, sizeof app.project.root,
                            error, sizeof error)) {
        fprintf(stderr, "%s\n", error);
        return false;
    }
    if (!pe_project_load(app.project.root, app.project.scene_path, sizeof app.project.scene_path,
                         error, sizeof error)) {
        fprintf(stderr, "%s\n", error);
        return false;
    }
    return true;
}

static bool initialize_platform(void) {
    if (!SDL_Init(SDL_INIT_VIDEO)) {
        fprintf(stderr, "%s\n", SDL_GetError());
        return false;
    }
    app.platform.window = SDL_CreateWindow("PenguinEngine - Editor", 1280, 800,
                                           SDL_WINDOW_RESIZABLE | SDL_WINDOW_HIDDEN);
    app.platform.renderer =
        app.platform.window ? SDL_CreateRenderer(app.platform.window, NULL) : NULL;
    if (!app.platform.renderer) {
        fprintf(stderr, "%s\n", SDL_GetError());
        SDL_DestroyWindow(app.platform.window);
        SDL_Quit();
        return false;
    }
    SDL_SetWindowMinimumSize(app.platform.window, 1100, 740);
    SDL_SetRenderVSync(app.platform.renderer, 1);
    return true;
}

static bool initialize_shell(void) {
    char error[256];
    ui_init(&app.shell.ui);
    if (!*app.shell.ui_path) {
        snprintf(app.shell.ui_path, sizeof app.shell.ui_path, "%sui-layouts/shell.yaml",
                 SDL_GetBasePath());
        FILE *check = fopen(app.shell.ui_path, "rb");
        if (check) {
            fclose(check);
        } else {
            snprintf(app.shell.ui_path, sizeof app.shell.ui_path, "%s../ui-layouts/shell.yaml",
                     SDL_GetBasePath());
        }
    }
    app.shell.shell_layout = pe_ui_layout_load(app.shell.ui_path, "shell", error, sizeof error);
    if (!app.shell.shell_layout) {
        fprintf(stderr, "%s\n", error);
        return false;
    }
    app.shell.gap = 3;
    app.shell.tab_gap = 2;
    app.shell.min_left = 192;
    app.shell.min_right = 264;
    app.shell.min_center = 320;
    editor_apply_shell_layout();
    app.shell.left_width = 240;
    app.shell.right_width = 280;
    editor_apply_shell_layout();
    UiTab left_tabs[] = {
        {UI_EXPLORER_TAB, true, 0}, {UI_SCENE_TAB, true, 0}, {UI_MAP_TAB, true, 0}};
    UiTab right_tabs[] = {
        {UI_INSPECTOR_TAB, true, 0}, {UI_INFO_TAB, true, 0}, {UI_TILES_TAB, true, 0}};
    ui_tabs_init(&app.shell.left_tabs, left_tabs, 3, UI_EXPLORER_TAB);
    ui_tabs_init(&app.shell.right_tabs, right_tabs, 3, UI_INSPECTOR_TAB);
    app.platform.vertical_cursor = SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_NS_RESIZE);
    app.platform.resize_cursor = SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_EW_RESIZE);
    app.platform.width = 1280;
    app.platform.height = 800;
    editor_window_init();
    app.shell.viewport =
        (SDL_FRect){LEFT, TOP + TABS, 1280 - LEFT - RIGHT, 800 - TOP - TABS - BOTTOM};
    return true;
}

static bool initialize_sessions(void) {
    char *pref = app.runtime.smoke ? SDL_strdup(SDL_GetBasePath())
                                   : SDL_GetPrefPath("PenguinEngine", "editor-sessions");
    if (!pref) {
        fprintf(stderr, "Cannot create session directory: %s\n", SDL_GetError());
        return false;
    }
    for (int i = 0; i < editor_view_count; i++) {
        snprintf(app.workspace.session_paths[i], sizeof app.workspace.session_paths[i],
                 "%s%08x-%llx-%d.session", pref, ui_id(app.project.root),
                 (unsigned long long)SDL_GetPerformanceCounter(), i);
    }
    SDL_free(pref);
    return true;
}

static bool start_editor(void) {
    char error[256];
    app.runtime.running = true;
    app.runtime.last_time = SDL_GetTicksNS();
    strcpy(app.shell.expanded[0], "assets");
    strcpy(app.shell.expanded[1], "scenes");
    strcpy(app.shell.expanded[2], "scripts");
    app.shell.expanded_count = 3;
    editor_tree_refresh();
    editor_message("Welcome! Select a tile, paint a map, and press F5 to play.");
    if (!editor_activate(app.shell.workspaces.selected, error, sizeof error)) {
        fprintf(stderr, "%s\n", error);
        return false;
    }

    SDL_ShowWindow(app.platform.window);
    return true;
}

static int run_smoke_checks(void) {
    int exit_code = 0;
#ifdef PE_EDITOR_SMOKE_TESTS
    if (app.runtime.smoke &&
        (!editor_verify_editor() || !editor_verify_script_editor() || !editor_verify_ui_layout() ||
         !editor_verify_workspace_tools() || !editor_verify_lifecycle() ||
         !editor_verify_console() || !editor_verify_window() || !editor_verify_scene_graph() ||
         !editor_verify_components() || !editor_verify_map_panels() || !editor_verify_selection() ||
         !vector_smoke())) {
        app.runtime.running = false;
        exit_code = 1;
    }
#endif
#ifdef PE_EDITOR_SMOKE_TESTS
    if (app.runtime.smoke && exit_code == 0 && editor_screenshot_path) {
        editor_capture_component_dropdown();
    }
#endif
    return exit_code;
}

static void select_start_view(void) {
    if (start_vector) {
        editor_select_workspace(UI_VECTOR_VIEW);
        if (vector_preview && app.workspace.backend->startup) {
            app.workspace.backend->startup("demo");
        }
    } else if (start_scripts) {
        editor_tab_select(true);
    } else if (app.workspace.backend->startup) {
        if (start_map_tools) {
            app.workspace.backend->startup("tools");
        } else if (start_scene_tree) {
            app.workspace.backend->startup("scene-tree");
        }
    }
}

static void run_main_loop(void) {
    int frames = 0;
    while (app.runtime.running) {
        SDL_Event e;
        while (SDL_PollEvent(&e)) {
            editor_event(&e);
        }
        Uint64 now = SDL_GetTicksNS();
        double elapsed = (now - app.runtime.last_time) / 1e9;
        app.runtime.last_time = now;
        if (elapsed > .1) {
            elapsed = .1;
        }
        pe_console_tick(app.shell.console);
        if (app.workspace.backend && app.workspace.backend->update) {
            app.workspace.backend->update(elapsed);
        }
        editor_draw();
        if (app.runtime.smoke && ++frames >= 3) {
            app.runtime.running = false;
        }
        SDL_Delay(1);
    }
}

static void shutdown_application(void) {
    editor_release_active();
    pe_ui_images_free(&app.shell.shell_images);
    pe_ui_layout_free(app.shell.shell_layout);
    for (int i = 0; i < editor_view_count; i++) {
        remove(app.workspace.session_paths[i]);
    }
    pe_console_free(app.shell.console);
    SDL_DestroyCursor(app.platform.vertical_cursor);
    SDL_DestroyCursor(app.platform.resize_cursor);
    SDL_DestroyRenderer(app.platform.renderer);
    SDL_DestroyWindow(app.platform.window);
    SDL_Quit();
}

int pe_application_run(int argc, char **argv) {
    const char *root = NULL;
    if (!parse_arguments(argc, argv, &root) || !load_project(root) || !initialize_platform() ||
        !initialize_shell() || !initialize_sessions() || !start_editor()) {
        return 1;
    }

    int exit_code = run_smoke_checks();
    select_start_view();
    run_main_loop();
    shutdown_application();
    return exit_code;
}
