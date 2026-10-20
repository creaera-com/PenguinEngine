/** @file @brief Clean 1:1 content view and workspace region overlay. */
#ifndef PE_VECTOR_PREVIEW_RENDER_H
#define PE_VECTOR_PREVIEW_RENDER_H
#include "editors/vector/image/render.h"
#include "editors/vector/text/render.h"
/** @brief Render only content, including masks and live transforms, without editing overlays. */
void vector_preview_draw(const VectorDocument *document, SDL_Renderer *renderer, PUIRect area,
                         VectorTextRenderer *text, VectorImageRenderer *images);
/** @brief Mark the chosen/draft region on the zoomed workspace. */
void vector_preview_overlay(const VectorDocument *document, SDL_Renderer *renderer);
#endif
