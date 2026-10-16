/** @file
 * @brief Layoutgebundener, begrenzter Bildcache; keine Dateizugriffe der UI-Bibliothek. */
#ifndef PENGUIN_LAYOUT_IMAGES_H
#define PENGUIN_LAYOUT_IMAGES_H
#include "editor/layout/ui_layout.h"
#include <SDL3/SDL.h>
/** @brief Privater Cacheeintrag; Inhalte bleiben in images.c. */
typedef struct PeUiImageEntry PeUiImageEntry;

/** @brief Besitzender Cache für maximal 64 PNGs einschließlich fehlgeschlagener Ladeversuche. */
typedef struct {
    PeUiImageEntry *entries; ///< Bei Bedarf angelegter Eintragspuffer.
    int count;               ///< Anzahl belegter Einträge.
} PeUiImages;

/** @brief Gibt alle Texturen und Einträge frei; vor dem zugehörigen Renderer aufrufen. */
void pe_ui_images_free(PeUiImages *images);
/** @brief Lädt einmalig relativ zur YAML-Datei oder absolut und liefert eine geliehene Textur.
 * @param images Besitzender Layoutcache; auch fehlende Dateien werden bis Reload gemerkt.
 * @param renderer Geliehener Texturbesitzer.
 * @param layout YAML-Modell mit Ursprungspfad.
 * @param path PNG-Pfad aus dem Button.
 * @return Geliehene Textur bis Freigabe des Caches; NULL für Fallback-Symbol.
 */
SDL_Texture *pe_ui_images_get(PeUiImages *images, SDL_Renderer *renderer, const PeUiLayout *layout,
                              const char *path);
#endif
