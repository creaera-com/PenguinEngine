/** @file @brief A preview selection is view state, never a document edit or undo command. */
#include "tool.h"
#include "editors/vector/artboard/tool.h"
#include <math.h>
#include <stdio.h>
#include <string.h>

void vector_preview_cancel(VectorDocument *d) {
    if (d->preview.dragging) {
        SDL_CaptureMouse(false);
    }
    d->preview.armed = d->preview.dragging = false;
    d->viewport.panning = false;
}

static const VectorArtboard *board(const VectorDocument *d, unsigned id) {
    for (int i = 0; i < d->artboard_count; i++) {
        if (d->artboards[i].id == id && !(d->artboards[i].flags & VECTOR_OBJECT_HIDDEN)) {
            return &d->artboards[i];
        }
    }
    return NULL;
}

const VectorArtboard *vector_preview_region(const VectorDocument *d, PUIRect *r) {
    if (!d) {
        return NULL;
    }
    const VectorArtboard *b = board(d, d->preview.board);
    if (!b) {
        return NULL;
    }
    *r = d->preview.region;
    r->x = fminf(b->rect.w, fmaxf(0, r->x));
    r->y = fminf(b->rect.h, fmaxf(0, r->y));
    r->w = floorf(fminf(r->w, b->rect.w - r->x));
    r->h = floorf(fminf(r->h, b->rect.h - r->y));
    return r->w >= 1 && r->h >= 1 ? b : NULL;
}

bool vector_preview_action(VectorDocument *d, PUIView *view, const char *action) {
    bool select = !strcmp(action, "vector.preview.select");
    if (!select && strcmp(action, "vector.preview.clear")) {
        return false;
    }
    if (d) {
        bool enable = select && !d->preview.armed;
        vector_artboard_tool_cancel(d);
        d->preview.armed = enable;
        if (!select) {
            d->preview = (VectorPreview){0};
        }
        pui_view_set_text(view, "status",
                          enable ? "Vorschaubereich im Artboard aufziehen; Esc: abbrechen"
                                 : "Vorschauauswahl beendet");
    }
    vector_preview_sync(view, d);
    return true;
}

static void endpoint(VectorDocument *d, const VectorArtboard *b, float x, float y) {
    const VectorViewport *v = &d->viewport;
    x = roundf((x - v->bounds.x - v->x) / v->zoom - b->rect.x);
    y = roundf((y - v->bounds.y - v->y) / v->zoom - b->rect.y);
    x = fminf(b->rect.w, fmaxf(0, x));
    y = fminf(b->rect.h, fmaxf(0, y));
    d->preview.draft = (PUIRect){fminf(x, d->preview.start_x), fminf(y, d->preview.start_y),
                                 fabsf(x - d->preview.start_x), fabsf(y - d->preview.start_y)};
}

bool vector_preview_event(VectorDocument *d, PUIView *view, const SDL_Event *event) {
    if (!d || !d->preview.armed) {
        return false;
    }
    VectorPreview *p = &d->preview;
    if (event->type == SDL_EVENT_WINDOW_FOCUS_LOST ||
        (event->type == SDL_EVENT_KEY_DOWN && event->key.key == SDLK_ESCAPE) ||
        (event->type == SDL_EVENT_MOUSE_BUTTON_DOWN && event->button.button == SDL_BUTTON_RIGHT)) {
        vector_preview_cancel(d);
        return true;
    }
    const VectorArtboard *b = board(d, p->draft_board);
    if (p->dragging) {
        if (!b) {
            vector_preview_cancel(d);
            return true;
        }
        if (event->type == SDL_EVENT_MOUSE_MOTION) {
            endpoint(d, b, event->motion.x, event->motion.y);
            return true;
        }
        if (event->type == SDL_EVENT_MOUSE_BUTTON_UP && event->button.button == SDL_BUTTON_LEFT) {
            endpoint(d, b, event->button.x, event->button.y);
            if (p->draft.w >= 1 && p->draft.h >= 1) {
                p->board = p->draft_board;
                p->region = p->draft;
            }
            vector_preview_cancel(d);
            return true;
        }
    }
    if (event->type == SDL_EVENT_MOUSE_BUTTON_DOWN && event->button.button == SDL_BUTTON_LEFT) {
        char id[96];
        snprintf(id, sizeof id, "vector_document_%u.canvas", d->id);
        if (!pui_view_hit(view, id, event->button.x, event->button.y)) {
            return false;
        }
        const VectorViewport *v = &d->viewport;
        float x = (event->button.x - v->bounds.x - v->x) / v->zoom;
        float y = (event->button.y - v->bounds.y - v->y) / v->zoom;
        for (int i = d->artboard_count - 1; i >= 0; i--) {
            b = &d->artboards[i];
            if (!(b->flags & VECTOR_OBJECT_HIDDEN) && pui_contains(b->rect, x, y)) {
                p->draft_board = b->id;
                p->start_x = fminf(b->rect.w, fmaxf(0, roundf(x - b->rect.x)));
                p->start_y = fminf(b->rect.h, fmaxf(0, roundf(y - b->rect.y)));
                p->dragging = true;
                endpoint(d, b, event->button.x, event->button.y);
                SDL_CaptureMouse(true);
                break;
            }
        }
        return true;
    }
    return false;
}

void vector_preview_sync(PUIView *view, const VectorDocument *d) {
    pui_view_set_checked(view, "vector_preview_select", d && d->preview.armed);
    pui_view_set_checked(view, "vector_preview_toolbar", d && d->preview.armed);
}
