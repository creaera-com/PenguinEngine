/** @file @brief Value geometry, winding tests and connected components for polygonal paths. */
#include "path.h"
#include <math.h>
#include <string.h>

bool vector_path_valid(const VectorPath *p) {
    if (!p || p->count > VECTOR_PATH_POINTS || p->contours > VECTOR_PATH_CONTOURS ||
        ((!p->count) != (!p->contours))) {
        return false;
    }
    unsigned first = 0;
    for (unsigned c = 0; c < p->contours; c++) {
        if (p->ends[c] < first + 3 || p->ends[c] > p->count) {
            return false;
        }
        first = p->ends[c];
    }
    if (first != p->count) {
        return false;
    }
    for (unsigned i = 0; i < p->count; i++) {
        if (!isfinite(p->points[i].x) || !isfinite(p->points[i].y) ||
            fabsf(p->points[i].x) > 4000000 || fabsf(p->points[i].y) > 4000000) {
            return false;
        }
    }
    return true;
}

float vector_path_contour_area(const VectorPath *p, unsigned c) {
    unsigned first = c ? p->ends[c - 1] : 0, end = p->ends[c];
    double area = 0;
    for (unsigned i = first; i < end; i++) {
        VectorPoint a = p->points[i], b = p->points[i + 1 < end ? i + 1 : first];
        area += (double)a.x * b.y - (double)b.x * a.y;
    }
    return (float)(area / 2);
}

static int winding(const VectorPath *p, unsigned c, float x, float y) {
    unsigned first = c ? p->ends[c - 1] : 0, end = p->ends[c];
    int result = 0;
    for (unsigned i = first; i < end; i++) {
        VectorPoint a = p->points[i], b = p->points[i + 1 < end ? i + 1 : first];
        double side = ((double)b.x - a.x) * (y - a.y) - ((double)b.y - a.y) * (x - a.x);
        if (a.y <= y && b.y > y && side > 0) {
            result++;
        }
        if (a.y > y && b.y <= y && side < 0) {
            result--;
        }
    }
    return result;
}

bool vector_path_contains(const VectorPath *p, float x, float y) {
    int total = 0;
    for (unsigned c = 0; c < p->contours; c++) {
        total += winding(p, c, x, y);
    }
    return total != 0;
}

PUIRect vector_path_bounds(const VectorPath *p) {
    if (!p->count) {
        return (PUIRect){0};
    }
    float x = p->points[0].x, y = p->points[0].y, right = x, bottom = y;
    for (unsigned i = 1; i < p->count; i++) {
        x = fminf(x, p->points[i].x);
        y = fminf(y, p->points[i].y);
        right = fmaxf(right, p->points[i].x);
        bottom = fmaxf(bottom, p->points[i].y);
    }
    return (PUIRect){x, y, right - x, bottom - y};
}

bool vector_path_intersects(const VectorPath *p, PUIRect box) {
    if (box.w <= 0 || box.h <= 0) {
        return false;
    }
    if (vector_path_contains(p, box.x, box.y) || vector_path_contains(p, box.x + box.w, box.y) ||
        vector_path_contains(p, box.x, box.y + box.h) ||
        vector_path_contains(p, box.x + box.w, box.y + box.h)) {
        return true;
    }
    for (unsigned c = 0, first = 0; c < p->contours; first = p->ends[c++]) {
        for (unsigned i = first; i < p->ends[c]; i++) {
            VectorPoint a = p->points[i], b = p->points[i + 1 < p->ends[c] ? i + 1 : first];
            float low = 0, high = 1;
            float delta[2] = {b.x - a.x, b.y - a.y}, pos[2] = {a.x, a.y};
            float minimum[2] = {box.x, box.y}, maximum[2] = {box.x + box.w, box.y + box.h};
            for (int axis = 0; axis < 2 && low <= high; axis++) {
                if (fabsf(delta[axis]) < 1e-12f) {
                    if (pos[axis] < minimum[axis] || pos[axis] > maximum[axis]) {
                        high = -1;
                    }
                } else {
                    float from = (minimum[axis] - pos[axis]) / delta[axis];
                    float to = (maximum[axis] - pos[axis]) / delta[axis];
                    low = fmaxf(low, fminf(from, to));
                    high = fminf(high, fmaxf(from, to));
                }
            }
            if (low <= high) {
                return true;
            }
        }
    }
    return false;
}

static bool append(VectorPath *out, const VectorPath *p, unsigned c) {
    unsigned first = c ? p->ends[c - 1] : 0, count = p->ends[c] - first;
    if (out->count + count > VECTOR_PATH_POINTS || out->contours >= VECTOR_PATH_CONTOURS) {
        return false;
    }
    memcpy(out->points + out->count, p->points + first, count * sizeof *p->points);
    out->count += count;
    out->ends[out->contours++] = out->count;
    return true;
}

int vector_path_components(const VectorPath *p, VectorPath *out, int capacity) {
    int count = 0, owners[VECTOR_PATH_CONTOURS];
    for (unsigned c = 0; c < p->contours; c++) {
        owners[c] = -1;
        if (vector_path_contour_area(p, c) > 0) {
            if (count >= capacity) {
                return -1;
            }
            out[count] = (VectorPath){0};
            if (!append(&out[count], p, c)) {
                return -1;
            }
            owners[c] = count++;
        }
    }
    for (unsigned c = 0; c < p->contours; c++) {
        if (vector_path_contour_area(p, c) >= 0) {
            continue;
        }
        VectorPoint point = p->points[c ? p->ends[c - 1] : 0];
        int owner = -1;
        float smallest = INFINITY;
        for (unsigned j = 0; j < p->contours; j++) {
            float area = vector_path_contour_area(p, j);
            if (owners[j] >= 0 && area < smallest && winding(p, j, point.x, point.y)) {
                smallest = area;
                owner = owners[j];
            }
        }
        if (owner < 0 || !append(&out[owner], p, c)) {
            return -1;
        }
    }
    return count;
}
