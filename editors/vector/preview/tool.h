/** @file
 * @brief Canvas-region selection and preview control bindings. */
#ifndef PE_VECTOR_PREVIEW_TOOL_H
#define PE_VECTOR_PREVIEW_TOOL_H
#include "editors/vector/document/document.h"
/** @brief Cancel a gesture without losing its previously committed region. */
void vector_preview_cancel(VectorDocument *document);
/** @brief Handle vector.preview.select and vector.preview.clear actions. */
bool vector_preview_action(VectorDocument *document, PUIView *view, const char *action);
/** @brief Capture a region inside one visible artboard; Escape/right-click cancels. */
bool vector_preview_event(VectorDocument *document, PUIView *view, const SDL_Event *event);
/** @brief Resolve and clamp the stored region to the current artboard size. */
const VectorArtboard *vector_preview_region(const VectorDocument *document, PUIRect *region);
/** @brief Synchronize YAML toggle state after tool changes. */
void vector_preview_sync(PUIView *view, const VectorDocument *document);
#endif
