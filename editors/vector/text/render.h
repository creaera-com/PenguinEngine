/** @file
 * @brief Owned text rendering cache and shared layout metrics for text interactions. */
#ifndef PE_VECTOR_TEXT_RENDER_H
#define PE_VECTOR_TEXT_RENDER_H
#include "editor_host/editor.h"
#include "editors/vector/document/document.h"
#include "layout.h"
typedef struct VectorTextRenderer VectorTextRenderer;
/** @brief Own a bounded frame cache; the borrowed font service must outlive it. */
VectorTextRenderer *vector_text_renderer_create(PUIFonts *fonts);
/** @brief Destroy cached GPU textures before destroying their SDL renderer. */
void vector_text_renderer_destroy(VectorTextRenderer *renderer);
/** @brief Reflow using the same font metrics as painting; no GPU allocation. */
void vector_text_measure(VectorTextRenderer *renderer, const VectorShape *shape,
                         VectorTextLayout *out);
/** @brief Draw glyphs, frame outline, caret and selection inside the frame and current clip. */
void vector_text_draw(VectorTextRenderer *cache, SDL_Renderer *renderer,
                      const VectorDocument *document, const VectorShape *shape, PUIRect rect,
                      float angle, SDL_FRect board, float zoom, uint32_t fill, bool selected);
#endif
