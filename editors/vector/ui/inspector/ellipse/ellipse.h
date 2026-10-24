/** @file
 * @brief Parametric ellipse inspector bindings. */
#ifndef PE_VECTOR_UI_ELLIPSE_H
#define PE_VECTOR_UI_ELLIPSE_H
#include "editors/vector/document/document.h"
/** @brief Route segment fields to a single validated history command. */
bool vector_inspector_ellipse_action(VectorDocument *document, VectorShape *shape, PUIView *view,
                                     const char *action);
/** @brief Refresh committed parameters without replacing unrelated edits. */
void vector_inspector_ellipse_sync(const VectorShape *shape, const VectorShape *previous,
                                   PUIView *view, bool reset);
#endif
