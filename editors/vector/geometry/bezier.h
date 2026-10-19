/** @file @brief Editable cubic Bezier anchors and pure, bounded curve geometry. */
#ifndef PE_VECTOR_GEOMETRY_BEZIER_H
#define PE_VECTOR_GEOMETRY_BEZIER_H
#include "path.h"
#define VECTOR_BEZIER_NODES 32

/** @brief Absolute anchor and incoming/outgoing control points in the same coordinate space. */
typedef struct {
    VectorPoint point, incoming, outgoing;
} VectorBezierNode;

/** @brief One open or closed contour. Shapes store normalized values; tools use parent pixels. */
typedef struct {
    unsigned count;
    unsigned closed; ///< Zero=open, one=closed; validated before reading checkpoint values.
    VectorBezierNode nodes[VECTOR_BEZIER_NODES];
} VectorBezier;

/** @brief Validate finite bounded coordinates and the fixed node capacity; empty is valid. */
bool vector_bezier_valid(const VectorBezier *curve);
/** @brief Evaluate the cubic segment from node a to b at t in [0,1]. */
VectorPoint vector_bezier_evaluate(VectorBezierNode a, VectorBezierNode b, float t);
/** @brief Exact cubic-extrema bounds; control handles themselves do not enlarge the shape. */
PUIRect vector_bezier_bounds(const VectorBezier *curve);
/** @brief Adaptive de Casteljau approximation, relaxing tolerance to respect VECTOR_PATH_POINTS.
 * Open results may have two points and are polylines, not filled VectorPaths. */
bool vector_bezier_flatten(const VectorBezier *curve, float tolerance, VectorPath *out);
/** @brief Split one cubic at t without changing its shape; inserts one editable anchor.
 * Returns false for invalid segments, endpoints or full capacity; input stays unchanged. */
bool vector_bezier_insert(VectorBezier *curve, unsigned segment, float t);
#endif
