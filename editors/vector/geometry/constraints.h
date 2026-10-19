/** @file @brief Pure geometry for centered and aspect-constrained drawing/resizing. */
#ifndef PE_VECTOR_GEOMETRY_CONSTRAINTS_H
#define PE_VECTOR_GEOMETRY_CONSTRAINTS_H
#include "pengui/pengui.h"
/** @brief Construct a rectangle from anchor/endpoint. Optional bounds constrain it while
 * preserving its anchor/center and square constraint. Creation may have zero size. */
PUIRect vector_draw_constrained(float x, float y, float end_x, float end_y, bool square,
                                bool centered, const PUIRect *bounds);
/** @brief Resize with opposite-edge or center anchor. Aspect uses the original ratio;
 * side handles keep the other axis centered. Limits take precedence over the pointer. */
PUIRect vector_resize_constrained(PUIRect original, unsigned edges, float dx, float dy, bool aspect,
                                  bool centered, float min_w, float min_h, float max_w,
                                  float max_h);
#endif
