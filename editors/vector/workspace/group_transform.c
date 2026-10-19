/** @file @brief Resize selected rectangles relative to their shared world-space bounds. */
#include "group_transform.h"
#include "editors/vector/geometry/resize.h"
#include "editors/vector/geometry/rotation.h"
#include "editors/vector/geometry/scale.h"
#include "editors/vector/grid/snap.h"
#include "editors/vector/rectangle/transform.h"
#include <float.h>
#include <math.h>

bool vector_group_capture(VectorDocument *document) {
    VectorSelectionTool *tool = &document->selection_tool;
    for (int i = 0; i < tool->before.count; i++) {
        VectorRectangle *rectangle = vector_rectangle_find(document, tool->before.ids[i]);
        VectorArtboard *parent =
            rectangle ? vector_artboard_find(document, rectangle->parent_id) : NULL;
        if (!parent ||
            (vector_rectangle_effective_flags(document, rectangle) & VECTOR_OBJECT_STATE_MASK)) {
            return false;
        }
        tool->group_originals[i] = rectangle->rect;
        tool->group_angles[i] = rectangle->angle;
        tool->group_parents[i] = parent->rect;
        PUIRect world = rectangle->rect;
        world.x += parent->rect.x;
        world.y += parent->rect.y;
        world = vector_rotated_bounds(world, rectangle->angle);
        if (i == 0) {
            tool->group_bounds = world;
        } else {
            PUIRect *bounds = &tool->group_bounds;
            float right = fmaxf(bounds->x + bounds->w, world.x + world.w);
            float bottom = fmaxf(bounds->y + bounds->h, world.y + world.h);
            bounds->x = fminf(bounds->x, world.x);
            bounds->y = fminf(bounds->y, world.y);
            bounds->w = right - bounds->x;
            bounds->h = bottom - bounds->y;
        }
    }
    return tool->before.count > 1;
}

static PUIRect target_bounds(const VectorDocument *document) {
    const VectorSelectionTool *tool = &document->selection_tool;
    PUIRect source = tool->group_bounds, origin = {0};
    bool aspect = tool->keep_aspect;
    float minimum_x = 0, minimum_y = 0, maximum_x = FLT_MAX, maximum_y = FLT_MAX;
    for (int i = 0; i < tool->before.count; i++) {
        aspect |= vector_angle_normalize(tool->group_angles[i]) != 0;
        PUIRect r = tool->group_originals[i];
        minimum_x = fmaxf(minimum_x, 1 / r.w);
        minimum_y = fmaxf(minimum_y, 1 / r.h);
        maximum_x = fminf(maximum_x, VECTOR_MAX_ARTBOARD_SIZE / r.w);
        maximum_y = fminf(maximum_y, VECTOR_MAX_ARTBOARD_SIZE / r.h);
        if (tool->before.ids[i] == tool->rectangle_id) {
            origin = tool->group_parents[i];
        }
    }
    PUIRect local = source;
    local.x -= origin.x;
    local.y -= origin.y;
    PUIRect target = vector_grid_resize_constrained(
        &document->grid, document->viewport.zoom, local, tool->resize_edges, tool->offset_x,
        tool->offset_y, aspect, tool->from_center, source.w * minimum_x, source.h * minimum_y,
        source.w * maximum_x, source.h * maximum_y);
    target.x += origin.x;
    target.y += origin.y;
    return target;
}

static bool preview_all(const VectorDocument *document, PUIRect rects[VECTOR_MAX_RECTANGLES]) {
    const VectorSelectionTool *tool = &document->selection_tool;
    PUIRect source = tool->group_bounds, target = target_bounds(document);
    for (int i = 0; i < tool->before.count; i++) {
        PUIRect r = tool->group_originals[i], parent = tool->group_parents[i];
        if (tool->offset_x != 0 || tool->offset_y != 0) {
            r.x += parent.x;
            r.y += parent.y;
            r = vector_scale_rect(r, source, target);
            r.x -= parent.x;
            r.y -= parent.y;
            if (source.x == target.x && source.w == target.w) {
                r.x = tool->group_originals[i].x;
                r.w = tool->group_originals[i].w;
            }
            if (source.y == target.y && source.h == target.h) {
                r.y = tool->group_originals[i].y;
                r.h = tool->group_originals[i].h;
            }
            r.w = fmaxf(1, r.w);
            r.h = fmaxf(1, r.h);
        }
        if (!isfinite(r.x) || !isfinite(r.y) || !isfinite(r.w) || !isfinite(r.h) ||
            fabsf(r.x) > VECTOR_MAX_ARTBOARD_SIZE || fabsf(r.y) > VECTOR_MAX_ARTBOARD_SIZE ||
            r.w > VECTOR_MAX_ARTBOARD_SIZE || r.h > VECTOR_MAX_ARTBOARD_SIZE) {
            return false;
        }
        rects[i] = r;
    }
    return true;
}

PUIRect vector_group_display_rect(const VectorDocument *document,
                                  const VectorRectangle *rectangle) {
    const VectorSelectionTool *tool = &document->selection_tool;
    PUIRect rects[VECTOR_MAX_RECTANGLES];
    bool valid = preview_all(document, rects);
    for (int i = 0; i < tool->before.count; i++) {
        if (tool->before.ids[i] == rectangle->id) {
            return valid ? rects[i] : tool->group_originals[i];
        }
    }
    return rectangle->rect;
}

bool vector_group_commit(VectorDocument *document) {
    const VectorSelectionTool *tool = &document->selection_tool;
    PUIRect rects[VECTOR_MAX_RECTANGLES];
    return preview_all(document, rects) &&
           vector_rectangles_set_rects(document, tool->before.ids, rects, tool->before.count);
}
