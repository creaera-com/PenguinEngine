/** @file
 * @brief Object-independent rectangular resize handles and constrained geometry. */
#ifndef PE_VECTOR_RESIZE_H
#define PE_VECTOR_RESIZE_H
#include "pengui/pengui.h"

/** @brief Edge mask; corners combine two edges. Zero means no handle. */
typedef enum {
    VECTOR_RESIZE_NONE = 0,
    VECTOR_RESIZE_TOP = 1,
    VECTOR_RESIZE_RIGHT = 2,
    VECTOR_RESIZE_BOTTOM = 4,
    VECTOR_RESIZE_LEFT = 8
} VectorResizeHandle;

/** @brief Eight square handles, clockwise from the upper-left corner; size uses screen pixels. */
PUIRect vector_resize_handle_rect(PUIRect bounds, int index, float size);
/** @brief Edge mask for a clockwise handle index, or NONE for an invalid index. */
unsigned vector_resize_handle_edges(int index);
/** @brief Hit-test handles in screen coordinates. Corners take priority on tiny rectangles. */
unsigned vector_resize_hit(PUIRect bounds, float x, float y, float size);
/** @brief Resize in document units, keeping opposite edges fixed; minimum size is one pixel. */
PUIRect vector_resize_rect(PUIRect original, unsigned edges, float dx, float dy, float maximum);
#endif
