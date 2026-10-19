/** @file @brief libtess2 adapter for canonical polygon booleans, triangulation and contour bands.
 */
#include "path.h"
#include <math.h>
#include <string.h>
#include <tesselator.h>

static void add(TESStesselator *tess, const VectorPath *p, bool reverse) {
    for (unsigned c = 0, first = 0; c < p->contours; first = p->ends[c++]) {
        unsigned count = p->ends[c] - first;
        VectorPoint points[VECTOR_PATH_POINTS];
        for (unsigned i = 0; i < count; i++) {
            points[i] = p->points[reverse ? p->ends[c] - 1 - i : first + i];
        }
        tessAddContour(tess, 2, points, sizeof *points, (int)count);
    }
}

static bool finish(TESStesselator *tess, int rule, VectorPath *out) {
    const TESSreal normal[3] = {0, 0, 1};
    if (!tessTesselate(tess, rule, TESS_BOUNDARY_CONTOURS, 0, 2, normal)) {
        return false;
    }
    int count = tessGetVertexCount(tess), contours = tessGetElementCount(tess);
    if (count > VECTOR_PATH_POINTS || contours > VECTOR_PATH_CONTOURS) {
        return false;
    }
    VectorPath result = {0};
    const TESSreal *vertices = tessGetVertices(tess);
    const TESSindex *elements = tessGetElements(tess);
    for (int c = 0; c < contours; c++) {
        int first = elements[2 * c], n = elements[2 * c + 1];
        if (n < 3) {
            continue;
        }
        for (int i = 0; i < n; i++) {
            result.points[result.count++] =
                (VectorPoint){vertices[2 * (first + i)], vertices[2 * (first + i) + 1]};
        }
        result.ends[result.contours++] = result.count;
    }
    if (!vector_path_valid(&result)) {
        return false;
    }
    *out = result;
    return true;
}

bool vector_path_combine(const VectorPath *a, const VectorPath *b, VectorAreaOperation operation,
                         VectorPath *out) {
    if (!vector_path_valid(a) || !vector_path_valid(b) || operation < VECTOR_AREA_UNION ||
        operation > VECTOR_AREA_XOR) {
        return false;
    }
    TESStesselator *tess = tessNewTess(NULL);
    if (!tess) {
        return false;
    }
    add(tess, a, false);
    add(tess, b, operation == VECTOR_AREA_DIFFERENCE);
    int rule = operation == VECTOR_AREA_XOR            ? TESS_WINDING_ODD
               : operation == VECTOR_AREA_INTERSECTION ? TESS_WINDING_ABS_GEQ_TWO
                                                       : TESS_WINDING_POSITIVE;
    bool ok = finish(tess, rule, out);
    tessDeleteTess(tess);
    return ok;
}

bool vector_path_triangles(const VectorPath *p, void (*triangle)(void *, const VectorPoint[3]),
                           void *context) {
    if (!p->count) {
        return true;
    }
    if (!vector_path_valid(p) || !triangle) {
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
            VectorPoint points[3];
            bool complete = true;
            for (int j = 0; j < 3; j++) {
                TESSindex at = indices[3 * i + j];
                if (at == TESS_UNDEF) {
                    complete = false;
                    break;
                }
                points[j] = (VectorPoint){vertices[2 * at], vertices[2 * at + 1]};
            }
            if (complete) {
                triangle(context, points);
            }
        }
    }
    tessDeleteTess(tess);
    return ok;
}

static void polygon(TESStesselator *tess, VectorPoint *points, int count) {
    float area = 0;
    for (int i = 0; i < count; i++) {
        VectorPoint a = points[i], b = points[(i + 1) % count];
        area += a.x * b.y - b.x * a.y;
    }
    if (area < 0) {
        for (int i = 0; i < count / 2; i++) {
            VectorPoint swap = points[i];
            points[i] = points[count - 1 - i];
            points[count - 1 - i] = swap;
        }
    }
    tessAddContour(tess, 2, points, sizeof *points, count);
}

bool vector_path_outline(const VectorPath *p, float width, int placement, VectorPath *out) {
    if (!vector_path_valid(p) || !isfinite(width) || width < 0 || placement < -1 || placement > 1) {
        return false;
    }
    if (!width || !p->count) {
        *out = (VectorPath){0};
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
            VectorPoint a = p->points[i], b = p->points[i + 1 < end ? i + 1 : first];
            VectorPoint prev = p->points[i > first ? i - 1 : end - 1];
            float length = hypotf(b.x - a.x, b.y - a.y),
                  previous = hypotf(a.x - prev.x, a.y - prev.y);
            if (length < 1e-7f || previous < 1e-7f) {
                continue;
            }
            VectorPoint n = {-(b.y - a.y) / length, (b.x - a.x) / length};
            VectorPoint pn = {-(a.y - prev.y) / previous, (a.x - prev.x) / previous};
            VectorPoint quad[4] = {{a.x + n.x * half, a.y + n.y * half},
                                   {b.x + n.x * half, b.y + n.y * half},
                                   {b.x - n.x * half, b.y - n.y * half},
                                   {a.x - n.x * half, a.y - n.y * half}};
            polygon(tess, quad, 4);
            float denominator = 1 + n.x * pn.x + n.y * pn.y;
            for (int sign = -1; sign <= 1; sign += 2) {
                VectorPoint join[4] = {a,
                                       {a.x + pn.x * half * sign, a.y + pn.y * half * sign},
                                       a,
                                       {a.x + n.x * half * sign, a.y + n.y * half * sign}};
                if (denominator > .125f) {
                    join[2] = (VectorPoint){a.x + (n.x + pn.x) * half * sign / denominator,
                                            a.y + (n.y + pn.y) * half * sign / denominator};
                    polygon(tess, join, 4);
                } else {
                    join[2] = join[3];
                    polygon(tess, join, 3);
                }
            }
        }
    }
    VectorPath band;
    bool ok = finish(tess, TESS_WINDING_POSITIVE, &band);
    tessDeleteTess(tess);
    if (!ok) {
        return false;
    }
    if (!placement) {
        *out = band;
        return true;
    }
    return vector_path_combine(
        &band, p, placement < 0 ? VECTOR_AREA_INTERSECTION : VECTOR_AREA_DIFFERENCE, out);
}
