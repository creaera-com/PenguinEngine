/** @file @brief Transient drag-to-create rectangle state in parent-local pixels. */
#ifndef PE_VECTOR_RECTANGLE_TOOL_STATE_H
#define PE_VECTOR_RECTANGLE_TOOL_STATE_H
#include <stdbool.h>

typedef struct {
    bool enabled, dragging;
    unsigned parent_id;
    float start_x, start_y, end_x, end_y;
} VectorRectangleTool;
#endif
