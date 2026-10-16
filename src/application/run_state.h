/** @file
 * @brief Besitz- und Zustandsbereich runtime; keine globale Instanz. */
#ifndef PENGUIN_PERUNSTATE_H
#define PENGUIN_PERUNSTATE_H
#include <SDL3/SDL.h>
#include <stdbool.h>

/** @brief Zustand für runtime; Lebensdauer wird vom Anwendungshost koordiniert. */
typedef struct {
    bool running;     ///< Ob die Anwendungshauptschleife weiterläuft.
    bool smoke;       ///< Ob der kurze headless Prüfmodus aktiv ist.
    Uint64 last_time; ///< Zeitpunkt des vorigen Frames in SDL-Nanosekunden.
} PeRunState;
#endif
