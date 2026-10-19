/** @file @brief Rectangle selection by stable IDs, without native input or UI dependencies. */
#ifndef PE_VECTOR_DOCUMENT_SELECTION_H
#define PE_VECTOR_DOCUMENT_SELECTION_H
#include "document.h"
/** @brief Membership also supports the legacy single active rectangle. */
bool vector_document_is_selected(const VectorDocument *document, unsigned id);
/** @brief Replace rectangle selection; zero clears it. Artboard selection is kept when clearing. */
/** @brief Select a shape and all members of its persistent group, or clear with ID zero. */
void vector_document_select(VectorDocument *document, unsigned id);
/** @brief Add/remove one editable rectangle and choose a primary selection. */
/** @brief Add/remove the complete group containing the clicked shape. */
void vector_document_toggle(VectorDocument *document, unsigned id);
/** @brief Reconcile deleted IDs and external primary-selection changes between frames. */
void vector_document_selection_sync(VectorDocument *document);
#endif
