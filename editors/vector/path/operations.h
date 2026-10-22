/** @file
 * @brief Undoable conversion and filled-area operations for selected vector shapes. */
#ifndef PE_VECTOR_PATH_OPERATIONS_H
#define PE_VECTOR_PATH_OPERATIONS_H
#include "editors/vector/document/document.h"

typedef enum {
    VECTOR_PATH_CONVERT,
    VECTOR_PATH_STROKE,
    VECTOR_PATH_UNION,
    VECTOR_PATH_SUBTRACT,
    VECTOR_PATH_INTERSECT,
    VECTOR_PATH_XOR,
    VECTOR_PATH_DIVIDE
} VectorPathOperation;

/** @brief Cheap eligibility check, before any expensive tessellation. */
bool vector_paths_can_operate(VectorDocument *document, VectorPathOperation operation);
/** @brief Complete operation transactionally. Geometric overflow reports failure without edits. */
bool vector_paths_operate(VectorDocument *document, VectorPathOperation operation);
#endif
