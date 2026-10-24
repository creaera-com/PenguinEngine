/** @file @brief Cubic evaluation, analytic bounds and bounded adaptive subdivision. */
#include "pengdraw/pengdraw.h"
#include <math.h>

static bool finite_point(PDrawPoint p) {
    return isfinite(p.x) && isfinite(p.y) && fabsf(p.x) <= 4000000 && fabsf(p.y) <= 4000000;
}

bool pdraw_bezier_valid(const PDrawBezier *c) {
    if (!c || c->count > PDRAW_BEZIER_NODES || c->closed > 1 || (c->closed && c->count < 2)) {
        return false;
    }
    for (unsigned i = 0; i < c->count; i++) {
        if (!finite_point(c->nodes[i].point) || !finite_point(c->nodes[i].incoming) ||
            !finite_point(c->nodes[i].outgoing)) {
            return false;
        }
    }
    return true;
}

PDrawPoint pdraw_bezier_evaluate(PDrawBezierNode a, PDrawBezierNode b, float t) {
    float u = 1 - t;
    return (PDrawPoint){u * u * u * a.point.x + 3 * u * u * t * a.outgoing.x +
                            3 * u * t * t * b.incoming.x + t * t * t * b.point.x,
                        u * u * u * a.point.y + 3 * u * u * t * a.outgoing.y +
                            3 * u * t * t * b.incoming.y + t * t * t * b.point.y};
}

static void include(PDrawRect *bounds, PDrawPoint p) {
    float right = fmaxf(bounds->x + bounds->w, p.x), bottom = fmaxf(bounds->y + bounds->h, p.y);
    bounds->x = fminf(bounds->x, p.x);
    bounds->y = fminf(bounds->y, p.y);
    bounds->w = right - bounds->x;
    bounds->h = bottom - bounds->y;
}

static void extrema(PDrawRect *bounds, PDrawBezierNode a, PDrawBezierNode b, bool y) {
    double p = y ? a.point.y : a.point.x, q = y ? a.outgoing.y : a.outgoing.x;
    double r = y ? b.incoming.y : b.incoming.x, s = y ? b.point.y : b.point.x;
    double aa = -p + 3 * q - 3 * r + s, bb = 2 * (p - 2 * q + r), cc = q - p;
    double roots[2];
    int count = 0;
    if (fabs(aa) < 1e-12) {
        if (fabs(bb) > 1e-12) {
            roots[count++] = -cc / bb;
        }
    } else {
        double d = bb * bb - 4 * aa * cc;
        if (d >= 0) {
            roots[count++] = (-bb + sqrt(d)) / (2 * aa);
            roots[count++] = (-bb - sqrt(d)) / (2 * aa);
        }
    }
    for (int i = 0; i < count; i++) {
        if (roots[i] > 0 && roots[i] < 1) {
            include(bounds, pdraw_bezier_evaluate(a, b, (float)roots[i]));
        }
    }
}

PDrawRect pdraw_bezier_bounds(const PDrawBezier *c) {
    if (!pdraw_bezier_valid(c) || !c->count) {
        return (PDrawRect){0};
    }
    PDrawRect bounds = {c->nodes[0].point.x, c->nodes[0].point.y, 0, 0};
    unsigned segments = c->closed ? c->count : c->count - 1;
    for (unsigned i = 0; i < segments; i++) {
        PDrawBezierNode a = c->nodes[i], b = c->nodes[(i + 1) % c->count];
        include(&bounds, b.point);
        extrema(&bounds, a, b, false);
        extrema(&bounds, a, b, true);
    }
    return bounds;
}

static PDrawPoint mid(PDrawPoint a, PDrawPoint b) {
    return (PDrawPoint){(a.x + b.x) / 2, (a.y + b.y) / 2};
}

static float distance(PDrawPoint p, PDrawPoint a, PDrawPoint b) {
    float dx = b.x - a.x, dy = b.y - a.y, length = dx * dx + dy * dy;
    float t = length > 0 ? ((p.x - a.x) * dx + (p.y - a.y) * dy) / length : 0;
    t = fmaxf(0, fminf(1, t));
    return hypotf(p.x - a.x - dx * t, p.y - a.y - dy * t);
}

static bool subdivide(PDrawPath *out, PDrawPoint a, PDrawPoint b, PDrawPoint c, PDrawPoint d,
                      float tolerance, int depth) {
    if (depth >= 12 || fmaxf(distance(b, a, d), distance(c, a, d)) <= tolerance) {
        if (out->count == PDRAW_PATH_POINTS) {
            return false;
        }
        out->points[out->count++] = d;
        return true;
    }
    PDrawPoint ab = mid(a, b), bc = mid(b, c), cd = mid(c, d), abc = mid(ab, bc), bcd = mid(bc, cd),
               center = mid(abc, bcd);
    return subdivide(out, a, ab, abc, center, tolerance, depth + 1) &&
           subdivide(out, center, bcd, cd, d, tolerance, depth + 1);
}

bool pdraw_bezier_flatten(const PDrawBezier *c, float tolerance, PDrawPath *out) {
    if (!out || !pdraw_bezier_valid(c) || c->count < 2 || !isfinite(tolerance) || tolerance <= 0) {
        return false;
    }
    for (int attempt = 0; attempt < 32; attempt++, tolerance *= 2) {
        PDrawPath path = {.count = 1, .contours = 1};
        path.points[0] = c->nodes[0].point;
        bool ok = true;
        unsigned segments = c->closed ? c->count : c->count - 1;
        for (unsigned i = 0; i < segments && ok; i++) {
            PDrawBezierNode a = c->nodes[i], b = c->nodes[(i + 1) % c->count];
            ok = subdivide(&path, a.point, a.outgoing, b.incoming, b.point, tolerance, 0);
        }
        if (ok) {
            if (c->closed) {
                path.count--;
            }
            if (c->closed && path.count < 3) {
                return false;
            }
            path.ends[0] = path.count;
            *out = path;
            return true;
        }
    }
    return false;
}

static PDrawPoint mix(PDrawPoint a, PDrawPoint b, float t) {
    return (PDrawPoint){a.x + (b.x - a.x) * t, a.y + (b.y - a.y) * t};
}

bool pdraw_bezier_insert(PDrawBezier *c, unsigned segment, float t) {
    if (!pdraw_bezier_valid(c) || c->count < 2 || c->count == PDRAW_BEZIER_NODES ||
        segment >= (c->closed ? c->count : c->count - 1) || !isfinite(t) || t <= 0 || t >= 1) {
        return false;
    }
    PDrawBezierNode a = c->nodes[segment], b = c->nodes[(segment + 1) % c->count];
    PDrawPoint ab = mix(a.point, a.outgoing, t), bc = mix(a.outgoing, b.incoming, t),
               cd = mix(b.incoming, b.point, t);
    PDrawPoint abc = mix(ab, bc, t), bcd = mix(bc, cd, t), center = mix(abc, bcd, t);
    PDrawBezier result = *c;
    for (unsigned i = result.count; i > segment + 1; i--) {
        result.nodes[i] = result.nodes[i - 1];
    }
    result.count++;
    result.nodes[segment].outgoing = ab;
    result.nodes[(segment + 2) % result.count].incoming = cd;
    result.nodes[segment + 1] = (PDrawBezierNode){center, abc, bcd};
    *c = result;
    return true;
}
