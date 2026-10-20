/** @file @brief Validation and defaults without rendering or editor dependencies. */
#include "shadow.h"
#include <math.h>

bool vector_shadow_valid(const VectorShadow *s) {
    return s && s->enabled <= 1 && isfinite(s->offset_x) && isfinite(s->offset_y) &&
           fabsf(s->offset_x) <= 4096 && fabsf(s->offset_y) <= 4096 && isfinite(s->blur) &&
           s->blur >= 0 && s->blur <= 128 && isfinite(s->spread) && s->spread >= 0 &&
           s->spread <= 64;
}

VectorShadow vector_shadow_default(void) {
    return (VectorShadow){
        .enabled = 1, .color = 0x00000059, .offset_x = 8, .offset_y = 8, .blur = 12};
}
