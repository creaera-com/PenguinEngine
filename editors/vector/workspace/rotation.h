/** @file
 * @brief Center rotation ring and atomic single/group rotation gestures. */
#ifndef PE_VECTOR_WORKSPACE_ROTATION_H
#define PE_VECTOR_WORKSPACE_ROTATION_H
#include "editors/vector/document/document.h"
/** @brief Constant screen-space ring hit test; its center stays available for movement. */
bool vector_rotation_hit(PUIRect bounds, float x, float y);
/** @brief Capture members and pivot from the current selection. */
bool vector_rotation_begin(VectorDocument *document, PUIRect screen_bounds, float x, float y);
/** @brief Process a captured rotation; Shift snaps angles to 15-degree steps. */
bool vector_rotation_event(VectorDocument *document, PUIView *view, const SDL_Event *event);
/** @brief Read transient rotation geometry/angle without changing the document. */
PUIRect vector_rotation_display_rect(const VectorDocument *document,
                                     const VectorRectangle *rectangle);
float vector_rotation_display_angle(const VectorDocument *document,
                                    const VectorRectangle *rectangle);
/** @brief Draw the center ring and the live angle while rotating. */
void vector_rotation_draw(const VectorDocument *document, SDL_Renderer *renderer, PUIRect bounds);
#endif
