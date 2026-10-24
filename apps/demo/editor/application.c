#include "pengdraw/pengdraw.h"
/** @file @brief App bootstrap and loop; depends only on shell and editor-host interfaces. */
#include "application.h"
#include "config.h"
#include "documents/controller.h"
#include "editor_host/host.h"
#include "monitor/monitor.h"
#include "platform/sdl_memory.h"
#include "project/controller.h"
#include "shell/shell.h"
#include <stdio.h>
#include <string.h>

typedef struct {
    SDL_Window *window;
    SDL_Renderer *renderer;
    EditorShell shell;
    EditorHost host;
    ProjectController projects;
    DocumentController documents;
    PUIWindow chrome;
    bool running, smoke, disable_sdl_memory;
    const char *resources, *config_path, *initial_editor, *screenshot;
} EditorApp;

static bool arguments(EditorApp *app, int argc, char **argv) {
    for (int i = 1; i < argc; i++) {
        if (!strcmp(argv[i], "--no-sdl-memory-tracking")) {
            app->disable_sdl_memory = true;
        } else if (!strcmp(argv[i], "--smoke")) {
            app->smoke = true;
        } else if (!strcmp(argv[i], "--resources") && i + 1 < argc) {
            app->resources = argv[++i];
        } else if (!strcmp(argv[i], "--config") && i + 1 < argc) {
            app->config_path = argv[++i];
        } else if (!strcmp(argv[i], "--editor") && i + 1 < argc) {
            app->initial_editor = argv[++i];
        } else if (!strcmp(argv[i], "--screenshot") && i + 1 < argc) {
            app->screenshot = argv[++i];
        } else {
            fprintf(stderr, "Usage: pengui_editor [--resources directory] [--config file.yaml] "
                            "[--editor vector|scripts|game] [--smoke] [--screenshot image.bmp] "
                            "[--no-sdl-memory-tracking]\n");
            return false;
        }
    }
    return true;
}

static void action(void *context, const char *name, const char *widget) {
    EditorApp *app = context;
    editor_host_project(&app->host, app->projects.project.root);
    editor_host_log_action(&app->host, app->shell.view, name, widget);
    if (!strcmp(name, "monitor.gpu_wait")) {
        editor_host_action(&app->host, app->shell.view, name, widget);
        return;
    }
    if (!strcmp(name, "monitor.log")) {
        editor_host_action(&app->host, app->shell.view, name, widget);
        project_explorer_action(&app->projects.explorer, app->shell.view, "project.refresh", "");
        return;
    }
    if (document_controller_action(&app->documents, &app->host, &app->projects, app->window,
                                   app->shell.view, name, widget)) {
        return;
    }
    if (project_controller_action(&app->projects, app->window, app->shell.view, name, widget)) {
        return;
    }
    if (pui_window_action(app->window, name)) {
        return;
    }
    if (!strncmp(name, "editor.", 7) && editor_host_request(&app->host, name + 7, false)) {
        return;
    } else if (!strcmp(name, "shell.reload") && app->host.active) {
        editor_host_request(&app->host, app->host.active->id, true);
    } else if (strncmp(name, "shell.", 6)) {
        if (!editor_host_action(&app->host, app->shell.view, name, widget)) {
            pui_view_set_text(app->shell.view, "status", "No handler for this editor action yet");
        }
    }
}

static void events(EditorApp *app) {
    SDL_Event event;
    while (SDL_PollEvent(&event)) {
        if (document_controller_event(&app->documents, app->shell.view, &event)) {
            continue;
        }
        if (project_controller_event(&app->projects, app->shell.view, &event)) {
            editor_host_project(&app->host, app->projects.project.root);
            editor_host_log_action(&app->host, app->shell.view, "project.dialog.complete", "");
            continue;
        }
        bool consumed = pui_view_event(app->shell.view, &app->shell.input, app->window, &event);
        if (!consumed && event.type == SDL_EVENT_KEY_DOWN && !event.key.repeat &&
            event.key.key == SDLK_S && (event.key.mod & (SDL_KMOD_CTRL | SDL_KMOD_GUI))) {
            editor_host_log_action(
                &app->host, app->shell.view,
                (event.key.mod & SDL_KMOD_SHIFT) ? "editor.save_as" : "editor.save", "");
            consumed = document_controller_action(
                &app->documents, &app->host, &app->projects, app->window, app->shell.view,
                (event.key.mod & SDL_KMOD_SHIFT) ? "editor.save_as" : "editor.save", "");
        }
        if (!consumed) {
            consumed = editor_host_event(&app->host, app->shell.view, &event);
        }
        if (event.type == SDL_EVENT_QUIT || event.type == SDL_EVENT_WINDOW_CLOSE_REQUESTED) {
            app->running = false;
        } else if (!consumed && event.type == SDL_EVENT_KEY_DOWN && !event.key.repeat &&
                   event.key.key == SDLK_F6 && app->host.active) {
            editor_host_log_action(&app->host, app->shell.view, "shell.reload", "");
            editor_host_request(&app->host, app->host.active->id, true);
        }
    }
}

static bool start(EditorApp *app, char *error, size_t capacity) {
    char config_path[1600], shell_path[1600];
    if (!app->resources) {
        app->resources = SDL_GetBasePath();
    }
    if (snprintf(config_path, sizeof config_path, "%s/application.yaml", app->resources) >=
            (int)sizeof config_path ||
        snprintf(shell_path, sizeof shell_path, "%s/shell.yaml", app->resources) >=
            (int)sizeof shell_path) {
        snprintf(error, capacity, "resource directory path is too long");
        return false;
    }
    EditorConfig config;
    if (!editor_config_load(&config, app->config_path ? app->config_path : config_path, error,
                            capacity)) {
        return false;
    }
    app->window =
        SDL_CreateWindow("pengUI Editor", 1280, 800, SDL_WINDOW_RESIZABLE | SDL_WINDOW_HIDDEN);
    app->renderer = app->window ? pdraw_create_renderer(app->window) : NULL;
    if (!app->renderer) {
        snprintf(error, capacity, "%s", SDL_GetError());
        return false;
    }
    if (!editor_shell_load(&app->shell, app->renderer, shell_path, error, capacity)) {
        return false;
    }
    editor_host_init(&app->host, (EditorServices){app->resources});
    const char *initial = app->initial_editor ? app->initial_editor : config.startup_editor;
    if (!editor_host_request(&app->host, initial, false)) {
        snprintf(error, capacity, "unknown startup editor: %s", initial);
        return false;
    }
    if (!editor_host_commit(&app->host, app->shell.view, &app->shell.input, error, capacity)) {
        return false;
    }
    document_controller_init(&app->documents);
    if (!project_controller_init(&app->projects, app->resources, !app->smoke, error, capacity)) {
        return false;
    }
    if (*error) {
        pui_view_set_text(app->shell.view, "status", error);
        error[0] = 0;
    }
    if (!project_explorer_commit(&app->projects.explorer, app->shell.view, &app->shell.input, error,
                                 capacity)) {
        return false;
    }
    SDL_SetWindowTitle(app->window, app->shell.layout->title);
    SDL_SetWindowSize(app->window, app->shell.layout->width, app->shell.layout->height);
    SDL_SetWindowMinimumSize(app->window, 800, 600);
    SDL_SetRenderVSync(app->renderer, 1);
    pui_window_attach(&app->chrome, app->window, app->shell.view, &app->shell.input);
    SDL_MaximizeWindow(app->window);
    SDL_ShowWindow(app->window);
    return true;
}

static bool save_screenshot(EditorApp *app) {
    if (!app->screenshot) {
        return true;
    }
    SDL_Surface *surface = SDL_RenderReadPixels(app->renderer, NULL);
    bool ok = surface && SDL_SaveBMP(surface, app->screenshot);
    SDL_DestroySurface(surface);
    return ok;
}

int editor_application_run(int argc, char **argv) {
    EditorApp app = {0};
    if (!arguments(&app, argc, argv)) {
        return 1;
    }
    if (!app.disable_sdl_memory && !app_sdl_memory_install()) {
        fprintf(stderr, "SDL memory diagnostics unavailable\n");
    }
    if (!SDL_Init(SDL_INIT_VIDEO)) {
        return 1;
    }
    char error[256] = {0};
    int result = 1;
    if (!start(&app, error, sizeof error)) {
        fprintf(stderr, "Editor startup failed: %s\n", error);
        goto cleanup;
    }
    app.running = true;
    result = 0;
    int frames = 0;
    while (app.running) {
        Uint64 frame_start = SDL_GetTicksNS();
        editor_host_project(&app.host, app.projects.project.root);
        events(&app);
        int width, height;
        SDL_GetWindowSize(app.window, &width, &height);
        pui_panel(app.renderer, (SDL_FRect){0, 0, width, height}, PUI_CANVAS);
        pui_view_draw(app.shell.view, &app.shell.input, (PUIRect){3, 3, width - 6.f, height - 6.f},
                      action, &app);
        if (!save_screenshot(&app)) {
            result = 1;
            break;
        }
        SDL_RenderPresent(app.renderer);
        if (!process_monitor_gpu_wait(&app.host.monitor, app.renderer)) {
            pui_view_set_text(app.shell.view, "status",
                              "GPU-Warten fehlgeschlagen; Diagnose deaktiviert");
            editor_host_log_action(&app.host, app.shell.view, "monitor.gpu_wait.failed", "");
        }
        if (!editor_host_commit(&app.host, app.shell.view, &app.shell.input, error, sizeof error)) {
            fprintf(stderr, "Editor update failed: %s\n", error);
            pui_view_set_text(app.shell.view, "status", error);
            if (app.host.active) {
                pui_view_set_checked(app.shell.view, app.host.active->id, true);
            }
        }
        if (app.smoke && ++frames >= 3) {
            app.running = false;
        }
        document_controller_commit(&app.documents, &app.host, &app.projects, app.shell.view,
                                   &app.shell.input);
        if (!project_explorer_commit(&app.projects.explorer, app.shell.view, &app.shell.input,
                                     error, sizeof error)) {
            pui_view_set_text(app.shell.view, "status", error);
        }
        /* Keep the editor at at most 60 frames/s. Faster cached rendering must
         * reduce CPU use rather than simply produce more frames in the idle UI. */
        Uint64 elapsed = SDL_GetTicksNS() - frame_start;
        Uint64 frame_budget = SDL_NS_PER_SECOND / 60;
        if (elapsed < frame_budget) {
            SDL_DelayPrecise(frame_budget - elapsed);
        }
    }
cleanup:
    document_controller_destroy(&app.documents);
    project_controller_destroy(&app.projects);
    pui_window_detach(&app.chrome);
    editor_shell_destroy(&app.shell);
    editor_host_destroy(&app.host);
    pdraw_destroy_renderer(app.renderer);
    SDL_DestroyWindow(app.window);
    SDL_Quit();
    return result;
}
