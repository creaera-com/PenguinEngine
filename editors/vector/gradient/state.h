/** @file
 * @brief Editor compatibility names for independent pengDraw geometry. */
#pragma once
#include "pengdraw/pengdraw.h"
#define VECTOR_GRADIENT_STOPS PDRAW_GRADIENT_STOPS
typedef PDrawGradientKind VectorGradientKind;
typedef PDrawGradientStop VectorGradientStop;
typedef PDrawGradient VectorGradient;
#define VECTOR_GRADIENT_CONICAL PDRAW_GRADIENT_CONICAL
#define VECTOR_GRADIENT_LINEAR PDRAW_GRADIENT_LINEAR
#define VECTOR_GRADIENT_RADIAL PDRAW_GRADIENT_RADIAL
#define VECTOR_GRADIENT_REFLECTED PDRAW_GRADIENT_REFLECTED
#define VECTOR_GRADIENT_SOLID PDRAW_GRADIENT_SOLID
#define vector_gradient_add_stop pdraw_gradient_add_stop
#define vector_gradient_default pdraw_gradient_default
#define vector_gradient_remove_stop pdraw_gradient_remove_stop
#define vector_gradient_sample pdraw_gradient_sample
#define vector_gradient_valid pdraw_gradient_valid
