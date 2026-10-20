/** @file @brief Non-destructive per-object drop-shadow values in document pixels. */
#ifndef PE_VECTOR_EFFECTS_SHADOW_H
#define PE_VECTOR_EFFECTS_SHADOW_H
#include <stdbool.h>
#include <stdint.h>

/** @brief Color alpha is opacity; blur is softness radius, spread expands the alpha silhouette. */
typedef struct {
    unsigned enabled;
    uint32_t color;
    float offset_x, offset_y, blur, spread;
} VectorShadow;

/** @brief Validate finite, bounded settings, including disabled effects. */
bool vector_shadow_valid(const VectorShadow *shadow);
/** @brief Subtle black shadow, offset 8 px, softness 12 px, opacity 35 percent. */
VectorShadow vector_shadow_default(void);
#endif
