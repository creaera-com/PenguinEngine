/** @file
 * @brief Workspace marquee gesture and overlay. */
#ifndef PE_VECTOR_MARQUEE_H
#define PE_VECTOR_MARQUEE_H
#include "editors/vector/document/document.h"
/** @brief Start a rectangle-only selection frame; Shift keeps the initial selection. */
void vector_marquee_begin(VectorDocument *document, float x, float y, bool additive,
                          unsigned board);
/** @brief Update/finish a captured marquee; returns true for consumed events. */
bool vector_marquee_event(VectorDocument *document, const SDL_Event *event);
/** @brief Draw the selection frame in viewport coordinates. */
void vector_marquee_draw(const VectorDocument *document, SDL_Renderer *renderer);
#endif
