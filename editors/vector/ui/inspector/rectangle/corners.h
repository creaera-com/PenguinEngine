/** @file @brief Rectangle corner inspector field bindings. */
#ifndef PE_VECTOR_UI_CORNERS_H
#define PE_VECTOR_UI_CORNERS_H
#include "editors/vector/document/document.h"
/** @brief Translate a matching field into a validated undoable edit. */
bool vector_inspector_corners_action(VectorDocument *document, VectorShape *shape, PUIView *view,
                                     const char *action);
/** @brief Refresh rectangle corner fields without overwriting unrelated input. */
void vector_inspector_corners_sync(VectorShape *shape, const VectorShape *previous, PUIView *view,
                                   bool reset);
#endif
