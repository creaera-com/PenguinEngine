/** @file @brief Open strokes and pen/node overlays; closed curves reuse polygon fill rendering. */
#include "render.h"
#include "editors/vector/geometry/rotation.h"
#include "editors/vector/path/render.h"
#include "tool.h"
#include <math.h>

static void line(SDL_Renderer *r, VectorPoint a, VectorPoint b, float width, uint32_t packed) {
    float length = hypotf(b.x - a.x, b.y - a.y);
    if (length <= 0 || width <= 0) {
        return;
    }
    float nx = -(b.y - a.y) * width / (2 * length), ny = (b.x - a.x) * width / (2 * length);
    SDL_FColor color = {(packed >> 24) / 255.f, ((packed >> 16) & 255) / 255.f,
                        ((packed >> 8) & 255) / 255.f, (packed & 255) / 255.f};
    SDL_Vertex v[4] = {{{a.x + nx, a.y + ny}, color, {0, 0}},
                       {{b.x + nx, b.y + ny}, color, {0, 0}},
                       {{b.x - nx, b.y - ny}, color, {0, 0}},
                       {{a.x - nx, a.y - ny}, color, {0, 0}}};
    const int indices[] = {0, 1, 2, 0, 2, 3};
    SDL_RenderGeometry(r, NULL, v, 4, indices, 6);
}

void vector_curve_draw(SDL_Renderer *r, const VectorShape *s, PUIRect rect, float angle,
                       SDL_FRect board, float zoom, uint32_t fill, bool selected) {
    if (s->curve.closed) {
        vector_path_draw(r, s, rect, angle, board, zoom, fill, selected);
        return;
    }
    VectorPath path = vector_shape_path(s, rect, angle);
    SDL_BlendMode previous;
    SDL_GetRenderDrawBlendMode(r, &previous);
    SDL_SetRenderDrawBlendMode(r, SDL_BLENDMODE_BLEND);
    for (unsigned i = 1; i < path.count; i++) {
        VectorPoint a = {board.x + path.points[i - 1].x * zoom,
                         board.y + path.points[i - 1].y * zoom};
        VectorPoint b = {board.x + path.points[i].x * zoom, board.y + path.points[i].y * zoom};
        if (s->stroke.width > 0) {
            line(r, a, b, s->stroke.width * zoom, s->stroke.color);
        }
        if (selected) {
            line(r, a, b, 1, 0xc28e5cff);
        }
    }
    SDL_SetRenderDrawBlendMode(r, previous);
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
        VectorShape draft = {.stroke = {0xc28e5cff, 1, VECTOR_STROKE_CENTER}};
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
            SDL_SetRenderDrawColor(r, 194, 142, 92, 255);
            SDL_RenderLine(r, p.x, p.y, controls[j].x, controls[j].y);
            SDL_FRect handle = {controls[j].x - 3, controls[j].y - 3, 6, 6};
            SDL_SetRenderDrawColor(r, 242, 242, 242, 255);
            SDL_RenderFillRect(r, &handle);
            SDL_SetRenderDrawColor(r, 194, 142, 92, 255);
            SDL_RenderRect(r, &handle);
        }
        SDL_FRect anchor = {p.x - 3.5f, p.y - 3.5f, 7, 7};
        SDL_SetRenderDrawColor(r, (int)i == t->node ? 194 : 242, (int)i == t->node ? 142 : 242,
                               (int)i == t->node ? 92 : 242, 255);
        SDL_RenderFillRect(r, &anchor);
        SDL_SetRenderDrawColor(r, 194, 142, 92, 255);
        SDL_RenderRect(r, &anchor);
    }
}
