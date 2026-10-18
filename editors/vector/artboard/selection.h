/** @file @brief Artboard selection and label-only dragging on the document canvas. */
#ifndef PE_VECTOR_SELECTION_H
#define PE_VECTOR_SELECTION_H
#include "editors/vector/document/document.h"
/** @brief Screen-space label bounds shared by rendering and hit testing. */
PUIRect vector_artboard_label_rect(const VectorDocument *document, const VectorArtboard *board);
/** @brief Resolve transient drag offsets without changing document geometry. */
PUIRect vector_artboard_display_rect(const VectorDocument *document, const VectorArtboard *board);
/** @brief Cancel the preview and release mouse capture. */
void vector_selection_cancel(VectorDocument *document);
/** @brief Select only in selection mode; drag labels to move, or handles to resize. */
bool vector_selection_event(VectorDocument *document, PUIView *view, const SDL_Event *event);
#endif
