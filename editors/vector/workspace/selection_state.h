/** @file
 * @brief Plain transient selection gesture state for vector objects. */
#ifndef PE_VECTOR_SELECTION_STATE_H
#define PE_VECTOR_SELECTION_STATE_H
#include "pengui/pengui.h"
#include <stdbool.h>

/** @brief Stable rectangle IDs; the active rectangle is the primary selection. */
typedef struct {
    unsigned ids[64];
    int count;
} VectorRectangleSelection;

/** @brief Move/resize preview; committed geometry stays unchanged until release. */
typedef struct {
    bool dragging, marquee, additive, marquee_moved;
    bool keep_aspect, from_center;
    bool rotating;
    float pivot_x, pivot_y, rotation_start, rotation_delta, rotation_raw;
    VectorRectangleSelection before;
    unsigned previous_board, previous_rectangle;
    unsigned artboard_id;
    unsigned rectangle_id; ///< Zero selects the artboard itself; otherwise its rectangle child.
    unsigned resize_edges; ///< Zero for movement, otherwise a rectangular resize edge mask.
    float start_x, start_y, offset_x, offset_y;
    PUIRect group_bounds;
    float group_angles[64];
    PUIRect group_originals[64]; ///< Parent-local snapshot; never modified during a gesture.
    PUIRect group_parents[64];   ///< Parent positions captured alongside original geometry.
} VectorSelectionTool;
#endif
