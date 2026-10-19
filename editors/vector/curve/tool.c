/** @file @brief Mouse gestures produce temporary parent-local curves; release commits once. */
#include "tool.h"
#include "editors/vector/artboard/tool.h"
#include "editors/vector/document/selection.h"
#include "editors/vector/grid/snap.h"
#include <math.h>
#include <string.h>

void vector_curve_tool_cancel(VectorDocument *d) {
    if (d->curve_tool.dragging) {
        SDL_CaptureMouse(false);
    }
    d->curve_tool = (VectorCurveTool){0};
}

bool vector_curve_tool_action(VectorDocument *d, PUIView *view, const char *action) {
    bool pen = !strcmp(action, "vector.tool_pen"), nodes = !strcmp(action, "vector.tool_nodes");
    if (!pen && !nodes) {
        return false;
    }
    if (!d) {
        return true;
    }
    bool was = pen ? d->curve_tool.pen : d->curve_tool.nodes;
    vector_artboard_tool_cancel(d);
    d->curve_tool.pen = pen && !was;
    d->curve_tool.nodes = nodes && !was;
    d->curve_tool.node = -1;
    pui_view_set_text(
        view, "status",
        pen ? "Stift: klicken/ziehen; Startpunkt schliesst; Enter beendet; Esc verwirft"
            : "Knoten: Punkte/Griffe ziehen; Alt trennt Griffe; Entf loescht Knoten");
    return true;
}

const VectorShape *vector_curve_display_shape(const VectorDocument *d, const VectorShape *s) {
    return d->curve_tool.editing && d->curve_tool.dragging && d->curve_tool.shape_id == s->id
               ? &d->curve_tool.preview
               : s;
}

static VectorPoint local(const VectorDocument *d, const VectorArtboard *b, float x, float y,
                         bool snap) {
    const VectorViewport *v = &d->viewport;
    VectorPoint p = {(x - v->bounds.x - v->x) / v->zoom - b->rect.x,
                     (y - v->bounds.y - v->y) / v->zoom - b->rect.y};
    if (snap) {
        vector_grid_snap_point(&d->grid, v->zoom, &p.x, &p.y);
    }
    return p;
}

static float screen_distance(const VectorDocument *d, VectorPoint a, VectorPoint b) {
    return hypotf(a.x - b.x, a.y - b.y) * d->viewport.zoom;
}

static bool on_canvas(VectorDocument *d, PUIView *view, float x, float y) {
    char id[96];
    snprintf(id, sizeof id, "vector_document_%u.canvas", d->id);
    return pui_view_hit(view, id, x, y);
}

static bool finish(VectorDocument *d, PUIView *view, bool closed) {
    VectorCurveTool *t = &d->curve_tool;
    if (t->draft.count < 2) {
        return false;
    }
    t->draft.closed = closed;
    if (!vector_curve_add(d, t->parent_id, &t->draft)) {
        t->draft.closed = false;
        pui_view_set_text(view, "status", "Kurve konnte nicht angelegt werden");
        return false;
    }
    SDL_CaptureMouse(false);
    *t = (VectorCurveTool){.pen = true};
    return true;
}

static bool pick_control(const VectorDocument *d, const VectorBezier *c, VectorPoint p, int *node,
                         int *handle) {
    for (unsigned i = 0; i < c->count; i++) {
        if (screen_distance(d, c->nodes[i].point, p) <= 6) {
            *node = (int)i;
            *handle = 0;
            return true;
        }
    }
    for (unsigned i = 0; i < c->count; i++) {
        VectorPoint points[] = {c->nodes[i].incoming, c->nodes[i].outgoing};
        for (int j = 0; j < 2; j++) {
            if (screen_distance(d, points[j], c->nodes[i].point) > 1 &&
                screen_distance(d, points[j], p) <= 6) {
                *node = (int)i;
                *handle = j + 1;
                return true;
            }
        }
    }
    return false;
}

static void update(VectorDocument *d, VectorArtboard *board, float x, float y) {
    VectorCurveTool *t = &d->curve_tool;
    VectorPoint p = local(d, board, x, y, true);
    VectorBezierNode *n = &t->draft.nodes[t->node];
    if (t->pen) {
        if (SDL_GetModState() & SDL_KMOD_SHIFT) {
            float a =
                roundf(atan2f(p.y - n->point.y, p.x - n->point.x) / .785398163f) * .785398163f;
            float length = hypotf(p.x - n->point.x, p.y - n->point.y);
            p = (VectorPoint){n->point.x + cosf(a) * length, n->point.y + sinf(a) * length};
        }
        n->outgoing = p;
        n->incoming = (VectorPoint){2 * n->point.x - p.x, 2 * n->point.y - p.y};
    } else if (!t->handle) {
        VectorBezierNode before = t->before.nodes[t->node];
        float dx = p.x - before.point.x, dy = p.y - before.point.y;
        *n = before;
        n->point = p;
        n->incoming.x += dx;
        n->incoming.y += dy;
        n->outgoing.x += dx;
        n->outgoing.y += dy;
    } else {
        VectorPoint *point = t->handle == 1 ? &n->incoming : &n->outgoing;
        VectorPoint *opposite = t->handle == 1 ? &n->outgoing : &n->incoming;
        *point = p;
        if (!(SDL_GetModState() & SDL_KMOD_ALT)) {
            float length = hypotf(opposite->x - n->point.x, opposite->y - n->point.y);
            float distance = hypotf(p.x - n->point.x, p.y - n->point.y);
            if (distance > 0) {
                opposite->x = n->point.x - (p.x - n->point.x) * length / distance;
                opposite->y = n->point.y - (p.y - n->point.y) * length / distance;
            }
        }
    }
    if (t->editing) {
        VectorShape *original = vector_rectangle_find(d, t->shape_id);
        if (original) {
            VectorShape preview = *original;
            if (vector_curve_shape(&preview, &t->draft)) {
                t->preview = preview;
            }
        }
    }
}

static bool pen_down(VectorDocument *d, PUIView *view, const SDL_Event *event) {
    VectorCurveTool *t = &d->curve_tool;
    if (event->button.clicks >= 2 && t->draft.count >= 2) {
        finish(d, view, false);
        return true;
    }
    VectorArtboard *board = vector_artboard_find(d, t->parent_id);
    if (!board) {
        for (int i = d->artboard_count - 1; i >= 0; i--) {
            VectorArtboard *b = &d->artboards[i];
            VectorPoint p = local(d, b, event->button.x, event->button.y, false);
            if (!(b->flags & VECTOR_OBJECT_HIDDEN) &&
                pui_contains((PUIRect){0, 0, b->rect.w, b->rect.h}, p.x, p.y)) {
                if (b->flags & VECTOR_OBJECT_LOCKED) {
                    return true;
                }
                board = b;
                t->parent_id = b->id;
                break;
            }
        }
    }
    if (!board || board->flags & VECTOR_OBJECT_STATE_MASK) {
        return true;
    }
    VectorPoint p = local(d, board, event->button.x, event->button.y, true);
    if (t->draft.count >= 2 && screen_distance(d, p, t->draft.nodes[0].point) <= 7) {
        finish(d, view, true);
        return true;
    }
    if (t->draft.count == VECTOR_BEZIER_NODES) {
        pui_view_set_text(view, "status", "Maximal 32 Knoten; Enter beendet die Kurve");
        return true;
    }
    t->node = (int)t->draft.count++;
    t->draft.nodes[t->node] = (VectorBezierNode){p, p, p};
    t->dragging = true;
    SDL_CaptureMouse(true);
    return true;
}

static bool insert_node(VectorDocument *d, VectorShape *shape, VectorPoint p) {
    VectorBezier curve = vector_curve_parent(shape);
    unsigned segments = curve.closed ? curve.count : curve.count - 1;
    float best = 6, parameter = 0;
    unsigned segment = 0;
    bool found = false;
    for (unsigned i = 0; i < segments; i++) {
        VectorBezierNode a = curve.nodes[i], b = curve.nodes[(i + 1) % curve.count];
        float sampled = INFINITY, candidate = .5f;
        for (int sample = 0; sample <= 32; sample++) {
            float t = sample / 32.f;
            float distance = screen_distance(d, p, vector_bezier_evaluate(a, b, t));
            if (distance < sampled) {
                sampled = distance;
                candidate = t;
            }
        }
        float lo = fmaxf(.0001f, candidate - 1 / 32.f), hi = fminf(.9999f, candidate + 1 / 32.f);
        for (int j = 0; j < 16; j++) {
            float left = (2 * lo + hi) / 3, right = (lo + 2 * hi) / 3;
            if (screen_distance(d, p, vector_bezier_evaluate(a, b, left)) <
                screen_distance(d, p, vector_bezier_evaluate(a, b, right))) {
                hi = right;
            } else {
                lo = left;
            }
        }
        candidate = (lo + hi) / 2;
        float distance = screen_distance(d, p, vector_bezier_evaluate(a, b, candidate));
        if (distance < best) {
            best = distance;
            parameter = candidate;
            segment = i;
            found = true;
        }
    }
    if (!found) {
        return false;
    }
    if (!vector_bezier_insert(&curve, segment, parameter) ||
        !vector_curve_update(d, shape->id, &curve)) {
        return false;
    }
    d->curve_tool.node = (int)segment + 1;
    d->curve_tool.shape_id = shape->id;
    return true;
}

static bool nodes_down(VectorDocument *d, const SDL_Event *event) {
    VectorCurveTool *t = &d->curve_tool;
    VectorShape *shape = vector_rectangle_find(d, d->active_rectangle);
    if (shape && shape->curve.count &&
        !(vector_rectangle_effective_flags(d, shape) & VECTOR_OBJECT_STATE_MASK)) {
        VectorArtboard *board = vector_artboard_find(d, shape->parent_id);
        VectorBezier curve = vector_curve_parent(shape);
        VectorPoint p = local(d, board, event->button.x, event->button.y, false);
        if (pick_control(d, &curve, p, &t->node, &t->handle)) {
            if (!t->handle && (SDL_GetModState() & SDL_KMOD_ALT)) {
                t->handle = 2; // Alt-drag a corner anchor to create its outgoing control handle.
            }
            t->draft = t->before = curve;
            t->parent_id = shape->parent_id;
            t->shape_id = shape->id;
            t->preview = *shape;
            t->editing = t->dragging = true;
            SDL_CaptureMouse(true);
            return true;
        }
        if (event->button.clicks >= 2 && insert_node(d, shape, p)) {
            return true;
        }
    }
    for (int i = d->rectangle_count - 1; i >= 0; i--) {
        shape = &d->rectangles[i];
        if (!shape->curve.count ||
            vector_rectangle_effective_flags(d, shape) & VECTOR_OBJECT_STATE_MASK) {
            continue;
        }
        VectorArtboard *board = vector_artboard_find(d, shape->parent_id);
        VectorPoint p = local(d, board, event->button.x, event->button.y, false);
        if (pui_contains((PUIRect){0, 0, board->rect.w, board->rect.h}, p.x, p.y) &&
            vector_shape_contains(shape, p.x, p.y)) {
            vector_document_select(d, shape->id);
            t->shape_id = shape->id;
            t->node = -1;
            return true;
        }
    }
    return true;
}

bool vector_curve_tool_event(VectorDocument *d, PUIView *view, const SDL_Event *event) {
    if (!d || (!d->curve_tool.pen && !d->curve_tool.nodes)) {
        return false;
    }
    VectorCurveTool *t = &d->curve_tool;
    if (t->nodes && !t->dragging && t->shape_id != d->active_rectangle) {
        t->shape_id = d->active_rectangle;
        t->node = -1;
    }
    if (event->type == SDL_EVENT_WINDOW_FOCUS_LOST ||
        (event->type == SDL_EVENT_KEY_DOWN && event->key.key == SDLK_ESCAPE)) {
        vector_curve_tool_cancel(d);
        return event->type != SDL_EVENT_WINDOW_FOCUS_LOST;
    }
    if (event->type == SDL_EVENT_KEY_DOWN && t->pen && event->key.key == SDLK_RETURN) {
        finish(d, view, false);
        return true;
    }
    if (event->type == SDL_EVENT_KEY_DOWN &&
        (event->key.key == SDLK_DELETE || event->key.key == SDLK_BACKSPACE)) {
        if (t->pen && t->draft.count && !t->dragging) {
            t->draft.count--;
            return true;
        }
        VectorShape *shape = vector_rectangle_find(d, t->shape_id);
        if (t->nodes && shape && t->node >= 0 && (unsigned)t->node < shape->curve.count &&
            shape->curve.count > 2 && !t->dragging) {
            VectorBezier curve = vector_curve_parent(shape);
            for (unsigned i = (unsigned)t->node; i + 1 < curve.count; i++) {
                curve.nodes[i] = curve.nodes[i + 1];
            }
            curve.count--;
            vector_curve_update(d, shape->id, &curve);
            t->node = -1;
        }
        return true;
    }
    VectorArtboard *board = vector_artboard_find(d, t->parent_id);
    VectorShape *edited = t->editing ? vector_rectangle_find(d, t->shape_id) : NULL;
    if (t->dragging && (!board || board->flags & VECTOR_OBJECT_STATE_MASK ||
                        (t->editing && (!edited || vector_rectangle_effective_flags(d, edited) &
                                                       VECTOR_OBJECT_STATE_MASK)))) {
        vector_curve_tool_cancel(d);
        return true;
    }
    if (t->dragging && event->type == SDL_EVENT_MOUSE_MOTION) {
        update(d, board, event->motion.x, event->motion.y);
        return true;
    }
    if (t->dragging && event->type == SDL_EVENT_MOUSE_BUTTON_UP &&
        event->button.button == SDL_BUTTON_LEFT) {
        update(d, board, event->button.x, event->button.y);
        if (t->editing && memcmp(&t->draft, &t->before, sizeof t->draft)) {
            vector_curve_update(d, t->shape_id, &t->draft);
        }
        t->dragging = t->editing = false;
        SDL_CaptureMouse(false);
        return true;
    }
    if (event->type == SDL_EVENT_MOUSE_BUTTON_DOWN &&
        on_canvas(d, view, event->button.x, event->button.y)) {
        if (event->button.button == SDL_BUTTON_RIGHT && t->pen) {
            finish(d, view, false);
            return true;
        }
        if (event->button.button == SDL_BUTTON_LEFT) {
            return t->pen ? pen_down(d, view, event) : nodes_down(d, event);
        }
    }
    if (event->type == SDL_EVENT_MOUSE_BUTTON_UP && event->button.button == SDL_BUTTON_LEFT) {
        return true;
    }
    return t->dragging && event->type == SDL_EVENT_MOUSE_WHEEL;
}
