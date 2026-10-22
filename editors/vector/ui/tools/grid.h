/** @file
 * @brief YAML controls for per-document grid preferences. */
#ifndef PE_VECTOR_UI_GRID_H
#define PE_VECTOR_UI_GRID_H
#include "editors/vector/document/document.h"
/** @brief Route grid settings; guide preferences do not modify artwork/history. */
bool vector_grid_action(VectorDocument *document, PUIView *view, const char *action);
/** @brief Synchronize on document changes while preserving edits in focused fields. */
void vector_grid_sync(PUIView *view, const VectorDocument *document, unsigned *presented_id);
#endif
