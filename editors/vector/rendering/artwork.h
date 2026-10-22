/** @file @brief Bounded, reusable supersampling for vector artwork, independent of editor UI. */
#ifndef PE_VECTOR_ARTWORK_RENDER_H
#define PE_VECTOR_ARTWORK_RENDER_H
#include "editors/vector/rendering/objects.h"

/** @brief Renderer-local scratch resources; document/history data are only borrowed while drawing.
 */
typedef struct {
    PDrawRenderer drawing;
    VectorDocument *scratch;
} VectorArtworkRenderer;

/** @brief Draw with 2x supersampling in bounded tiles. NULL cache uses temporary resources.
 * Preserves render target, viewport, clipping, scale, draw color, blend and document state.
 * Falls back to native rendering if scratch resources cannot be allocated.
 */
void vector_artwork_draw(VectorArtworkRenderer *cache, VectorDocument *document,
                         const VectorArtboard *board, SDL_Renderer *renderer, SDL_FRect bounds,
                         VectorTextRenderer *text, VectorImageRenderer *images);
/** @brief Release scratch textures and the borrowed-value document copy; safe repeatedly. */
void vector_artwork_renderer_destroy(VectorArtworkRenderer *cache);
#endif
