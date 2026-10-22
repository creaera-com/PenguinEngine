/** @file @brief libtess2 adapter for canonical polygon booleans, triangulation and contour bands.
 */
#include "pengdraw/pengdraw.h"
#include <math.h>
#include <string.h>
#include <tesselator.h>

static void add(TESStesselator *tess, const PDrawPath *p, bool reverse) {
    for (unsigned c = 0, first = 0; c < p->contours; first = p->ends[c++]) {
        unsigned count = p->ends[c] - first;
        PDrawPoint points[PDRAW_PATH_POINTS];
        for (unsigned i = 0; i < count; i++) {
            points[i] = p->points[reverse ? p->ends[c] - 1 - i : first + i];
        }
        tessAddContour(tess, 2, points, sizeof *points, (int)count);
    }
}

static bool finish(TESStesselator *tess, int rule, PDrawPath *out) {
    const TESSreal normal[3] = {0, 0, 1};
    if (!tessTesselate(tess, rule, TESS_BOUNDARY_CONTOURS, 0, 2, normal)) {
        return false;
    }
    int count = tessGetVertexCount(tess), contours = tessGetElementCount(tess);
    if (count > PDRAW_PATH_POINTS || contours > PDRAW_PATH_CONTOURS) {
        return false;
    }
    PDrawPath result = {0};
    const TESSreal *vertices = tessGetVertices(tess);
    const TESSindex *elements = tessGetElements(tess);
    for (int c = 0; c < contours; c++) {
        int first = elements[2 * c], n = elements[2 * c + 1];
        if (n < 3) {
            continue;
        }
        for (int i = 0; i < n; i++) {
            result.points[result.count++] =
                (PDrawPoint){vertices[2 * (first + i)], vertices[2 * (first + i) + 1]};
        }
        result.ends[result.contours++] = result.count;
    }
    if (!pdraw_path_valid(&result)) {
        return false;
    }
    *out = result;
    return true;
}

bool pdraw_path_combine(const PDrawPath *a, const PDrawPath *b, PDrawAreaOperation operation,
                        PDrawPath *out) {
    if (!pdraw_path_valid(a) || !pdraw_path_valid(b) || operation < PDRAW_AREA_UNION ||
        operation > PDRAW_AREA_XOR) {
        return false;
    }
    TESStesselator *tess = tessNewTess(NULL);
    if (!tess) {
        return false;
    }
    add(tess, a, false);
    add(tess, b, operation == PDRAW_AREA_DIFFERENCE);
    int rule = operation == PDRAW_AREA_XOR            ? TESS_WINDING_ODD
               : operation == PDRAW_AREA_INTERSECTION ? TESS_WINDING_ABS_GEQ_TWO
                                                      : TESS_WINDING_POSITIVE;
    bool ok = finish(tess, rule, out);
    tessDeleteTess(tess);
    return ok;
}

bool pdraw_path_triangles(const PDrawPath *p, void (*triangle)(void *, const PDrawPoint[3]),
                          void *context) {
    if (!p->count) {
        return true;
    }
    if (!pdraw_path_valid(p) || !triangle) {
        return false;
    }
    TESStesselator *tess = tessNewTess(NULL);
    if (!tess) {
        return false;
    }
    add(tess, p, false);
    const TESSreal normal[3] = {0, 0, 1};
    bool ok = tessTesselate(tess, TESS_WINDING_POSITIVE, TESS_POLYGONS, 3, 2, normal) != 0;
    if (ok) {
        const TESSreal *vertices = tessGetVertices(tess);
        const TESSindex *indices = tessGetElements(tess);
        for (int i = 0; i < tessGetElementCount(tess); i++) {
            PDrawPoint points[3];
            bool complete = true;
            for (int j = 0; j < 3; j++) {
                TESSindex at = indices[3 * i + j];
                if (at == TESS_UNDEF) {
                    complete = false;
                    break;
                }
                points[j] = (PDrawPoint){vertices[2 * at], vertices[2 * at + 1]};
            }
            if (complete) {
                triangle(context, points);
            }
        }
    }
    tessDeleteTess(tess);
    return ok;
}

static void polygon(TESStesselator *tess, PDrawPoint *points, int count) {
    float area = 0;
    for (int i = 0; i < count; i++) {
        PDrawPoint a = points[i], b = points[(i + 1) % count];
        area += a.x * b.y - b.x * a.y;
    }
    if (area < 0) {
        for (int i = 0; i < count / 2; i++) {
            PDrawPoint swap = points[i];
            points[i] = points[count - 1 - i];
            points[count - 1 - i] = swap;
        }
    }
    tessAddContour(tess, 2, points, sizeof *points, count);
}

bool pdraw_path_outline(const PDrawPath *p, float width, int placement, PDrawPath *out) {
    if (!pdraw_path_valid(p) || !isfinite(width) || width < 0 || placement < -1 || placement > 1) {
        return false;
    }
    if (!width || !p->count) {
        *out = (PDrawPath){0};
        return true;
    }
    TESStesselator *tess = tessNewTess(NULL);
    if (!tess) {
        return false;
    }
    float half = placement ? width : width / 2;
    for (unsigned c = 0, first = 0; c < p->contours; first = p->ends[c++]) {
        unsigned end = p->ends[c];
        for (unsigned i = first; i < end; i++) {
            PDrawPoint a = p->points[i], b = p->points[i + 1 < end ? i + 1 : first];
            PDrawPoint prev = p->points[i > first ? i - 1 : end - 1];
            float length = hypotf(b.x - a.x, b.y - a.y),
                  previous = hypotf(a.x - prev.x, a.y - prev.y);
            if (length < 1e-7f || previous < 1e-7f) {
                continue;
            }
            PDrawPoint n = {-(b.y - a.y) / length, (b.x - a.x) / length};
            PDrawPoint pn = {-(a.y - prev.y) / previous, (a.x - prev.x) / previous};
            PDrawPoint quad[4] = {{a.x + n.x * half, a.y + n.y * half},
                                  {b.x + n.x * half, b.y + n.y * half},
                                  {b.x - n.x * half, b.y - n.y * half},
                                  {a.x - n.x * half, a.y - n.y * half}};
            polygon(tess, quad, 4);
            float denominator = 1 + n.x * pn.x + n.y * pn.y;
            for (int sign = -1; sign <= 1; sign += 2) {
                PDrawPoint join[4] = {a,
                                      {a.x + pn.x * half * sign, a.y + pn.y * half * sign},
                                      a,
                                      {a.x + n.x * half * sign, a.y + n.y * half * sign}};
                if (denominator > .125f) {
                    join[2] = (PDrawPoint){a.x + (n.x + pn.x) * half * sign / denominator,
                                           a.y + (n.y + pn.y) * half * sign / denominator};
                    polygon(tess, join, 4);
                } else {
                    join[2] = join[3];
                    polygon(tess, join, 3);
                }
            }
        }
    }
    PDrawPath band;
    bool ok = finish(tess, TESS_WINDING_POSITIVE, &band);
    tessDeleteTess(tess);
    if (!ok) {
        return false;
    }
    if (!placement) {
        *out = band;
        return true;
    }
    return pdraw_path_combine(&band, p,
                              placement < 0 ? PDRAW_AREA_INTERSECTION : PDRAW_AREA_DIFFERENCE, out);
}
