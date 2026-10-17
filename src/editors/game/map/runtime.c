#include "editors/game/map/internal.h"

void editor_play_toggle(void) {
    if (!pe_map_active_instance()) {
        return;
    }
    if (editor_edit_pending()) {
        return;
    }
    pe_map_active_instance()->view.playing = !pe_map_active_instance()->view.playing;
    if (pe_map_active_instance()->view.playing) {
        pe_map_active_instance()->runtime = pe_map_active_instance()->scene;
        pe_map_active_instance()->view.accumulator = 0;
        editor_message("Play: WASD / arrows. Stop restores edited scene.");
    } else {
        editor_message("Stopped; edited scene preserved");
    }
}

void editor_map_update(double elapsed) {
    if (!pe_map_active_instance()) {
        return;
    }
    if (pe_map_active_instance()->view.playing) {
        pe_map_active_instance()->view.accumulator += elapsed;
        const bool *keys = SDL_GetKeyboardState(NULL);
        float x = (keys[SDL_SCANCODE_D] || keys[SDL_SCANCODE_RIGHT]) -
                  (keys[SDL_SCANCODE_A] || keys[SDL_SCANCODE_LEFT]),
              y = (keys[SDL_SCANCODE_S] || keys[SDL_SCANCODE_DOWN]) -
                  (keys[SDL_SCANCODE_W] || keys[SDL_SCANCODE_UP]);
        if (pe_console_focused(app.shell.console)) {
            x = y = 0;
        }
        while (pe_map_active_instance()->view.accumulator >= 1.0 / 60) {
            pe_update(&pe_map_active_instance()->runtime, x, y, 1.f / 60);
            pe_map_active_instance()->view.accumulator -= 1.0 / 60;
        }
    }
}
