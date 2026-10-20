/** @file @brief Derive virtual instances or materialize independent shapes via atomic history. */
#ifndef PE_VECTOR_REPEAT_H
#define PE_VECTOR_REPEAT_H
#include "editors/vector/document/document.h"
/** @brief Derive one cell; clears repetition and hierarchy identity on the returned value.
 * Row/column deltas add; finite dimensions/colors are clamped and angles wrap. */
bool vector_repeat_instance(const VectorShape *source, const VectorRepeat *settings,
                            unsigned column, unsigned row, VectorShape *result);
/** @brief Return the single independent editable source, otherwise NULL. */
VectorShape *vector_repeat_source(VectorDocument *document);
/** @brief Save linked settings on the original, or create real independent objects, in one step. */
bool vector_repeat_apply(VectorDocument *document, const VectorRepeat *settings, bool linked);
/** @brief Bake the selected source's saved repetition into ordinary independent objects. */
bool vector_repeat_detach(VectorDocument *document);
/** @brief Remove linked repetition while keeping the original, with undo support. */
bool vector_repeat_remove(VectorDocument *document);
#endif
