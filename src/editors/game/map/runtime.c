#include "editors/game/map/internal.h"

void editor_play_toggle(void) {
    if (!app.workspace.map) {
        return;
    }
    if (editor_edit_pending()) {
        return;
    }
    app.workspace.map->view.playing = !app.workspace.map->view.playing;
    if (app.workspace.map->view.playing) {
        app.workspace.map->runtime = app.workspace.map->scene;
        app.workspace.map->view.accumulator = 0;
        editor_message("Play: WASD / arrows. Stop restores edited scene.");
    } else {
        editor_message("Stopped; edited scene preserved");
    }
}

void editor_map_update(double elapsed) {
    if (!app.workspace.map) {
        return;
    }
    if (app.workspace.map->view.playing) {
        app.workspace.map->view.accumulator += elapsed;
        const bool *keys = SDL_GetKeyboardState(NULL);
        float x = (keys[SDL_SCANCODE_D] || keys[SDL_SCANCODE_RIGHT]) -
                  (keys[SDL_SCANCODE_A] || keys[SDL_SCANCODE_LEFT]),
              y = (keys[SDL_SCANCODE_S] || keys[SDL_SCANCODE_DOWN]) -
                  (keys[SDL_SCANCODE_W] || keys[SDL_SCANCODE_UP]);
        if (pe_console_focused(app.shell.console)) {
            x = y = 0;
        }
        while (app.workspace.map->view.accumulator >= 1.0 / 60) {
            pe_update(&app.workspace.map->runtime, x, y, 1.f / 60);
            app.workspace.map->view.accumulator -= 1.0 / 60;
        }
    }
}
