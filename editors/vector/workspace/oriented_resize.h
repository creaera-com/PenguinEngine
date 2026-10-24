/** @file
 * @brief Resize in a rotated rectangle's own axes, preserving its world anchor. */
#ifndef PE_VECTOR_ORIENTED_RESIZE_H
#define PE_VECTOR_ORIENTED_RESIZE_H
#include "editors/vector/document/document.h"
PUIRect vector_oriented_resize(const VectorDocument *document, const VectorRectangle *rectangle);
#endif
