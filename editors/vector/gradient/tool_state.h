/** @file @brief Transient gradient handles; completed drags alone become document edits. */
#ifndef PE_VECTOR_GRADIENT_TOOL_STATE_H
#define PE_VECTOR_GRADIENT_TOOL_STATE_H
#include "state.h"

typedef struct {
    bool enabled, dragging;
    unsigned shape_id, handle;
    VectorGradient draft;
} VectorGradientTool;
#endif
