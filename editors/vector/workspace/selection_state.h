/** @file @brief Plain transient selection gesture state for vector objects. */
#ifndef PE_VECTOR_SELECTION_STATE_H
#define PE_VECTOR_SELECTION_STATE_H
#include <stdbool.h>

/** @brief Stable rectangle IDs; the active rectangle is the primary selection. */
typedef struct {
    unsigned ids[64];
    int count;
} VectorRectangleSelection;

/** @brief Move/resize preview; committed geometry stays unchanged until release. */
typedef struct {
    bool dragging, marquee, additive, marquee_moved;
    VectorRectangleSelection before;
    unsigned previous_board, previous_rectangle;
    unsigned artboard_id;
    unsigned rectangle_id; ///< Zero selects the artboard itself; otherwise its rectangle child.
    unsigned resize_edges; ///< Zero for movement, otherwise a rectangular resize edge mask.
    float start_x, start_y, offset_x, offset_y;
} VectorSelectionTool;
#endif
