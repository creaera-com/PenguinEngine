#include "penguin/render.h"
#include <SDL3/SDL_main.h>
#include <stdio.h>
#include <string.h>
static PeScene scene;

int main(int argc, char **argv) {
    const char *root = NULL;
    bool smoke = false;
    for (int i = 1; i < argc; i++) {
        if (!strcmp(argv[i], "--project") && i + 1 < argc) {
            root = argv[++i];
        } else if (!strcmp(argv[i], "--smoke")) {
            smoke = true;
        } else {
            fprintf(stderr, "Usage: penguin_game [--project path] [--smoke]\n");
            return 1;
        }
    }

    char path[1024], error[256], resolved_root[768];
    if (!pe_project_resolve(root, SDL_GetBasePath(), resolved_root, sizeof resolved_root, error,
                            sizeof error)) {
        fprintf(stderr, "%s\n", error);
        return 1;
    }
    root = resolved_root;
    if (!pe_project_load(root, path, sizeof path, error, sizeof error) ||
        !pe_scene_load(&scene, path, error, sizeof error)) {
        fprintf(stderr, "%s\n", error);
        return 1;
    }
    if (!SDL_Init(SDL_INIT_VIDEO)) {
        fprintf(stderr, "%s\n", SDL_GetError());
        return 1;
    }

    SDL_Window *w =
        SDL_CreateWindow("PenguinEngine - Top-down example", 960, 640, SDL_WINDOW_RESIZABLE);
    SDL_Renderer *r = w ? SDL_CreateRenderer(w, NULL) : NULL;
    if (!r) {
        fprintf(stderr, "%s\n", SDL_GetError());
        SDL_DestroyWindow(w);
        SDL_Quit();
        return 1;
    }
    SDL_SetRenderVSync(r, 1);
    PeAssets assets = {0};
    if (!pe_assets_load(&assets, r, root, &scene, error, sizeof error)) {
        fprintf(stderr, "%s\n", error);
        SDL_DestroyRenderer(r);
        SDL_DestroyWindow(w);
        SDL_Quit();
        return 1;
    }

    bool running = true;
    Uint64 last = SDL_GetTicksNS();
    double accumulator = 0;
    int frames = 0;
    while (running) {
        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_EVENT_QUIT ||
                (event.type == SDL_EVENT_KEY_DOWN && event.key.key == SDLK_ESCAPE)) {
                running = false;
            }
        }
        Uint64 now = SDL_GetTicksNS();
        double elapsed = (now - last) / 1e9;
        last = now;
        if (elapsed > .1) {
            elapsed = .1;
        }
        accumulator += elapsed;
        const bool *keys = SDL_GetKeyboardState(NULL);
        float x = (keys[SDL_SCANCODE_D] || keys[SDL_SCANCODE_RIGHT]) -
                  (keys[SDL_SCANCODE_A] || keys[SDL_SCANCODE_LEFT]),
              y = (keys[SDL_SCANCODE_S] || keys[SDL_SCANCODE_DOWN]) -
                  (keys[SDL_SCANCODE_W] || keys[SDL_SCANCODE_UP]);
        while (accumulator >= 1.0 / 60) {
            pe_update(&scene, x, y, 1.f / 60);
            accumulator -= 1.0 / 60;
        }
        int rw, rh;
        SDL_GetRenderOutputSize(r, &rw, &rh);
        float zoom = (float)rw / (scene.width * scene.tile_size);
        float zy = (float)rh / (scene.height * scene.tile_size);
        if (zy < zoom) {
            zoom = zy;
        }
        PeCamera camera = {0, 0, zoom};
        SDL_SetRenderDrawColor(r, 14, 20, 28, 255);
        SDL_RenderClear(r);
        pe_render_scene(r, &scene, &assets, &camera, (SDL_FRect){0, 0, (float)rw, (float)rh}, false,
                        false);
        SDL_SetRenderDrawColor(r, 240, 240, 240, 255);
        SDL_RenderDebugText(r, 12, 12, "WASD / arrows: move   ESC: quit");
        SDL_RenderPresent(r);
        if (smoke && ++frames >= 3) {
            running = false;
        }
        SDL_Delay(1);
    }
    pe_assets_free(&assets);
    SDL_DestroyRenderer(r);
    SDL_DestroyWindow(w);
    SDL_Quit();
    return 0;
}
