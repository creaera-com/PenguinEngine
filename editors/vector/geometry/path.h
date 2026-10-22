/** @file @brief Editor compatibility names for independent pengDraw geometry. */
#pragma once
#include "pengdraw/pengdraw.h"
#include "pengui/pengui.h"
#define VECTOR_PATH_POINTS PDRAW_PATH_POINTS
#define VECTOR_PATH_CONTOURS PDRAW_PATH_CONTOURS
typedef PDrawPoint VectorPoint;
typedef PDrawPath VectorPath;
typedef PDrawAreaOperation VectorAreaOperation;
#define VECTOR_AREA_DIFFERENCE PDRAW_AREA_DIFFERENCE
#define VECTOR_AREA_INTERSECTION PDRAW_AREA_INTERSECTION
#define VECTOR_AREA_UNION PDRAW_AREA_UNION
#define VECTOR_AREA_XOR PDRAW_AREA_XOR

static inline PUIRect vector_path_bounds(const VectorPath *p) {
    PDrawRect r = pdraw_path_bounds(p);
    return (PUIRect){r.x, r.y, r.w, r.h};
}

#define vector_path_combine pdraw_path_combine
#define vector_path_components pdraw_path_components
#define vector_path_contains pdraw_path_contains
#define vector_path_contour_area pdraw_path_contour_area

static inline bool vector_path_intersects(const VectorPath *p, PUIRect b) {
    return pdraw_path_intersects(p, (PDrawRect){b.x, b.y, b.w, b.h});
}

#define vector_path_outline pdraw_path_outline
#define vector_path_triangles pdraw_path_triangles
#define vector_path_valid pdraw_path_valid
