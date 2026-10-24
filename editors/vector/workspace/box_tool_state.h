/** @file
 * @brief Transient rectangle/ellipse/polygon box-creation state in parent-local pixels. */
#ifndef PE_VECTOR_RECTANGLE_TOOL_STATE_H
#define PE_VECTOR_RECTANGLE_TOOL_STATE_H
#include <stdbool.h>
#include <stdint.h>

typedef struct {
    bool enabled, dragging;
    bool text;    ///< Create a text frame.
    bool polygon; ///< Choose the parametric polygon factory.
    bool ellipse; ///< Choose the ellipse factory; otherwise the rectangle factory.
    bool keep_aspect, from_center;
    unsigned parent_id;
    float parent_width, parent_height;
    float start_x, start_y, end_x, end_y;
} VectorRectangleTool;

/** @brief Transient inspector color preview, excluded from history and checkpoints. */
typedef struct {
    unsigned rectangle_id;
    uint32_t color;
    bool active;
} VectorFillPreview;
#endif
