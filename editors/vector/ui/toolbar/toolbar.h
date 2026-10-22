/** @file
 * @brief Toolbar presentation derived from the active document's tool state. */
#ifndef PE_VECTOR_TOOLBAR_H
#define PE_VECTOR_TOOLBAR_H
#include "editors/vector/document/document.h"
/** @brief Handle toolbar viewing preferences without changing artwork history. */
bool vector_toolbar_action(PUIView *view, VectorDocument *document, const char *action);
/** @brief Keep tool button highlights aligned with the active document; no document mutation. */
void vector_toolbar_sync(PUIView *view, const VectorDocument *document);
#endif
