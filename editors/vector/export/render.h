/** @file
 * @brief Clean, zoom-independent raster output using the existing artwork renderers. */
#ifndef PE_VECTOR_EXPORT_RENDER_H
#define PE_VECTOR_EXPORT_RENDER_H
#include "editors/vector/image/render.h"
#include "editors/vector/text/render.h"
#include "plan.h"
/** @brief Render one plan item at 1:1; caller owns returned straight-alpha RGBA surface.
 * No UI, view previews or document/history mutations are included. SDL state is restored.
 */
SDL_Surface *vector_export_render(const VectorDocument *document, const VectorExportPlan *plan,
                                  unsigned item, SDL_Renderer *renderer, VectorTextRenderer *text,
                                  VectorImageRenderer *images, bool transparent);
/** @brief Export all planned files through staged siblings. Existing destinations are replaced.
 * All-artboard names contain stable IDs; error includes partial-batch progress on failure.
 */
bool vector_export_write(const VectorDocument *document, const VectorExportPlan *plan,
                         SDL_Renderer *renderer, VectorTextRenderer *text,
                         VectorImageRenderer *images, const char *directory, const char *stem,
                         bool jpeg, bool transparent, int quality, char *error, size_t capacity);
#endif
