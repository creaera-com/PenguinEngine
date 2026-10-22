/** @file
 * @brief Ellipse creation using four editable cubic segments and shared shape history. */
#ifndef PE_VECTOR_ELLIPSE_H
#define PE_VECTOR_ELLIPSE_H
#include "editors/vector/document/document.h"
/** @brief Build a normalized ellipse with rectangle-default fill; no IDs or document changes. */
bool vector_ellipse_shape(PUIRect rect, VectorShape *out);
/** @brief Add an ellipse inside an editable artboard through one undoable command. */
bool vector_ellipse_add(VectorDocument *document, unsigned parent, PUIRect rect);
/** @brief Rebuild parametric geometry and commit one undoable change, preserving style/transform.
 */
bool vector_ellipse_set_segment(VectorDocument *document, unsigned id, VectorEllipse value);
#endif
