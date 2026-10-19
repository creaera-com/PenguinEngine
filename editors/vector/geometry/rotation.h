/** @file @brief Pure clockwise rectangle rotation and hit-test geometry. */
#ifndef PE_VECTOR_GEOMETRY_ROTATION_H
#define PE_VECTOR_GEOMETRY_ROTATION_H
#include "pengui/pengui.h"
/** @brief Normalize finite degrees to [-180,180). */
float vector_angle_normalize(float degrees);
/** @brief Preserve rotation direction in [-360,360], including signed full turns. */
float vector_angle_turn(float degrees);
/** @brief Rotate a point about a pivot in screen/document coordinates. */
void vector_rotate_point(float *x, float *y, float cx, float cy, float degrees);
/** @brief Four clockwise corners, starting at the unrotated upper-left corner. */
void vector_rotated_corners(PUIRect rect, float degrees, PUIRect corners[4]);
/** @brief Axis-aligned bounds enclosing the rotated rectangle. */
PUIRect vector_rotated_bounds(PUIRect rect, float degrees);
/** @brief Exact oriented rectangle hit-test. */
bool vector_rotated_contains(PUIRect rect, float degrees, float x, float y);
/** @brief Separating-axis test against an axis-aligned box. */
bool vector_rotated_intersects(PUIRect rect, float degrees, PUIRect box);
#endif
