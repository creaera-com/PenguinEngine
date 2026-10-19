/** @file @brief Workspace selection, live transforms and shared handles for vector objects. */
#ifndef PE_VECTOR_WORKSPACE_SELECTION_H
#define PE_VECTOR_WORKSPACE_SELECTION_H
#include "editors/vector/document/document.h"
/** @brief Screen-space label bounds shared by rendering and hit testing. */
PUIRect vector_artboard_label_rect(const VectorDocument *document, const VectorArtboard *board);
/** @brief Resolve transient drag offsets without changing document geometry. */
PUIRect vector_artboard_display_rect(const VectorDocument *document, const VectorArtboard *board);
/** @brief Rectangle preview in parent-local coordinates; persistent geometry stays unchanged. */
PUIRect vector_rectangle_display_rect(const VectorDocument *document,
                                      const VectorRectangle *rectangle);
/** @brief Clockwise preview angle, including a captured rotation gesture. */
float vector_rectangle_display_angle(const VectorDocument *document,
                                     const VectorRectangle *rectangle);
/** @brief Screen-space square grip bounds, oriented with a single rotated object. */
PUIRect vector_selection_handle_rect(VectorDocument *document, int index, float size);
/** @brief Visible editable selection bounds in screen pixels, including drag preview. */
bool vector_selection_bounds(VectorDocument *document, PUIRect *bounds);
/** @brief Draw shared white resize handles above document content. */
void vector_selection_draw_handles(VectorDocument *document, SDL_Renderer *renderer);
/** @brief Cancel the preview and release mouse capture. */
void vector_selection_cancel(VectorDocument *document);
/** @brief Selection-mode gestures: Shift toggles rectangles, free-area drag selects a frame.
 * Selected rectangles move and resize together through the shared eight handles.
 * Artboard movement remains label-only. */
bool vector_selection_event(VectorDocument *document, PUIView *view, const SDL_Event *event);
#endif
