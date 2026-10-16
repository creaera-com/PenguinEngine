#include "editors/vector/document.h"
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static bool bounds(float x, float y, float w, float h) {
    return isfinite(x) && isfinite(y) && isfinite(w) && isfinite(h) && fabsf(x) <= 1000000 &&
           fabsf(y) <= 1000000 && w > 0 && h > 0 && w <= 100000 && h <= 100000;
}

int ve_add_board(VeDocument *d, float x, float y, float w, float h) {
    if (!bounds(x, y, w, h)) {
        return -1;
    }
    for (int i = 0; i < VE_BOARDS; i++) {
        if (!d->boards[i].used) {
            d->boards[i] =
                (VeBoard){.used = true, .x = x, .y = y, .w = w, .h = h, .background = 0xf4f1eaff};
            snprintf(d->boards[i].name, 64, "Artboard %d", i + 1);
            return i;
        }
    }
    return -1;
}

void ve_init(VeDocument *d) {
    memset(d, 0, sizeof *d);
    d->next_id = 1;
    ve_add_board(d, 0, 0, 512, 384);
}

int ve_add_shape(VeDocument *d, int board, VeKind kind, float x, float y, float w, float h) {
    if (board < 0 || board >= VE_BOARDS || !d->boards[board].used || kind < VE_RECT ||
        kind > VE_PATH || !bounds(x, y, w, h)) {
        return -1;
    }
    for (int i = 0; i < VE_SHAPES; i++) {
        if (!d->shapes[i].used) {
            VeShape *s = &d->shapes[i];
            memset(s, 0, sizeof *s);
            s->used = s->fill_enabled = s->evenodd = true;
            s->board = board;
            s->kind = kind;
            s->x = x;
            s->y = y;
            s->w = w;
            s->h = h;
            if (kind == VE_PATH) {
                s->count = 1;
                s->nodes[0] = (VeNode){.command = VE_MOVE, .p = {x, y}};
                s->w = s->h = 0;
            }
            s->id = d->next_id++;
            s->fill = 0xc28e5cff;
            s->stroke = 0x30343bff;
            s->stroke_width = 2;
            snprintf(s->name, 64, "%s %u",
                     kind == VE_RECT      ? "Rectangle"
                     : kind == VE_ELLIPSE ? "Ellipse"
                                          : "Path",
                     s->id);
            return i;
        }
    }
    return -1;
}

bool ve_valid(const VeDocument *d) {
    bool board = false;
    for (int i = 0; i < VE_BOARDS; i++) {
        const VeBoard *b = &d->boards[i];
        if (b->used) {
            board = true;
            if (!memchr(b->name, 0, 64) || !bounds(b->x, b->y, b->w, b->h)) {
                return false;
            }
        }
    }
    if (!board || !d->next_id) {
        return false;
    }
    for (int i = 0; i < VE_SHAPES; i++) {
        const VeShape *s = &d->shapes[i];
        if (!s->used) {
            continue;
        }
        if (!s->id || s->id >= d->next_id || s->board < 0 || s->board >= VE_BOARDS ||
            !d->boards[s->board].used || s->kind < 0 || s->kind > VE_PATH || s->count < 0 ||
            s->count > VE_NODES || !memchr(s->name, 0, 64) || !isfinite(s->stroke_width) ||
            s->stroke_width < 0 || s->stroke_width > 4096 || !isfinite(s->x) || !isfinite(s->y) ||
            !isfinite(s->w) || !isfinite(s->h) || fabsf(s->x) > 1000000 || fabsf(s->y) > 1000000 ||
            s->w < 0 || s->h < 0 || s->w > 100000 || s->h > 100000) {
            return false;
        }
        if (s->kind != VE_PATH && (s->w <= 0 || s->h <= 0)) {
            return false;
        }
        if (s->kind == VE_PATH && (s->count < 1 || s->nodes[0].command != VE_MOVE)) {
            return false;
        }
        for (int k = 0; k < i; k++) {
            if (d->shapes[k].used && d->shapes[k].id == s->id) {
                return false;
            }
        }
        for (int j = 0; j < s->count; j++) {
            const VeNode *n = &s->nodes[j];
            const float values[] = {n->p.x, n->p.y, n->c1.x, n->c1.y, n->c2.x, n->c2.y};
            if (n->command < VE_MOVE || n->command > VE_CLOSE) {
                return false;
            }
            for (int k = 0; k < 6; k++) {
                if (!isfinite(values[k]) || fabsf(values[k]) > 1000000) {
                    return false;
                }
            }
        }
    }
    return true;
}

bool ve_to_path(VeShape *s) {
    if (!s->used || s->kind == VE_PATH) {
        return false;
    }
    float x = s->x, y = s->y, w = s->w, h = s->h;
    if (s->kind == VE_RECT) {
        s->count = 5;
        s->nodes[0] = (VeNode){.command = VE_MOVE, .p = {x, y}};
        s->nodes[1] = (VeNode){.command = VE_LINE, .p = {x + w, y}};
        s->nodes[2] = (VeNode){.command = VE_LINE, .p = {x + w, y + h}};
        s->nodes[3] = (VeNode){.command = VE_LINE, .p = {x, y + h}};
        s->nodes[4] = (VeNode){.command = VE_CLOSE};
    } else {
        float cx = x + w * .5f, cy = y + h * .5f, rx = w * .5f, ry = h * .5f, k = .552284749831f;
        s->count = 6;
        s->nodes[0] = (VeNode){.command = VE_MOVE, .p = {cx + rx, cy}};
        s->nodes[1] = (VeNode){.command = VE_CUBIC,
                               .c1 = {cx + rx, cy + k * ry},
                               .c2 = {cx + k * rx, cy + ry},
                               .p = {cx, cy + ry}};
        s->nodes[2] = (VeNode){.command = VE_CUBIC,
                               .c1 = {cx - k * rx, cy + ry},
                               .c2 = {cx - rx, cy + k * ry},
                               .p = {cx - rx, cy}};
        s->nodes[3] = (VeNode){.command = VE_CUBIC,
                               .c1 = {cx - rx, cy - k * ry},
                               .c2 = {cx - k * rx, cy - ry},
                               .p = {cx, cy - ry}};
        s->nodes[4] = (VeNode){.command = VE_CUBIC,
                               .c1 = {cx + k * rx, cy - ry},
                               .c2 = {cx + rx, cy - k * ry},
                               .p = {cx + rx, cy}};
        s->nodes[5] = (VeNode){.command = VE_CLOSE};
    }
    s->kind = VE_PATH;
    return true;
}

void ve_bounds(VeShape *s) {
    if (s->kind != VE_PATH) {
        return;
    }
    float minx = INFINITY, miny = INFINITY, maxx = -INFINITY, maxy = -INFINITY;
    for (int i = 0; i < s->count; i++) {
        VeNode *n = &s->nodes[i];
        if (n->command == VE_CLOSE) {
            continue;
        }
        VePoint p[3] = {n->p, n->c1, n->c2};
        int count = n->command == VE_CUBIC ? 3 : 1;
        for (int k = 0; k < count; k++) {
            minx = fminf(minx, p[k].x);
            miny = fminf(miny, p[k].y);
            maxx = fmaxf(maxx, p[k].x);
            maxy = fmaxf(maxy, p[k].y);
        }
    }
    if (isfinite(minx)) {
        s->x = minx;
        s->y = miny;
        s->w = maxx - minx;
        s->h = maxy - miny;
    }
}

void ve_move(VeShape *s, float dx, float dy) {
    s->x += dx;
    s->y += dy;
    for (int i = 0; i < s->count; i++) {
        if (s->nodes[i].command == VE_CLOSE) {
            continue;
        }
        s->nodes[i].p.x += dx;
        s->nodes[i].p.y += dy;
        if (s->nodes[i].command == VE_CUBIC) {
            s->nodes[i].c1.x += dx;
            s->nodes[i].c1.y += dy;
            s->nodes[i].c2.x += dx;
            s->nodes[i].c2.y += dy;
        }
    }
}

void ve_mirror(VeShape *s, bool horizontal, float axis) {
    if (s->kind != VE_PATH) {
        if (horizontal) {
            s->x = 2 * axis - s->x - s->w;
        } else {
            s->y = 2 * axis - s->y - s->h;
        }
        return;
    }
    for (int i = 0; i < s->count; i++) {
        VeNode *n = &s->nodes[i];
        if (n->command == VE_CLOSE) {
            continue;
        }
        VePoint *p[3] = {&n->p, &n->c1, &n->c2};
        for (int k = 0; k < (n->command == VE_CUBIC ? 3 : 1); k++) {
            if (horizontal) {
                p[k]->x = 2 * axis - p[k]->x;
            } else {
                p[k]->y = 2 * axis - p[k]->y;
            }
        }
    }
    ve_bounds(s);
}

static VePoint mid(VePoint a, VePoint b) {
    return (VePoint){(a.x + b.x) * .5f, (a.y + b.y) * .5f};
}

bool ve_split_node(VeShape *s, int index) {
    if (s->kind != VE_PATH || s->count >= VE_NODES || index < 1 || index >= s->count) {
        return false;
    }
    VeNode old = s->nodes[index];
    if (old.command != VE_LINE && old.command != VE_CUBIC) {
        return false;
    }
    VeNode first = old;
    VePoint p = s->nodes[index - 1].p;
    if (old.command == VE_LINE) {
        first.p = mid(p, old.p);
    } else {
        VePoint a = mid(p, old.c1), b = mid(old.c1, old.c2), c = mid(old.c2, old.p), d = mid(a, b),
                e = mid(b, c), f = mid(d, e);
        first.c1 = a;
        first.c2 = d;
        first.p = f;
        old.c1 = e;
        old.c2 = c;
    }
    memmove(&s->nodes[index + 1], &s->nodes[index], (size_t)(s->count - index) * sizeof(VeNode));
    s->nodes[index] = first;
    s->nodes[index + 1] = old;
    s->count++;
    ve_bounds(s);
    return true;
}

static bool point(VeFlat *f, VePoint p) {
    if (f->count >= VE_FLAT) {
        return false;
    }
    f->points[f->count++] = p;
    f->length[f->contours - 1]++;
    return true;
}

static bool cubic(VeFlat *f, VePoint a, VePoint b, VePoint c, VePoint d, int depth) {
    float dx = d.x - a.x, dy = d.y - a.y, l = hypotf(dx, dy);
    float distance = l > 1e-7f ? fmaxf(fabsf((b.x - a.x) * dy - (b.y - a.y) * dx),
                                       fabsf((c.x - a.x) * dy - (c.y - a.y) * dx)) /
                                     l
                               : fmaxf(hypotf(b.x - a.x, b.y - a.y), hypotf(c.x - a.x, c.y - a.y));
    if (distance <= .25f || depth >= 14) {
        return point(f, d);
    }
    VePoint ab = mid(a, b), bc = mid(b, c), cd = mid(c, d), abc = mid(ab, bc), bcd = mid(bc, cd),
            m = mid(abc, bcd);
    return cubic(f, a, ab, abc, m, depth + 1) && cubic(f, m, bcd, cd, d, depth + 1);
}

bool ve_flatten(const VeShape *shape, VeFlat *f) {
    VeShape temporary;
    const VeShape *s = shape;
    if (s->kind != VE_PATH) {
        temporary = *s;
        ve_to_path(&temporary);
        s = &temporary;
    }
    memset(f, 0, sizeof *f);
    f->evenodd = s->evenodd;
    VePoint previous = {0};
    for (int i = 0; i < s->count; i++) {
        const VeNode *n = &s->nodes[i];
        if (n->command == VE_MOVE) {
            if (f->contours >= VE_NODES) {
                return false;
            }
            int c = f->contours++;
            f->start[c] = f->count;
            if (!point(f, n->p)) {
                return false;
            }
            previous = n->p;
        } else if (n->command == VE_CLOSE) {
            if (!f->contours) {
                return false;
            }
            f->closed[f->contours - 1] = true;
            previous = f->points[f->start[f->contours - 1]];
        } else {
            if (!f->contours) {
                return false;
            }
            if (n->command == VE_LINE) {
                if (!point(f, n->p)) {
                    return false;
                }
            } else if (!cubic(f, previous, n->c1, n->c2, n->p, 0)) {
                return false;
            }
            previous = n->p;
        }
    }
    return true;
}

bool ve_inside(const VeFlat *f, double x, double y) {
    bool inside = false;
    int winding = 0;
    for (int c = 0; c < f->contours; c++) {
        int start = f->start[c], n = f->length[c];
        for (int i = 0, j = n - 1; i < n; j = i++) {
            VePoint a = f->points[start + j], b = f->points[start + i];
            if ((a.y > y) != (b.y > y) &&
                x < ((double)b.x - a.x) * (y - a.y) / ((double)b.y - a.y) + a.x) {
                inside = !inside;
                winding += b.y > a.y ? 1 : -1;
            }
        }
    }
    return f->evenodd ? inside : winding != 0;
}
