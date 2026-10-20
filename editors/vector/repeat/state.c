/** @file @brief Validate repetition parameters without allocation, UI or renderer dependencies. */
#include "state.h"
#include <math.h>
#include <string.h>

bool vector_repeat_valid(const VectorRepeat *r) {
    if (!r->enabled) {
        const VectorRepeat empty = {0};
        return !memcmp(r, &empty, sizeof empty);
    }
    if (r->enabled != 1 || !r->columns || !r->rows || r->columns > 64 || r->rows > 64 ||
        r->columns * r->rows > VECTOR_REPEAT_LIMIT || !isfinite(r->gap_x) || !isfinite(r->gap_y) ||
        fabsf(r->gap_x) > 1000000 || fabsf(r->gap_y) > 1000000) {
        return false;
    }
    for (int i = 0; i < VECTOR_REPEAT_PROPERTIES; i++) {
        if (!isfinite(r->column.value[i]) || !isfinite(r->row.value[i]) ||
            fabsf(r->column.value[i]) > 1000000 || fabsf(r->row.value[i]) > 1000000) {
            return false;
        }
    }
    return true;
}

VectorRepeat vector_repeat_default(void) {
    return (VectorRepeat){.enabled = 1, .columns = 5, .rows = 4, .gap_x = 8, .gap_y = 8};
}
