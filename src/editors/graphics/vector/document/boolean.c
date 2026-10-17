#include "editors/graphics/vector/document/document.h"
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define EDGE_LIMIT (VE_FLAT * 4)

typedef struct {
    VePoint a, b;
    bool used;
} Edge;

static float cross(VePoint a, VePoint b) {
    return a.x * b.y - a.y * b.x;
}

static VePoint sub(VePoint a, VePoint b) {
    return (VePoint){a.x - b.x, a.y - b.y};
}

static VePoint along(VePoint a, VePoint b, float t) {
    return (VePoint){a.x + (b.x - a.x) * t, a.y + (b.y - a.y) * t};
}

static bool same(VePoint a, VePoint b) {
    return hypotf(a.x - b.x, a.y - b.y) < .0005f;
}

static bool result(bool a, bool b, VeBoolean op) {
    return op == VE_UNION          ? a || b
           : op == VE_INTERSECTION ? a && b
           : op == VE_DIFFERENCE   ? a && !b
                                   : a != b;
}

static int compare(const void *a, const void *b) {
    float x = *(const float *)a, y = *(const float *)b;
    return (x > y) - (x < y);
}

static int edges(const VeFlat *f, Edge *e, int count) {
    for (int c = 0; c < f->contours; c++) {
        int start = f->start[c], n = f->length[c];
        for (int i = 0; i < n; i++) {
            VePoint a = f->points[start + i], b = f->points[start + (i + 1) % n];
            if (!same(a, b)) {
                e[count++] = (Edge){a, b, false};
            }
        }
    }
    return count;
}

bool ve_boolean(VeDocument *d, int ia, int ib, VeBoolean op, char *error, size_t size) {
    if (ia < 0 || ib < 0 || ia >= VE_SHAPES || ib >= VE_SHAPES || ia == ib || !d->shapes[ia].used ||
        !d->shapes[ib].used || d->shapes[ia].board != d->shapes[ib].board || op < 0 ||
        op > VE_XOR) {
        snprintf(error, size, "Choose two shapes on the same artboard");
        return false;
    }
    if (!d->shapes[ia].fill_enabled || !d->shapes[ib].fill_enabled) {
        snprintf(error, size, "Enable fill on both shapes before boolean operations");
        return false;
    }
    VeFlat *a = malloc(sizeof *a), *b = malloc(sizeof *b);
    Edge *source = calloc(EDGE_LIMIT, sizeof *source), *out = calloc(EDGE_LIMIT, sizeof *out);
    float *cuts = malloc((EDGE_LIMIT * 2 + 2) * sizeof *cuts);
    bool ok = false;
    int out_count = 0;
    uint64_t work = 0;
    if (!a || !b || !source || !out || !cuts) {
        snprintf(error, size, "Out of memory");
        goto done;
    }
    if (!ve_flatten(&d->shapes[ia], a) || !ve_flatten(&d->shapes[ib], b)) {
        snprintf(error, size, "Path too complex to flatten");
        goto done;
    }
    int count = edges(a, source, 0);
    count = edges(b, source, count);
    for (int i = 0; i < count; i++) {
        VePoint p = source[i].a, q = source[i].b, r = sub(q, p);
        float length = hypotf(r.x, r.y);
        int n = 2;
        cuts[0] = 0;
        cuts[1] = 1;
        for (int j = 0; j < count; j++) {
            if (++work > 50000000) {
                snprintf(error, size, "Boolean complexity budget exceeded");
                goto done;
            }
            VePoint u = source[j].a, v = source[j].b, s = sub(v, u), delta = sub(u, p);
            float den = cross(r, s);
            if (fabsf(den) > 1e-7f) {
                float t = cross(delta, s) / den, k = cross(delta, r) / den;
                if (t > 0 && t < 1 && k >= -.00001f && k <= 1.00001f) {
                    cuts[n++] = t;
                }
            } else if (fabsf(cross(delta, r)) < .0001f) {
                float dot = r.x * r.x + r.y * r.y;
                if (dot > 0) {
                    float t = ((u.x - p.x) * r.x + (u.y - p.y) * r.y) / dot,
                          k = ((v.x - p.x) * r.x + (v.y - p.y) * r.y) / dot;
                    if (t > 0 && t < 1) {
                        cuts[n++] = t;
                    }
                    if (k > 0 && k < 1) {
                        cuts[n++] = k;
                    }
                }
            }
        }
        qsort(cuts, (size_t)n, sizeof *cuts, compare);
        for (int j = 0; j < n - 1; j++) {
            if ((cuts[j + 1] - cuts[j]) * length < .0005f) {
                continue;
            }
            work += (uint64_t)(a->count + b->count) * 4;
            if (work > 50000000) {
                snprintf(error, size, "Boolean complexity budget exceeded");
                goto done;
            }
            double t = ((double)cuts[j] + cuts[j + 1]) * .5;
            double mx = p.x + ((double)q.x - p.x) * t, my = p.y + ((double)q.y - p.y) * t;
            double eps = fmin(.002, (cuts[j + 1] - cuts[j]) * length * .01);
            double nx = -r.y / length * eps, ny = r.x / length * eps;
            bool left = result(ve_inside(a, mx + nx, my + ny), ve_inside(b, mx + nx, my + ny), op);
            bool right = result(ve_inside(a, mx - nx, my - ny), ve_inside(b, mx - nx, my - ny), op);
            if (left == right) {
                continue;
            }
            Edge edge = {along(p, q, left ? cuts[j] : cuts[j + 1]),
                         along(p, q, left ? cuts[j + 1] : cuts[j]), false};
            bool duplicate = false;
            for (int k = 0; k < out_count; k++) {
                if (same(out[k].a, edge.a) && same(out[k].b, edge.b)) {
                    duplicate = true;
                    break;
                }
            }
            if (!duplicate) {
                if (out_count == EDGE_LIMIT) {
                    snprintf(error, size, "Boolean edge limit exceeded");
                    goto done;
                }
                out[out_count++] = edge;
            }
        }
    }
    VeShape shape = d->shapes[ia];
    shape.kind = VE_PATH;
    shape.evenodd = true;
    shape.count = 0;
    for (int i = 0; i < out_count; i++) {
        if (out[i].used) {
            continue;
        }
        int current = i;
        VePoint start = out[i].a, previous = start;
        bool closed = false;
        if (shape.count >= VE_NODES) {
            goto complex;
        }
        shape.nodes[shape.count++] = (VeNode){.command = VE_MOVE, .p = start};
        for (int step = 0; step <= out_count; step++) {
            Edge *edge = &out[current];
            edge->used = true;
            VePoint end = edge->b;
            if (same(end, start)) {
                closed = true;
                break;
            }
            if (shape.count >= VE_NODES - 1) {
                goto complex;
            }
            shape.nodes[shape.count++] = (VeNode){.command = VE_LINE, .p = end};
            int next = -1;
            float best = -INFINITY;
            VePoint in = sub(end, previous);
            for (int k = 0; k < out_count; k++) {
                if (!out[k].used && same(out[k].a, end)) {
                    VePoint dir = sub(out[k].b, out[k].a);
                    float angle = atan2f(cross(in, dir), in.x * dir.x + in.y * dir.y);
                    if (angle > best) {
                        best = angle;
                        next = k;
                    }
                }
            }
            if (next < 0) {
                snprintf(error, size, "Ambiguous contour; input was left unchanged");
                goto done;
            }
            previous = end;
            current = next;
        }
        if (!closed) {
            snprintf(error, size, "Cannot close boolean contour");
            goto done;
        }
        if (shape.count >= VE_NODES) {
            goto complex;
        }
        shape.nodes[shape.count++] = (VeNode){.command = VE_CLOSE};
    }
    if (shape.count) {
        ve_bounds(&shape);
        snprintf(shape.name, 64, "Boolean %u", shape.id);
        d->shapes[ia] = shape;
    } else {
        d->shapes[ia].used = false;
    }
    d->shapes[ib].used = false;
    ok = true;
    goto done;
complex:
    snprintf(error, size, "Result exceeds %d path nodes", VE_NODES);
done:
    free(a);
    free(b);
    free(source);
    free(out);
    free(cuts);
    return ok;
}
