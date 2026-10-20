/** @file @brief Object-relative fill gradients, without rendering or UI resources. */
#ifndef PE_VECTOR_GRADIENT_STATE_H
#define PE_VECTOR_GRADIENT_STATE_H
#include <stdbool.h>
#include <stdint.h>
#define VECTOR_GRADIENT_STOPS 8u

/** @brief Solid uses VectorShape.fill; reflected mirrors a linear ramp about its start. */
typedef enum {
    VECTOR_GRADIENT_SOLID,
    VECTOR_GRADIENT_LINEAR,
    VECTOR_GRADIENT_RADIAL,
    VECTOR_GRADIENT_CONICAL,
    VECTOR_GRADIENT_REFLECTED
} VectorGradientKind;

/** @brief Position in [0,1] and straight 0xRRGGBBAA color. */
typedef struct {
    float position;
    uint32_t color;
} VectorGradientStop;

/** @brief Endpoints in normalized, unrotated object coordinates. Outside endpoints are allowed. */
typedef struct {
    unsigned kind, count;
    float x1, y1, x2, y2;
    VectorGradientStop stops[VECTOR_GRADIENT_STOPS];
} VectorGradient;

/** @brief Validate endpoints, sorted stops and enum bounds, including retained disabled values. */
bool vector_gradient_valid(const VectorGradient *gradient);
/** @brief Two-stop ramp, using the existing fill (orange when fully transparent) and white. */
VectorGradient vector_gradient_default(uint32_t fill);
/** @brief Evaluate in unrotated object-local pixels with alpha-correct color interpolation. */
uint32_t vector_gradient_sample(const VectorGradient *gradient, float x, float y, float width,
                                float height);
/** @brief Insert a sampled stop in the widest interval; return its index, or -1 at capacity. */
int vector_gradient_add_stop(VectorGradient *gradient);
/** @brief Remove a stop while preserving at least two and endpoints at zero/one. */
bool vector_gradient_remove_stop(VectorGradient *gradient, unsigned index);
#endif
