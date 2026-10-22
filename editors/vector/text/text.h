/** @file
 * @brief History-backed text frame creation and paragraph updates. */
#ifndef PE_VECTOR_TEXT_H
#define PE_VECTOR_TEXT_H
#include "editors/vector/document/document.h"
/** @brief Default editable text frame with 24px bundled font. */
bool vector_text_shape(PUIRect rect, VectorShape *shape);
/** @brief Add a frame to an editable artboard. */
bool vector_text_add(VectorDocument *document, unsigned parent, PUIRect rect);
/** @brief Atomically change content and paragraph style, preserving transform and fill. */
bool vector_text_update(VectorDocument *document, unsigned id, const VectorText *text);
#endif
