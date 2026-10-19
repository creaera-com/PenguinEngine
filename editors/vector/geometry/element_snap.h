/** @file @brief Pure bounding-box alignment and equal-gap snapping in document coordinates. */
#ifndef PE_VECTOR_ELEMENT_SNAP_H
#define PE_VECTOR_ELEMENT_SNAP_H
#include "pengui/pengui.h"
#define VECTOR_SNAP_MAX_GUIDES 6

/** @brief Dashed alignment line or equal-spacing bracket between two endpoints. */
typedef struct {
    float x1, y1, x2, y2;
    bool spacing;
} VectorSnapGuide;

/** @brief Per-axis correction and final guide geometry; never owns document data. */
typedef struct {
    float dx, dy;
    bool snapped_x, snapped_y;
    VectorSnapGuide guides[VECTOR_SNAP_MAX_GUIDES];
    int guide_count;
} VectorElementSnap;

/** @brief Match left/center/right and top/center/bottom anchors, or equal gaps between neighbours.
 * tolerance is in document pixels. Targets must be finite positive rectangles.
 * Nearest correction wins per axis; alignment wins equal-distance ties. */
VectorElementSnap vector_element_snap(PUIRect moving, const PUIRect *targets, int count,
                                      float tolerance);
#endif
