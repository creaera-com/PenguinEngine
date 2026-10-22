/** @file
 * @brief Object-independent mapping between rectangular selection bounds. */
#ifndef PE_VECTOR_GEOMETRY_SCALE_H
#define PE_VECTOR_GEOMETRY_SCALE_H
#include "pengui/pengui.h"
/** @brief Scale a rectangle's position and size relative to source bounds into target bounds.
 * Bounds must have positive dimensions. The caller controls minimum sizes and validity.
 */
PUIRect vector_scale_rect(PUIRect rect, PUIRect source, PUIRect target);
#endif
