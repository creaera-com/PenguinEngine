/** @file @brief Transient drag-to-create rectangle state in parent-local pixels. */
#ifndef PE_VECTOR_RECTANGLE_TOOL_STATE_H
#define PE_VECTOR_RECTANGLE_TOOL_STATE_H
#include <stdbool.h>
#include <stdint.h>

typedef struct {
    bool enabled, dragging;
    unsigned parent_id;
    float start_x, start_y, end_x, end_y;
} VectorRectangleTool;

/** @brief Transient inspector color preview, excluded from history and checkpoints. */
typedef struct {
    unsigned rectangle_id;
    uint32_t color;
    bool active;
} VectorFillPreview;
#endif
