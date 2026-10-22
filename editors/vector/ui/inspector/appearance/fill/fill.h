/** @file
 * @brief Fill inspector field bindings. */
#ifndef PE_VECTOR_UI_FILL_H
#define PE_VECTOR_UI_FILL_H
#include "editors/vector/document/document.h"
/** @brief Translate a matching field into a validated undoable edit. */
bool vector_inspector_fill_action(VectorDocument *document, VectorShape *shape, PUIView *view,
                                  const char *action);
/** @brief Refresh fill fields without overwriting unrelated input. */
void vector_inspector_fill_sync(VectorDocument *document, VectorShape *shape,
                                const VectorShape *previous, PUIView *view, bool reset);
#endif
