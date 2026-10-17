/** @file
 * @brief Besitz- und Zustandsbereich platform; keine globale Instanz. */
#ifndef PENGUIN_PEWINDOWSTATE_H
#define PENGUIN_PEWINDOWSTATE_H
#include <SDL3/SDL.h>
#include <stdbool.h>

/** @brief Zustand für platform; Lebensdauer wird vom Anwendungshost koordiniert. */
typedef struct {
    bool window_collapsed;       ///< Nur die Titelleiste ist sichtbar.
    bool window_was_maximized;   ///< Maximierung vor dem Wegklappen.
    int window_restore_width;    ///< Fensterbreite vor dem Wegklappen.
    int window_restore_height;   ///< Fensterhöhe vor dem Wegklappen.
    SDL_Window *window;          ///< Besitzendes Hauptfenster; vor SDL_Quit zerstören.
    SDL_Renderer *renderer;      ///< Besitzender SDL-Renderer; nach seinen Texturen freigeben.
    SDL_Cursor *vertical_cursor; ///< Besitzender Cursor für vertikale Größenänderung.
    SDL_Cursor *resize_cursor;   ///< Besitzender SDL-Cursor für Spaltengrenzen.
    int width;                   ///< Fensterbreite in Pixeln.
    int height;                  ///< Fensterhöhe in Pixeln.
} PeWindowState;
#endif
