/** @file @brief Multiple-selection inspector summary. */
#ifndef PE_VECTOR_SELECTION_INSPECTOR_H
#define PE_VECTOR_SELECTION_INSPECTOR_H
#include "editors/vector/document/document.h"
/** @brief Refresh the count and kind of selected objects. */
void vector_selection_inspector_sync(VectorDocument *document, PUIView *view);
#endif
