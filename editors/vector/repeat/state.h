/** @file @brief Value-only procedural repetition stored on its source object. */
#ifndef PE_VECTOR_REPEAT_STATE_H
#define PE_VECTOR_REPEAT_STATE_H
#include <stdbool.h>
#define VECTOR_REPEAT_LIMIT 1024u
#define VECTOR_REPEAT_DOCUMENT_LIMIT 4096u

/** @brief Stable order of stored property deltas; also used by the property dropdown. */
typedef enum {
    VECTOR_REPEAT_WIDTH,
    VECTOR_REPEAT_HEIGHT,
    VECTOR_REPEAT_ANGLE,
    VECTOR_REPEAT_STROKE_WIDTH,
    VECTOR_REPEAT_FILL_HUE,
    VECTOR_REPEAT_FILL_SATURATION,
    VECTOR_REPEAT_FILL_LIGHTNESS,
    VECTOR_REPEAT_FILL_ALPHA,
    VECTOR_REPEAT_STROKE_HUE,
    VECTOR_REPEAT_STROKE_SATURATION,
    VECTOR_REPEAT_STROKE_LIGHTNESS,
    VECTOR_REPEAT_STROKE_ALPHA,
    VECTOR_REPEAT_PROPERTIES
} VectorRepeatProperty;

/** @brief Per-axis deltas: width, height, degrees, stroke width; fill HSLA and stroke HSLA.
 * Hue uses degrees; saturation/lightness/alpha use percentage points. */
typedef struct {
    float value[VECTOR_REPEAT_PROPERTIES];
} VectorRepeatStep;

typedef struct {
    unsigned enabled, columns, rows;
    float gap_x, gap_y; ///< Original-size spacing; grid pitch remains fixed when copies grow.
    VectorRepeatStep column, row;
} VectorRepeat;

/** @brief Zero is disabled; enabled grids include the original and contain at most 1024 cells. */
bool vector_repeat_valid(const VectorRepeat *repeat);
/** @brief Default editing configuration: five columns, four rows, eight-pixel gaps. */
VectorRepeat vector_repeat_default(void);
#endif
