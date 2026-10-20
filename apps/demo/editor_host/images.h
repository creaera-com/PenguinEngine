/** @file @brief Bounded app image decoder shared through the editor service contract. */
#ifndef PENGUENGINE_EDITOR_IMAGES_H
#define PENGUENGINE_EDITOR_IMAGES_H
#include <SDL3/SDL.h>
/** @brief Decode PNG/JPEG, at most 32 MiB encoded, 8192 per axis and 16 million pixels. */
SDL_Surface *editor_image_decode(const char *path);
#endif
