/** @file @brief Convert pointer deltas and raster candidates into object-local resize axes. */
#include "oriented_resize.h"
#include "editors/vector/geometry/constraints.h"
#include "editors/vector/geometry/resize.h"
#include "editors/vector/geometry/rotation.h"
#include "editors/vector/grid/snap.h"

PUIRect vector_oriented_resize(const VectorDocument *document, const VectorRectangle *rectangle) {
    const VectorSelectionTool *tool = &document->selection_tool;
    if (tool->offset_x == 0 && tool->offset_y == 0) {
        return rectangle->rect;
    }
    float dx = tool->offset_x, dy = tool->offset_y, angle = rectangle->angle;
    vector_rotate_point(&dx, &dy, 0, 0, -angle);
    PUIRect source = {-rectangle->rect.w / 2, -rectangle->rect.h / 2, rectangle->rect.w,
                      rectangle->rect.h};
    PUIRect target = vector_resize_constrained(source, tool->resize_edges, dx, dy,
                                               tool->keep_aspect, tool->from_center, 1, 1,
                                               VECTOR_MAX_ARTBOARD_SIZE, VECTOR_MAX_ARTBOARD_SIZE);
    float cx = rectangle->rect.x + rectangle->rect.w / 2,
          cy = rectangle->rect.y + rectangle->rect.h / 2;
    if (tool->offset_x != 0 || tool->offset_y != 0) {
        unsigned edges = tool->resize_edges;
        bool horizontal = (edges & (VECTOR_RESIZE_LEFT | VECTOR_RESIZE_RIGHT)) != 0;
        bool vertical = (edges & (VECTOR_RESIZE_TOP | VECTOR_RESIZE_BOTTOM)) != 0;
        float hx = target.x + (edges & VECTOR_RESIZE_LEFT    ? 0
                               : edges & VECTOR_RESIZE_RIGHT ? target.w
                                                             : target.w / 2);
        float hy = target.y + (edges & VECTOR_RESIZE_TOP      ? 0
                               : edges & VECTOR_RESIZE_BOTTOM ? target.h
                                                              : target.h / 2);
        vector_rotate_point(&hx, &hy, 0, 0, angle);
        hx += cx;
        hy += cy;
        if (vector_grid_snap_point(&document->grid, document->viewport.zoom, &hx, &hy)) {
            hx -= cx;
            hy -= cy;
            vector_rotate_point(&hx, &hy, 0, 0, -angle);
            float ox = edges & VECTOR_RESIZE_LEFT ? source.x : source.x + source.w;
            float oy = edges & VECTOR_RESIZE_TOP ? source.y : source.y + source.h;
            target = vector_resize_constrained(
                source, edges, horizontal ? hx - ox : 0, vertical ? hy - oy : 0, tool->keep_aspect,
                tool->from_center, 1, 1, VECTOR_MAX_ARTBOARD_SIZE, VECTOR_MAX_ARTBOARD_SIZE);
        }
    }
    float tx = target.x + target.w / 2, ty = target.y + target.h / 2;
    vector_rotate_point(&tx, &ty, 0, 0, angle);
    return (PUIRect){cx + tx - target.w / 2, cy + ty - target.h / 2, target.w, target.h};
}
