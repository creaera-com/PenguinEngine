/** @file @brief YAML bindings for shape reflection and quarter-turn operations. */
#ifndef PE_VECTOR_UI_ORIENTATION_ACTIONS_H
#define PE_VECTOR_UI_ORIENTATION_ACTIONS_H
#include "editors/vector/document/document.h"
/** @brief Route a semantic orientation action to the document domain. */
bool vector_orientation_actions_action(VectorDocument *document, PUIView *view, const char *name);
/** @brief Update submenu availability when the context menu opens. */
void vector_orientation_actions_sync(VectorDocument *document, PUIView *view,
                                     bool alignment_available);
#endif
