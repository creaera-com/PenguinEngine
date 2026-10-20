/** @file @brief Object selection and transient gradient endpoints on the canvas. */
#ifndef PE_VECTOR_GRADIENT_TOOL_H
#define PE_VECTOR_GRADIENT_TOOL_H
#include "editors/vector/document/document.h"
#include <SDL3/SDL.h>
/** @brief Closed geometry eligible for a fill gradient; excludes image/text frames. */
bool vector_gradient_eligible(const VectorShape *shape);
/** @brief Cancel a gesture without committing, releasing mouse capture. */
void vector_gradient_tool_cancel(VectorDocument *document);
/** @brief Toggle gradient mode, cancelling other tools; selection may follow by canvas click. */
bool vector_gradient_tool_action(VectorDocument *document, PUIView *view, const char *action);
/** @brief Drag endpoints/stops or draw a fresh ramp; one history edit on release. Esc cancels. */
bool vector_gradient_tool_event(VectorDocument *document, PUIView *view, const SDL_Event *event);
/** @brief Overlay endpoints and stop marks only while the gradient tool is active. */
void vector_gradient_tool_draw(VectorDocument *document, SDL_Renderer *renderer);
/** @brief Return transient fill settings for this shape, without modifying document data. */
const VectorGradient *vector_gradient_display(const VectorDocument *document,
                                              const VectorShape *shape);
#endif
