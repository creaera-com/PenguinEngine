#include "app/editor_internal.h"

void editor_play_toggle(void) {
    if (editor_edit_pending())
        return;
    app.playing = !app.playing;
    if (app.playing) {
        app.map->runtime = app.map->scene;
        app.accumulator = 0;
        editor_message("Play: WASD / arrows. Stop restores edited scene.");
    } else
        editor_message("Stopped; edited scene preserved");
}

void editor_map_update(double elapsed) {
    if (app.playing) {
        app.accumulator += elapsed;
        const bool *keys = SDL_GetKeyboardState(NULL);
        float x = (keys[SDL_SCANCODE_D] || keys[SDL_SCANCODE_RIGHT]) -
                  (keys[SDL_SCANCODE_A] || keys[SDL_SCANCODE_LEFT]),
              y = (keys[SDL_SCANCODE_S] || keys[SDL_SCANCODE_DOWN]) -
                  (keys[SDL_SCANCODE_W] || keys[SDL_SCANCODE_UP]);
        while (app.accumulator >= 1.0 / 60) {
            pe_update(&app.map->runtime, x, y, 1.f / 60);
            app.accumulator -= 1.0 / 60;
        }
    }
}
