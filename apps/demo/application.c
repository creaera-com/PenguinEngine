#include "application.h"
#include "pengui/pengui.h"
#include <stdio.h>
#include <string.h>

typedef struct {
    SDL_Window *window;
    SDL_Renderer *renderer;
    PUILayout *layout;
    PUIView *view;
    PUIContext pui;
    PUIWindow chrome;
    bool running, smoke, reload;
    int clicks;
    const char *screenshot;
    char path[1200];
} Playground;

/* App actions stay outside the UI library; YAML only carries their names. */
static void action(void *context, const char *name, const char *widget) {
    Playground *app = context;
    if (pui_window_action(app->window, name)) {
        return;
    }
    if (!strcmp(name, "app.quit")) {
        app->running = false;
    } else if (!strcmp(name, "app.reload")) {
        app->reload = true;
    } else {
        char message[256];
        const char *value = pui_view_value(app->view, widget);
        if (value) {
            snprintf(message, sizeof message, "Action: %s | Value: %s", name, value);
        } else {
            snprintf(message, sizeof message, "Action: %s | Clicks: %d", name, ++app->clicks);
        }
        pui_view_set_text(app->view, "status", message);
    }
}

/* Validate and construct the replacement before releasing the currently usable layout. */
static bool load_layout(Playground *app) {
    char error[256] = {0};
    PUILayout *layout = pui_layout_load(app->path, error, sizeof error);
    if (!layout) {
        fprintf(stderr, "%s: %s\n", app->path, *error ? error : "out of memory");
        return false;
    }
    PUIView *view = pui_view_new(layout, app->renderer);
    if (!view) {
        pui_layout_free(layout);
        return false;
    }
    app->chrome.view = view;
    pui_view_free(app->view);
    pui_layout_free(app->layout);
    app->view = view;
    app->layout = layout;
    pui_init(&app->pui);
    pui_view_apply_theme(layout);
    SDL_SetWindowTitle(app->window, layout->title);
    return true;
}

static bool parse_arguments(Playground *app, int argc, char **argv) {
    for (int i = 1; i < argc; i++) {
        if (!strcmp(argv[i], "--smoke")) {
            app->smoke = true;
        } else if (!strcmp(argv[i], "--layout") && i + 1 < argc) {
            if (snprintf(app->path, sizeof app->path, "%s", argv[++i]) >= (int)sizeof app->path) {
                return false;
            }
        } else if (!strcmp(argv[i], "--screenshot") && i + 1 < argc) {
            app->screenshot = argv[++i];
        } else {
            fprintf(stderr, "Usage: pengui_playground [--layout file.yaml] [--smoke] [--screenshot "
                            "file.bmp]\n");
            return false;
        }
    }
    return true;
}

static void events(Playground *app) {
    SDL_Event event;
    while (SDL_PollEvent(&event)) {
        bool consumed = pui_view_event(app->view, &app->pui, app->window, &event);
        if (event.type == SDL_EVENT_QUIT || event.type == SDL_EVENT_WINDOW_CLOSE_REQUESTED) {
            app->running = false;
        } else if (!consumed && event.type == SDL_EVENT_KEY_DOWN && !event.key.repeat) {
            if (event.key.key == SDLK_F6) {
                app->reload = true;
            } else if (event.key.key == SDLK_ESCAPE) {
                app->running = false;
            }
        }
    }
}

static bool screenshot(Playground *app) {
    if (!app->screenshot) {
        return true;
    }
    SDL_Surface *surface = SDL_RenderReadPixels(app->renderer, NULL);
    bool ok = surface && SDL_SaveBMP(surface, app->screenshot);
    SDL_DestroySurface(surface);
    return ok;
}

int playground_run(int argc, char **argv) {
    Playground app = {0};
    int result = 1;
    if (!parse_arguments(&app, argc, argv) || !SDL_Init(SDL_INIT_VIDEO)) {
        return 1;
    }
    if (!*app.path) {
        snprintf(app.path, sizeof app.path, "%splayground.yaml", SDL_GetBasePath());
    }
    app.window = SDL_CreateWindow("pengUI", 1280, 800, SDL_WINDOW_RESIZABLE | SDL_WINDOW_HIDDEN);
    app.renderer = app.window ? SDL_CreateRenderer(app.window, NULL) : NULL;
    if (!app.renderer || !load_layout(&app)) {
        goto cleanup;
    }
    SDL_SetWindowMinimumSize(app.window, 800, 600);
    SDL_SetWindowSize(app.window, app.layout->width, app.layout->height);
    SDL_SetRenderVSync(app.renderer, 1);
    pui_window_attach(&app.chrome, app.window, app.view, &app.pui);
    SDL_ShowWindow(app.window);
    app.running = true;
    result = 0;
    int frames = 0;
    while (app.running) {
        events(&app);
        int width, height;
        SDL_GetWindowSize(app.window, &width, &height);
        pui_panel(app.renderer, (SDL_FRect){0, 0, (float)width, (float)height}, PUI_CANVAS);
        pui_view_draw(app.view, &app.pui, (PUIRect){3, 3, width - 6.f, height - 6.f}, action, &app);
        if (!screenshot(&app)) {
            result = 1;
            break;
        }
        SDL_RenderPresent(app.renderer);
        /* Reload after drawing: action callbacks may still borrow the old YAML nodes. */
        if (app.reload) {
            app.reload = false;
            load_layout(&app);
        }
        if (app.smoke && ++frames >= 3) {
            app.running = false;
        }
        SDL_Delay(1);
    }
cleanup:
    pui_window_detach(&app.chrome);
    pui_view_free(app.view);
    pui_layout_free(app.layout);
    SDL_DestroyRenderer(app.renderer);
    SDL_DestroyWindow(app.window);
    SDL_Quit();
    return result;
}
