/** @file
 * @brief Begrenztes Laden von PNG-Texturen für Spiele und Editor. */
#ifndef PENGUIN_IMAGE_H
#define PENGUIN_IMAGE_H
#include <SDL3/SDL.h>
#include <stddef.h>
/** @brief Lädt ein PNG mit maximal 1024 Pixeln je Achse; besitzt die Ergebnistextur.
 * @param renderer Geliehener Renderer; muss die Textur überleben.
 * @param path UTF-8-Dateipfad.
 * @param error Optionaler Fehlerpuffer.
 * @param size Kapazität des Fehlerpuffers.
 * @return Mit SDL_DestroyTexture freizugebende Textur; NULL bei Fehlern.
 */
SDL_Texture *pe_image_load(SDL_Renderer *renderer, const char *path, char *error, size_t size);
#endif
