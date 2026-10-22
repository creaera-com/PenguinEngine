/** @file @brief Stroke inspector field bindings. */
#ifndef PE_VECTOR_UI_STROKE_H
#define PE_VECTOR_UI_STROKE_H
#include "editors/vector/document/document.h"
/** @brief Translate a matching field into a validated undoable edit. */
bool vector_inspector_stroke_action(VectorDocument *document, VectorShape *shape, PUIView *view,
                                    const char *action);
/** @brief Refresh stroke fields without overwriting unrelated input. */
void vector_inspector_stroke_sync(VectorShape *shape, const VectorShape *previous, PUIView *view,
                                  bool reset);
#endif
