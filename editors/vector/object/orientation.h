/** @file
 * @brief Mirrors and exact quarter-turns of shapes around their shared selection center. */
#ifndef PE_VECTOR_OBJECT_ORIENTATION_H
#define PE_VECTOR_OBJECT_ORIENTATION_H
#include "editors/vector/document/document.h"

typedef enum {
    VECTOR_MIRROR_HORIZONTAL, ///< Swap left/right in document coordinates.
    VECTOR_MIRROR_VERTICAL,   ///< Swap top/bottom in document coordinates.
    VECTOR_ROTATE_LEFT,       ///< Counterclockwise 90 degrees.
    VECTOR_ROTATE_RIGHT       ///< Clockwise 90 degrees.
} VectorOrientation;

/** @brief Validate eligibility, target geometry and locks without changing the document. */
bool vector_objects_can_orient(VectorDocument *document, VectorOrientation operation);
/** @brief One atomic history command, retaining IDs, parents, style, order and selection. */
bool vector_objects_orient(VectorDocument *document, VectorOrientation operation);
#endif
