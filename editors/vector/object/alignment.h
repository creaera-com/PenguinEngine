/** @file
 * @brief Selection alignment in document coordinates, independent of UI and grid snapping.
 */
#ifndef PE_VECTOR_OBJECT_ALIGNMENT_H
#define PE_VECTOR_OBJECT_ALIGNMENT_H
#include "editors/vector/document/selection.h"

/** @brief Targets use the combined rotated geometric bounds of the selection. */
typedef enum {
    VECTOR_ALIGN_LEFT,
    VECTOR_ALIGN_CENTER_X,
    VECTOR_ALIGN_RIGHT,
    VECTOR_DISTRIBUTE_X,
    VECTOR_ALIGN_TOP,
    VECTOR_ALIGN_CENTER_Y,
    VECTOR_ALIGN_BOTTOM,
    VECTOR_DISTRIBUTE_Y,
    VECTOR_ALIGNMENT_COUNT
} VectorAlignment;

/** @brief Preflight: at least two editable rectangles, three for equal edge gaps; no-op is false.
 */
bool vector_objects_can_align(VectorDocument *document, VectorAlignment alignment);
/** @brief Translate selected rectangles atomically through transform history; parents stay fixed.
 */
bool vector_objects_align(VectorDocument *document, VectorAlignment alignment);
#endif
