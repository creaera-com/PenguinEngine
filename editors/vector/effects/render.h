/** @file
 * @brief Bounded alpha-silhouette compositing for individual object shadows. */
#ifndef PE_VECTOR_EFFECTS_RENDER_H
#define PE_VECTOR_EFFECTS_RENDER_H
#include "pengui/pengui.h"
#include "shadow.h"
#include <SDL3/SDL.h>
/** @brief Paint only object content, translated by dx/dy in renderer pixels; no selection UI. */
typedef void (*VectorShadowPaint)(void *context, float dx, float dy);
/** @brief Draw shadow behind content. Restores SDL state; failure leaves normal content usable.
 * Bounds include stroke. Allocations are limited to the visible shadow and its sampling margin.
 */
bool vector_shadow_draw(SDL_Renderer *renderer, const VectorShadow *shadow, float zoom,
                        PUIRect bounds, VectorShadowPaint paint, void *context);
#endif
