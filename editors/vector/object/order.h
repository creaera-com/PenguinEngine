/** @file @brief Sibling-only stacking order independent of UI, geometry and clipboard data. */
#ifndef PE_VECTOR_OBJECT_ORDER_H
#define PE_VECTOR_OBJECT_ORDER_H
#include "editors/vector/document/selection.h"

/** @brief Higher paint indices appear nearer the viewer and higher in the document tree. */
typedef enum {
    VECTOR_ORDER_FRONT,
    VECTOR_ORDER_BACK,
    VECTOR_ORDER_FORWARD,
    VECTOR_ORDER_BACKWARD
} VectorOrder;

/** @brief Whether the selection can move; boundaries, locks and history capacity are respected. */
bool vector_objects_can_order(VectorDocument *document, VectorOrder order);
/** @brief Reorder each selected sibling set atomically, preserving selection and relative order. */
bool vector_objects_order(VectorDocument *document, VectorOrder order);
/** @brief Pointer-free private command codec, type 7. */
bool vector_order_command_write(const VectorCommand *command, FILE *file);
bool vector_order_command_read(VectorCommand *command, FILE *file);
#endif
