/** @file
 * @brief Temporary pen/node interaction; excluded from document checkpoints. */
#ifndef PE_VECTOR_CURVE_TOOL_STATE_H
#define PE_VECTOR_CURVE_TOOL_STATE_H
#include "editors/vector/object/shape.h"

typedef struct {
    bool pen, nodes, dragging, editing;
    unsigned parent_id, shape_id;
    int node, handle; ///< Handle: 0 anchor, 1 incoming, 2 outgoing.
    VectorBezier draft, before;
    VectorShape preview;
} VectorCurveTool;
#endif
