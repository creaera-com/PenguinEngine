/** @file @brief On-canvas fill handles commit one value command when the gesture ends. */
#include "tool.h"
#include "editors/vector/artboard/tool.h"
#include "editors/vector/document/selection.h"
#include "editors/vector/geometry/rotation.h"
#include "editors/vector/mask/mask.h"
#include "editors/vector/object/commands.h"
#include <math.h>
#include <string.h>

bool vector_gradient_eligible(const VectorShape *s) {
    return s && !*s->image.source && !s->text.enabled && !s->clip_mask &&
           (!s->curve.count || s->curve.closed);
}

void vector_gradient_tool_cancel(VectorDocument *d) {
    if (d->gradient_tool.dragging) {
        SDL_CaptureMouse(false);
    }
    d->gradient_tool = (VectorGradientTool){0};
}

const VectorGradient *vector_gradient_display(const VectorDocument *d, const VectorShape *s) {
    return d->gradient_tool.dragging && d->gradient_tool.shape_id == s->id ? &d->gradient_tool.draft
                                                                           : &s->gradient;
}

bool vector_gradient_tool_action(VectorDocument *d, PUIView *view, const char *action) {
    if (strcmp(action, "vector.tool_gradient")) {
        return false;
    }
    if (d) {
        bool enable = !d->gradient_tool.enabled;
        vector_artboard_tool_cancel(d);
        d->gradient_tool.enabled = enable;
        pui_view_set_text(view, "status",
                          enable ? "Verlauf: Objekt anklicken, Strecke aufziehen oder Endpunkte "
                                   "ziehen; Esc: abbrechen"
                                 : "Auswahlwerkzeug aktiv");
    }
    return true;
}

static VectorPoint screen(const VectorDocument *d, const VectorShape *s, VectorPoint p) {
    const VectorArtboard *b = NULL;
    for (int i = 0; i < d->artboard_count; i++) {
        if (d->artboards[i].id == s->parent_id) {
            b = &d->artboards[i];
        }
    }
    float x = s->rect.x + p.x * s->rect.w, y = s->rect.y + p.y * s->rect.h;
    vector_rotate_point(&x, &y, s->rect.x + s->rect.w / 2, s->rect.y + s->rect.h / 2, s->angle);
    return (VectorPoint){
        d->viewport.bounds.x + d->viewport.x + (x + (b ? b->rect.x : 0)) * d->viewport.zoom,
        d->viewport.bounds.y + d->viewport.y + (y + (b ? b->rect.y : 0)) * d->viewport.zoom};
}

static VectorPoint local(VectorDocument *d, const VectorShape *s, float x, float y) {
    VectorArtboard *b = vector_artboard_find(d, s->parent_id);
    x = (x - d->viewport.bounds.x - d->viewport.x) / d->viewport.zoom - (b ? b->rect.x : 0);
    y = (y - d->viewport.bounds.y - d->viewport.y) / d->viewport.zoom - (b ? b->rect.y : 0);
    vector_rotate_point(&x, &y, s->rect.x + s->rect.w / 2, s->rect.y + s->rect.h / 2, -s->angle);
    return (VectorPoint){fmaxf(-16, fminf(17, (x - s->rect.x) / s->rect.w)),
                         fmaxf(-16, fminf(17, (y - s->rect.y) / s->rect.h))};
}

static void endpoint(VectorDocument *d, const VectorShape *s, float x, float y) {
    VectorPoint p = local(d, s, x, y);
    VectorGradient *g = &d->gradient_tool.draft;
    if (d->gradient_tool.handle == 0) {
        g->x1 = p.x;
        g->y1 = p.y;
    } else if (d->gradient_tool.handle == 1) {
        g->x2 = p.x;
        g->y2 = p.y;
    } else {
        unsigned stop = d->gradient_tool.handle - 2;
        if (!stop || stop + 1 >= g->count) {
            return;
        }
        float dx = (g->x2 - g->x1) * s->rect.w, dy = (g->y2 - g->y1) * s->rect.h;
        float length = dx * dx + dy * dy;
        if (length <= 0) {
            return;
        }
        float t = ((p.x - g->x1) * s->rect.w * dx + (p.y - g->y1) * s->rect.h * dy) / length;
        float lo = nextafterf(g->stops[stop - 1].position, g->stops[stop + 1].position);
        float hi = nextafterf(g->stops[stop + 1].position, g->stops[stop - 1].position);
        if (lo <= hi) {
            g->stops[stop].position = fmaxf(lo, fminf(hi, t));
        }
    }
}

bool vector_gradient_tool_event(VectorDocument *d, PUIView *view, const SDL_Event *e) {
    if (!d || !d->gradient_tool.enabled) {
        return false;
    }
    VectorGradientTool *t = &d->gradient_tool;
    VectorShape *s = vector_rectangle_find(d, d->active_rectangle);
    if (e->type == SDL_EVENT_WINDOW_FOCUS_LOST ||
        (e->type == SDL_EVENT_KEY_DOWN && e->key.key == SDLK_ESCAPE) ||
        (e->type == SDL_EVENT_MOUSE_BUTTON_DOWN && e->button.button == SDL_BUTTON_RIGHT)) {
        vector_gradient_tool_cancel(d);
        return true;
    }
    if (t->dragging) {
        s = vector_rectangle_find(d, t->shape_id);
        if (!s || s->id != d->active_rectangle ||
            vector_rectangle_effective_flags(d, s) & VECTOR_OBJECT_STATE_MASK) {
            vector_gradient_tool_cancel(d);
            return true;
        }
        if (e->type == SDL_EVENT_MOUSE_MOTION) {
            endpoint(d, s, e->motion.x, e->motion.y);
            return true;
        }
        if (e->type == SDL_EVENT_MOUSE_BUTTON_UP && e->button.button == SDL_BUTTON_LEFT) {
            endpoint(d, s, e->button.x, e->button.y);
            VectorShape updated = *s;
            updated.gradient = t->draft;
            t->dragging = false;
            SDL_CaptureMouse(false);
            bool ok = vector_shape_update(d, &updated);
            pui_view_set_text(view, "status", ok ? "Verlauf aktualisiert" : "Verlauf unveraendert");
            return true;
        }
    }
    if (e->type == SDL_EVENT_MOUSE_BUTTON_DOWN && e->button.button == SDL_BUTTON_LEFT) {
        char canvas[96];
        snprintf(canvas, sizeof canvas, "vector_document_%u.canvas", d->id);
        if (!pui_view_hit(view, canvas, e->button.x, e->button.y)) {
            return false;
        }
        float x = e->button.x, y = e->button.y;
        bool editable = s && d->selection.count <= 1 && vector_gradient_eligible(s) &&
                        !(vector_rectangle_effective_flags(d, s) & VECTOR_OBJECT_STATE_MASK);
        int handle = -1;
        if (editable && s->gradient.kind) {
            VectorPoint a = screen(d, s, (VectorPoint){s->gradient.x1, s->gradient.y1});
            VectorPoint b = screen(d, s, (VectorPoint){s->gradient.x2, s->gradient.y2});
            float best = 10;
            float da = hypotf(x - a.x, y - a.y), db = hypotf(x - b.x, y - b.y);
            if (da <= 9) {
                best = da;
                handle = 0;
            }
            if (db <= 9 && db < best) {
                best = db;
                handle = 1;
            }
            for (unsigned i = 1; i + 1 < s->gradient.count; i++) {
                float t = s->gradient.stops[i].position;
                float distance = hypotf(x - a.x - (b.x - a.x) * t, y - a.y - (b.y - a.y) * t);
                if (distance <= 7 && distance < best) {
                    best = distance;
                    handle = (int)i + 2;
                }
            }
        }
        if (handle < 0) {
            VectorShape *hit = NULL;
            for (int i = d->rectangle_count - 1; i >= 0; i--) {
                VectorShape *candidate = &d->rectangles[i];
                VectorArtboard *board = vector_artboard_find(d, candidate->parent_id);
                float px = (x - d->viewport.bounds.x - d->viewport.x) / d->viewport.zoom -
                           (board ? board->rect.x : 0);
                float py = (y - d->viewport.bounds.y - d->viewport.y) / d->viewport.zoom -
                           (board ? board->rect.y : 0);
                if (vector_gradient_eligible(candidate) &&
                    !(vector_rectangle_effective_flags(d, candidate) & VECTOR_OBJECT_STATE_MASK) &&
                    vector_mask_contains(d, candidate, px, py)) {
                    hit = candidate;
                    break;
                }
            }
            if (!hit) {
                return true;
            }
            if (!s || hit->id != s->id || d->selection.count > 1) {
                vector_document_select(d, hit->id);
                return true;
            }
            if (!editable) {
                return true;
            }
        }
        t->shape_id = s->id;
        t->draft = s->gradient.count ? s->gradient : vector_gradient_default(s->fill);
        if (!t->draft.kind) {
            t->draft.kind = VECTOR_GRADIENT_LINEAR;
        }
        t->handle = handle < 0 ? 1u : (unsigned)handle;
        if (handle < 0) {
            VectorPoint p = local(d, s, x, y);
            t->draft.x1 = t->draft.x2 = p.x;
            t->draft.y1 = t->draft.y2 = p.y;
        }
        t->dragging = true;
        SDL_CaptureMouse(true);
        return true;
    }
    return false;
}

void vector_gradient_tool_draw(VectorDocument *d, SDL_Renderer *r) {
    VectorShape *s = vector_rectangle_find(d, d->active_rectangle);
    if (!d->gradient_tool.enabled || d->selection.count > 1 || !vector_gradient_eligible(s) ||
        vector_rectangle_effective_flags(d, s) & VECTOR_OBJECT_STATE_MASK) {
        return;
    }
    const VectorGradient *g = vector_gradient_display(d, s);
    if (!g->kind) {
        return;
    }
    VectorPoint a = screen(d, s, (VectorPoint){g->x1, g->y1}),
                b = screen(d, s, (VectorPoint){g->x2, g->y2});
    SDL_BlendMode blend;
    Uint8 red, green, blue, alpha;
    SDL_GetRenderDrawBlendMode(r, &blend);
    SDL_GetRenderDrawColor(r, &red, &green, &blue, &alpha);
    SDL_SetRenderDrawBlendMode(r, SDL_BLENDMODE_BLEND);
    SDL_SetRenderDrawColor(r, 25, 25, 25, 255);
    SDL_RenderLine(r, a.x, a.y, b.x, b.y);
    for (unsigned i = 0; i < g->count; i++) {
        float t = g->stops[i].position;
        SDL_FRect grip = {a.x + (b.x - a.x) * t - 4, a.y + (b.y - a.y) * t - 4, 8, 8};
        uint32_t c = g->stops[i].color;
        SDL_SetRenderDrawColor(r, c >> 24, c >> 16, c >> 8, 255);
        SDL_RenderFillRect(r, &grip);
        SDL_SetRenderDrawColor(r, 20, 20, 20, 255);
        SDL_RenderRect(r, &grip);
    }
    SDL_SetRenderDrawColor(r, red, green, blue, alpha);
    SDL_SetRenderDrawBlendMode(r, blend);
}
