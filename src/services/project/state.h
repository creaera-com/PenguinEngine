/** @file
 * @brief Besitz- und Zustandsbereich project; keine globale Instanz. */
#ifndef PENGUIN_PEPROJECTSTATE_H
#define PENGUIN_PEPROJECTSTATE_H
#include <stdbool.h>

/** @brief Zustand für project; Lebensdauer wird vom Anwendungshost koordiniert. */
typedef struct {
    char root[768];        ///< Besitzender Modellbaum bzw. kopierter Projektwurzelpfad.
    char scene_path[1024]; ///< Pfad der aktuell bearbeiteten Szenendatei.
} PeProjectState;
#endif
