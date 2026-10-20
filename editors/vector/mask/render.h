/** @file @brief Reusable per-artboard texture compositor for arbitrary clipping paths. */
#ifndef PE_VECTOR_MASK_RENDER_H
#define PE_VECTOR_MASK_RENDER_H
#include "editors/vector/mask/mask.h"
#include "pengui/pengui.h"
#include <SDL3/SDL.h>

/** @brief Frame-local target reused for clipped objects; never stored in document/history. */
typedef struct {
    SDL_Renderer *renderer;
    SDL_Texture *texture, *clipped;
    int width, height;
} VectorMaskRenderer;

/** @brief Draw content through mask triangles, retaining holes and original alpha.
 * Render failures hide content rather than exposing pixels outside the mask.
 */
void vector_mask_render(VectorMaskRenderer *context, const VectorShape *mask, PUIRect rect,
                        float angle, SDL_FRect board, float zoom, void (*draw)(void *), void *data);
/** @brief Release frame-local GPU resources. */
void vector_mask_renderer_destroy(VectorMaskRenderer *context);
#endif
