/** @file @brief Open strokes and pen/node overlays; closed curves reuse polygon fill rendering. */
#include "render.h"
#include "editors/vector/geometry/rotation.h"
#include "editors/vector/path/render.h"
#include "tool.h"
#include <math.h>

#include "editors/vector/rendering/drawing.h"

void vector_curve_draw(SDL_Renderer *r, const VectorShape *s, PUIRect rect, float angle,
                       SDL_FRect board, float zoom, uint32_t fill, bool selected) {
    PDrawShape drawing = vector_shape_drawing(s);
    pdraw_curve_draw(r, &drawing, vector_draw_rect(rect), angle, board, zoom, fill,
                     selected ? PUI_HIGHLIGHT : 0);
}

static VectorPoint screen(const VectorDocument *d, const VectorArtboard *b, VectorPoint p) {
    const VectorViewport *v = &d->viewport;
    return (VectorPoint){v->bounds.x + v->x + (b->rect.x + p.x) * v->zoom,
                         v->bounds.y + v->y + (b->rect.y + p.y) * v->zoom};
}

void vector_curve_tool_draw(const VectorDocument *d, SDL_Renderer *r) {
    const VectorCurveTool *t = &d->curve_tool;
    VectorBezier curve = t->draft;
    const VectorArtboard *board = NULL;
    unsigned parent = t->parent_id;
    if (t->nodes && !t->editing) {
        const VectorShape *shape = NULL;
        for (int i = 0; i < d->rectangle_count; i++) {
            if (d->rectangles[i].id == d->active_rectangle) {
                shape = &d->rectangles[i];
            }
        }
        if (!shape || !shape->curve.count || shape->flags & VECTOR_OBJECT_HIDDEN) {
            return;
        }
        curve = vector_curve_parent(shape);
        parent = shape->parent_id;
    } else if (!t->pen && !t->editing) {
        return;
    }
    for (int i = 0; i < d->artboard_count; i++) {
        if (d->artboards[i].id == parent) {
            board = &d->artboards[i];
        }
    }
    if (!board || board->flags & VECTOR_OBJECT_HIDDEN) {
        return;
    }
    if (t->pen && curve.count >= 2) {
        VectorShape draft = {.stroke = {PUI_HIGHLIGHT, 1, VECTOR_STROKE_CENTER}};
        if (vector_curve_shape(&draft, &curve)) {
            SDL_FRect b = {d->viewport.bounds.x + d->viewport.x + board->rect.x * d->viewport.zoom,
                           d->viewport.bounds.y + d->viewport.y + board->rect.y * d->viewport.zoom,
                           board->rect.w * d->viewport.zoom, board->rect.h * d->viewport.zoom};
            vector_curve_draw(r, &draft, draft.rect, 0, b, d->viewport.zoom, 0, true);
        }
    }
    for (unsigned i = 0; i < curve.count; i++) {
        VectorPoint p = screen(d, board, curve.nodes[i].point);
        VectorPoint controls[] = {screen(d, board, curve.nodes[i].incoming),
                                  screen(d, board, curve.nodes[i].outgoing)};
        for (int j = 0; j < 2; j++) {
            if (hypotf(p.x - controls[j].x, p.y - controls[j].y) < 1) {
                continue;
            }
            pui_render_color(r, PUI_HIGHLIGHT);
            SDL_RenderLine(r, p.x, p.y, controls[j].x, controls[j].y);
            SDL_FRect handle = {controls[j].x - 3, controls[j].y - 3, 6, 6};
            SDL_SetRenderDrawColor(r, 242, 242, 242, 255);
            SDL_RenderFillRect(r, &handle);
            pui_render_color(r, PUI_HIGHLIGHT);
            SDL_RenderRect(r, &handle);
        }
        SDL_FRect anchor = {p.x - 3.5f, p.y - 3.5f, 7, 7};
        pui_render_color(r, (int)i == t->node ? PUI_HIGHLIGHT : 0xf2f2f2ff);
        SDL_RenderFillRect(r, &anchor);
        pui_render_color(r, PUI_HIGHLIGHT);
        SDL_RenderRect(r, &anchor);
    }
}
