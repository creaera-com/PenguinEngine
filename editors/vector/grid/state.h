/** @file @brief Per-document drawing guides, independent of tools and document history. */
#ifndef PE_VECTOR_GRID_STATE_H
#define PE_VECTOR_GRID_STATE_H
#include <stdbool.h>
#include <stdint.h>

typedef enum { VECTOR_GRID_STANDARD, VECTOR_GRID_HEXAGONAL } VectorGridType;

/** @brief Width/height describe a cell's bounding box in document pixels. */
typedef struct {
    bool visible;
    VectorGridType type;
    float width, height;
    uint32_t color;
} VectorGrid;
#endif
