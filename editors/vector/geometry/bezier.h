/** @file @brief Editor compatibility names for independent pengDraw geometry. */
#pragma once
#include "path.h"
#include "pengdraw/pengdraw.h"
#define VECTOR_BEZIER_NODES PDRAW_BEZIER_NODES
typedef PDrawBezierNode VectorBezierNode;
typedef PDrawBezier VectorBezier;
#define VECTOR_PATH_POINTS PDRAW_PATH_POINTS

static inline PUIRect vector_bezier_bounds(const VectorBezier *p) {
    PDrawRect r = pdraw_bezier_bounds(p);
    return (PUIRect){r.x, r.y, r.w, r.h};
}

#define vector_bezier_evaluate pdraw_bezier_evaluate
#define vector_bezier_flatten pdraw_bezier_flatten
#define vector_bezier_insert pdraw_bezier_insert
#define vector_bezier_valid pdraw_bezier_valid
