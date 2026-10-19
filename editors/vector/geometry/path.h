/** @file @brief Closed, line-segment contours and pure filled-area operations in C. */
#ifndef PE_VECTOR_GEOMETRY_PATH_H
#define PE_VECTOR_GEOMETRY_PATH_H
#include "pengui/pengui.h"
#define VECTOR_PATH_POINTS 128
#define VECTOR_PATH_CONTOURS 32

typedef struct {
    float x, y;
} VectorPoint;

/** @brief Nonzero winding: outer contours are positive, holes negative; ends are exclusive. */
typedef struct {
    unsigned count, contours;
    VectorPoint points[VECTOR_PATH_POINTS];
    unsigned ends[VECTOR_PATH_CONTOURS];
} VectorPath;

typedef enum {
    VECTOR_AREA_UNION,
    VECTOR_AREA_DIFFERENCE,
    VECTOR_AREA_INTERSECTION,
    VECTOR_AREA_XOR
} VectorAreaOperation;

/** @brief Validate finite coordinates and closed contours with at least three vertices. */
bool vector_path_valid(const VectorPath *path);
/** @brief Signed area of one contour; positive means filled outer boundary. */
float vector_path_contour_area(const VectorPath *path, unsigned contour);
/** @brief Nonzero winding hit-test with half-open edges; holes remain empty. */
bool vector_path_contains(const VectorPath *path, float x, float y);
/** @brief Intersect filled path with a box, excluding empty holes. */
bool vector_path_intersects(const VectorPath *path, PUIRect box);
/** @brief Bounds of a nonempty path. */
PUIRect vector_path_bounds(const VectorPath *path);
/** @brief Canonicalize two filled areas. Failure/complexity overflow leaves output untouched. */
bool vector_path_combine(const VectorPath *a, const VectorPath *b, VectorAreaOperation operation,
                         VectorPath *out);
/** @brief Extract connected components, retaining associated holes. Returns -1 on overflow. */
int vector_path_components(const VectorPath *path, VectorPath *out, int capacity);
/** @brief Build a solid contour band; placement -1=inside, 0=center, 1=outside. */
bool vector_path_outline(const VectorPath *path, float width, int placement, VectorPath *out);
/** @brief Emit non-overlapping triangles through a callback; no SDL dependency. */
bool vector_path_triangles(const VectorPath *path, void (*triangle)(void *, const VectorPoint[3]),
                           void *context);
#endif
