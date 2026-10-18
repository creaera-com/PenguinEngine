/** @file @brief Plain transient artboard gesture state; no native input dependencies. */
#ifndef PE_VECTOR_ARTBOARD_TOOL_STATE_H
#define PE_VECTOR_ARTBOARD_TOOL_STATE_H
#include <stdbool.h>

typedef struct {
    bool enabled, dragging;
    float start_x, start_y, end_x, end_y; ///< Document pixels, independent of zoom and pan.
} VectorArtboardTool;

/** @brief Label drag preview; committed geometry stays unchanged until release. */
typedef struct {
    bool dragging;
    unsigned artboard_id;
    float start_x, start_y, offset_x, offset_y;
} VectorSelectionTool;
#endif
