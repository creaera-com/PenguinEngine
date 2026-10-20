/** @file @brief Bounded app image codec services shared through the editor service contract. */
#ifndef PENGUENGINE_EDITOR_IMAGES_H
#define PENGUENGINE_EDITOR_IMAGES_H
#include <SDL3/SDL.h>
/** @brief Decode PNG/JPEG, at most 32 MiB encoded, 8192 per axis and 16 million pixels. */
SDL_Surface *editor_image_decode(const char *path);

/** @brief Encode an RGBA surface to PNG or JPEG using SDL file I/O; JPEG quality is 1..100. */
bool editor_image_write(SDL_Surface *surface, const char *path, bool jpeg, int quality);

#endif
