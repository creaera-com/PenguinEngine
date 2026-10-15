#include "penguin/engine.h"
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
static bool cell(const PeScene *s, int x, int y) {
    return x >= 0 && y >= 0 && x < s->width && y < s->height;
}
void pe_scene_init(PeScene *s, int w, int h, int tile) {
    memset(s, 0, sizeof *s);
    s->width = w > 0 && w <= PE_MAP_MAX ? w : 16;
    s->height = h > 0 && h <= PE_MAP_MAX ? h : 16;
    s->tile_size = tile > 0 && tile <= 128 ? tile : 16;
    s->tileset_tile_size = s->tile_size;
    s->layer_count = 1;
    s->next_id = 1;
    strcpy(s->layers[0].name, "Ground");
    s->layers[0].visible = true;
    for (int i = 0; i < PE_ENTITIES; i++)
        s->objects[i].generation = 1;
}
PeEntity pe_entity_create(PeScene *s, uint32_t mask) {
    for (uint32_t i = 0; i < PE_ENTITIES; i++)
        if (!s->objects[i].alive) {
            PeObject *o = &s->objects[i];
            uint32_t generation = o->generation;
            memset(o, 0, sizeof *o);
            o->generation = generation;
            o->alive = true;
            o->stable_id = s->next_id++;
            o->mask = mask;
            o->w = 12;
            o->h = 12;
            o->color = 0xffcc55ff;
            return (PeEntity){i, generation};
        }
    return (PeEntity){UINT32_MAX, 0};
}
bool pe_entity_valid(const PeScene *s, PeEntity e) {
    return e.index < PE_ENTITIES && s->objects[e.index].alive &&
           s->objects[e.index].generation == e.generation;
}
PeObject *pe_entity_get(PeScene *s, PeEntity e) {
    return pe_entity_valid(s, e) ? &s->objects[e.index] : NULL;
}
bool pe_entity_destroy(PeScene *s, PeEntity e) {
    if (!pe_entity_valid(s, e))
        return false;
    PeObject *o = &s->objects[e.index];
    o->alive = false;
    o->generation++;
    if (!o->generation)
        o->generation = 1;
    return true;
}
PeEntity pe_entity_find(const PeScene *s, uint32_t id) {
    for (uint32_t i = 0; i < PE_ENTITIES; i++)
        if (s->objects[i].alive && s->objects[i].stable_id == id)
            return (PeEntity){i, s->objects[i].generation};
    return (PeEntity){UINT32_MAX, 0};
}
bool pe_tile_set(PeScene *s, int l, int x, int y, uint16_t tile) {
    if (l < 0 || l >= s->layer_count || !cell(s, x, y))
        return false;
    s->layers[l].tiles[y * PE_MAP_MAX + x] = tile;
    return true;
}
uint16_t pe_tile_get(const PeScene *s, int l, int x, int y) {
    return l >= 0 && l < s->layer_count && cell(s, x, y) ? s->layers[l].tiles[y * PE_MAP_MAX + x]
                                                         : 0;
}
bool pe_fill(PeScene *s, int l, int x, int y, uint16_t tile) {
    if (l < 0 || l >= s->layer_count || !cell(s, x, y))
        return false;
    uint16_t old = pe_tile_get(s, l, x, y);
    if (old == tile)
        return true;
    int queue[PE_MAP_MAX * PE_MAP_MAX], head = 0, tail = 0;
    queue[tail++] = y * PE_MAP_MAX + x;
    pe_tile_set(s, l, x, y, tile);
    while (head < tail) {
        int p = queue[head++], px = p % PE_MAP_MAX, py = p / PE_MAP_MAX;
        int nx[4] = {px - 1, px + 1, px, px}, ny[4] = {py, py, py - 1, py + 1};
        for (int i = 0; i < 4; i++)
            if (cell(s, nx[i], ny[i]) && pe_tile_get(s, l, nx[i], ny[i]) == old) {
                pe_tile_set(s, l, nx[i], ny[i], tile);
                queue[tail++] = ny[i] * PE_MAP_MAX + nx[i];
            }
    }
    return true;
}
void pe_rectangle(PeScene *s, int l, int x0, int y0, int x1, int y1, uint16_t tile) {
    if (x0 > x1) {
        int t = x0;
        x0 = x1;
        x1 = t;
    }
    if (y0 > y1) {
        int t = y0;
        y0 = y1;
        y1 = t;
    }
    if (x0 < 0)
        x0 = 0;
    if (y0 < 0)
        y0 = 0;
    if (x1 >= s->width)
        x1 = s->width - 1;
    if (y1 >= s->height)
        y1 = s->height - 1;
    for (int y = y0; y <= y1; y++)
        for (int x = x0; x <= x1; x++)
            pe_tile_set(s, l, x, y, tile);
}
void pe_history_init(PeHistory *h) {
    memset(h, 0, sizeof *h);
    h->next_revision = 1;
}
static void command_free(PeCommand *c) {
    free(c->before);
    free(c->after);
    memset(c, 0, sizeof *c);
}
void pe_history_free(PeHistory *h) {
    for (int i = 0; i < h->count; i++)
        command_free(&h->commands[i]);
    free(h->pending);
    pe_history_init(h);
}
bool pe_history_begin(PeHistory *h, const PeScene *s) {
    if (h->pending)
        return false;
    h->pending = malloc(sizeof *s);
    if (!h->pending)
        return false;
    *h->pending = *s;
    return true;
}
void pe_history_cancel(PeHistory *h, PeScene *s) {
    if (h->pending) {
        *s = *h->pending;
        free(h->pending);
        h->pending = NULL;
    }
}
bool pe_history_commit(PeHistory *h, PeScene *s) {
    if (!h->pending)
        return false;
    if (!memcmp(h->pending, s, sizeof *s)) {
        free(h->pending);
        h->pending = NULL;
        return true;
    }
    PeScene *after = malloc(sizeof *after);
    if (!after) {
        pe_history_cancel(h, s);
        return false;
    }
    *after = *s;
    for (int i = h->cursor; i < h->count; i++)
        command_free(&h->commands[i]);
    h->count = h->cursor;
    if (h->count == PE_HISTORY) {
        command_free(&h->commands[0]);
        memmove(h->commands, h->commands + 1, sizeof(PeCommand) * (PE_HISTORY - 1));
        h->count--;
        h->cursor--;
    }
    PeCommand *c = &h->commands[h->count++];
    *c = (PeCommand){h->pending, after, h->revision, h->next_revision++};
    h->pending = NULL;
    h->revision = c->after_revision;
    h->cursor = h->count;
    return true;
}
bool pe_undo(PeHistory *h, PeScene *s) {
    if (h->pending || h->cursor == 0)
        return false;
    PeCommand *c = &h->commands[--h->cursor];
    *s = *c->before;
    h->revision = c->before_revision;
    return true;
}
bool pe_redo(PeHistory *h, PeScene *s) {
    if (h->pending || h->cursor == h->count)
        return false;
    PeCommand *c = &h->commands[h->cursor++];
    *s = *c->after;
    h->revision = c->after_revision;
    return true;
}
void pe_history_saved(PeHistory *h) {
    h->saved_revision = h->revision;
}
bool pe_history_dirty(const PeHistory *h) {
    return h->revision != h->saved_revision || h->pending != NULL;
}
void pe_screen_to_world(const PeCamera *c, float sx, float sy, float *wx, float *wy) {
    *wx = c->x + sx / c->zoom;
    *wy = c->y + sy / c->zoom;
}
void pe_camera_zoom(PeCamera *c, float sx, float sy, float factor) {
    float wx, wy;
    pe_screen_to_world(c, sx, sy, &wx, &wy);
    c->zoom = fmaxf(.25f, fminf(8, c->zoom * factor));
    c->x = wx - sx / c->zoom;
    c->y = wy - sy / c->zoom;
}
bool pe_collides(const PeScene *s, float x, float y, float w, float h) {
    if (w <= 0 || h <= 0 || !isfinite(w) || !isfinite(h) || !isfinite(x) || !isfinite(y))
        return true;
    if (x < 0 || y < 0 || x + w > s->width * s->tile_size || y + h > s->height * s->tile_size)
        return true;
    int x0 = (int)floorf(x / s->tile_size), y0 = (int)floorf(y / s->tile_size);
    int x1 = (int)ceilf((x + w) / s->tile_size) - 1, y1 = (int)ceilf((y + h) / s->tile_size) - 1;
    for (int py = y0; py <= y1; py++)
        for (int px = x0; px <= x1; px++)
            if (s->solid[py * PE_MAP_MAX + px])
                return true;
    return false;
}
void pe_move(PeScene *s, PeObject *o, float dx, float dy) {
    /* Substeps prevent tunneling through tiles. Axis separation permits wall sliding. */
    if (!isfinite(dx) || !isfinite(dy))
        return;
    float requested = ceilf(fmaxf(fabsf(dx), fabsf(dy)) / fmaxf(1, s->tile_size * .25f));
    if (requested > 4096)
        return;
    int steps = (int)requested;
    if (steps < 1)
        steps = 1;
    for (int i = 0; i < steps; i++) {
        if (!pe_collides(s, o->x + dx / steps, o->y, o->w, o->h))
            o->x += dx / steps;
        if (!pe_collides(s, o->x, o->y + dy / steps, o->w, o->h))
            o->y += dy / steps;
    }
}
void pe_update(PeScene *s, float ix, float iy, float dt) {
    if (!isfinite(ix) || !isfinite(iy) || !isfinite(dt) || dt <= 0 || dt > .1f)
        return;
    ix = fmaxf(-1, fminf(1, ix));
    iy = fmaxf(-1, fminf(1, iy));
    float length = sqrtf(ix * ix + iy * iy);
    if (length > 1) {
        ix /= length;
        iy /= length;
    }
    for (int i = 0; i < PE_ENTITIES; i++) {
        PeObject *o = &s->objects[i];
        if (!o->alive || (o->mask & (PE_TRANSFORM | PE_BODY)) != (PE_TRANSFORM | PE_BODY))
            continue;
        if (o->mask & PE_PLAYER) {
            o->vx = ix * 90;
            o->vy = iy * 90;
        }
        pe_move(s, o, o->vx * dt, o->vy * dt);
    }
}
