/** @file @brief Reusable grid snapping, without document or native input dependencies. */
#ifndef PE_VECTOR_GRID_SNAP_H
#define PE_VECTOR_GRID_SNAP_H
#include "pengui/pengui.h"
#include "state.h"
/** @brief Snap an artboard-local point within the screen-space tolerance.
 * Orthogonal grids snap axes independently; other grids use their nearest vertex.
 * Snapping is independent of grid visibility. Returns whether the point changed.
 */
bool vector_grid_snap_point(const VectorGrid *grid, float zoom, float *x, float *y);
/** @brief Move/resize a rectangle against grid vertices, preserving size or opposite edges.
 * A zero edge mask moves; other masks use VectorResizeHandle. Unmoved geometry stays unchanged.
 */
PUIRect vector_grid_snap_rect(const VectorGrid *grid, float zoom, PUIRect original, unsigned edges,
                              float dx, float dy, float maximum);
#endif
