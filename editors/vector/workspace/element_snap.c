/** @file @brief Smart-guide targets and overlay stay separate from selection/history. */
#include "element_snap.h"
#include "editors/vector/document/selection.h"
#include "editors/vector/geometry/rotation.h"
#include <math.h>

static PUIRect world_rect(const VectorDocument *d, const VectorRectangle *r) {
    PUIRect rect = vector_rotated_bounds(r->rect, r->angle);
    for (int i = 0; i < d->artboard_count; i++) {
        if (d->artboards[i].id == r->parent_id) {
            rect.x += d->artboards[i].rect.x;
            rect.y += d->artboards[i].rect.y;
            break;
        }
    }
    return rect;
}

static bool visible(const VectorDocument *d, const VectorRectangle *r) {
    for (int i = 0; i < d->artboard_count; i++) {
        if (d->artboards[i].id == r->parent_id) {
            return !((r->flags | d->artboards[i].flags) & VECTOR_OBJECT_HIDDEN);
        }
    }
    return false;
}

static PUIRect unite(PUIRect a, PUIRect b) {
    float x = fminf(a.x, b.x), y = fminf(a.y, b.y);
    return (PUIRect){x, y, fmaxf(a.x + a.w, b.x + b.w) - x, fmaxf(a.y + a.h, b.y + b.h) - y};
}

VectorElementSnap vector_workspace_element_snap(const VectorDocument *d) {
    const VectorSelectionTool *tool = &d->selection_tool;
    if (!d->element_snap_enabled || !tool->dragging || tool->marquee || tool->rotating ||
        tool->resize_edges || d->viewport.zoom <= 0) {
        return (VectorElementSnap){0};
    }
    PUIRect targets[VECTOR_MAX_ARTBOARDS + VECTOR_MAX_RECTANGLES], moving = {0};
    int count = 0;
    bool found = false;
    for (int i = 0; i < d->artboard_count; i++) {
        const VectorArtboard *b = &d->artboards[i];
        if (b->flags & VECTOR_OBJECT_HIDDEN) {
            continue;
        }
        if (!tool->rectangle_id && b->id == tool->artboard_id) {
            moving = b->rect;
            found = true;
        } else {
            targets[count++] = b->rect;
        }
    }
    for (int i = 0; i < d->rectangle_count; i++) {
        const VectorRectangle *r = &d->rectangles[i];
        if (!visible(d, r)) {
            continue;
        }
        PUIRect rect = world_rect(d, r);
        if (tool->rectangle_id && vector_document_is_selected(d, r->id)) {
            moving = found ? unite(moving, rect) : rect;
            found = true;
        } else if (tool->rectangle_id || r->parent_id != tool->artboard_id) {
            targets[count++] = rect;
        }
    }
    if (!found) {
        return (VectorElementSnap){0};
    }
    moving.x += tool->offset_x;
    moving.y += tool->offset_y;
    return vector_element_snap(moving, targets, count, d->grid.snap_tolerance / d->viewport.zoom);
}

static void dashed(SDL_Renderer *r, float x1, float y1, float x2, float y2) {
    float dx = x2 - x1, dy = y2 - y1, length = hypotf(dx, dy);
    if (length <= 0) {
        return;
    }
    dx /= length;
    dy /= length;
    /* Bound work to the visible canvas, even with distant objects or very large artboards. */
    SDL_Rect clip;
    SDL_GetRenderClipRect(r, &clip);
    float start = 0, end = length;
    if (SDL_RenderClipEnabled(r)) {
        if (fabsf(dx) > .5f) {
            start = fmaxf(0, clip.x - x1);
            end = fminf(length, clip.x + clip.w - x1);
        } else {
            start = fmaxf(0, clip.y - y1);
            end = fminf(length, clip.y + clip.h - y1);
        }
    }
    for (float t = floorf(start / 9) * 9; t < end; t += 9) {
        float stop = fminf(t + 5, end);
        SDL_RenderLine(r, x1 + dx * t, y1 + dy * t, x1 + dx * stop, y1 + dy * stop);
    }
}

void vector_element_guides_draw(const VectorDocument *d, SDL_Renderer *renderer) {
    VectorElementSnap snap = vector_workspace_element_snap(d);
    const VectorViewport *v = &d->viewport;
    pui_render_color(renderer, PUI_HIGHLIGHT);
    for (int i = 0; i < snap.guide_count; i++) {
        VectorSnapGuide g = snap.guides[i];
        float x1 = v->bounds.x + v->x + g.x1 * v->zoom;
        float y1 = v->bounds.y + v->y + g.y1 * v->zoom;
        float x2 = v->bounds.x + v->x + g.x2 * v->zoom;
        float y2 = v->bounds.y + v->y + g.y2 * v->zoom;
        dashed(renderer, x1, y1, x2, y2);
        if (g.spacing) {
            bool horizontal = fabsf(y2 - y1) < .01f;
            float dx = horizontal ? 0 : 3, dy = horizontal ? 3 : 0;
            SDL_RenderLine(renderer, x1 - dx, y1 - dy, x1 + dx, y1 + dy);
            SDL_RenderLine(renderer, x2 - dx, y2 - dy, x2 + dx, y2 + dy);
        }
    }
}
