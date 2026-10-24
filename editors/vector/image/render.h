/** @file
 * @brief Editor-owned bounded image texture cache, separate from document values. */
#ifndef PE_VECTOR_IMAGE_RENDER_H
#define PE_VECTOR_IMAGE_RENDER_H
#include "editors/vector/object/shape.h"
#include <SDL3/SDL.h>
typedef struct VectorImageRenderer VectorImageRenderer;
VectorImageRenderer *vector_image_renderer_create(void);
/** @brief Release cached textures before destroying their SDL renderer. */
void vector_image_renderer_destroy(VectorImageRenderer *cache);
/** @brief Draw placed images with scaling, rotation and reflection; missing assets show a cross. */
void vector_image_draw(VectorImageRenderer *cache, SDL_Renderer *renderer, const VectorShape *shape,
                       PUIRect rect, float angle, SDL_FRect board, float zoom);
#endif
