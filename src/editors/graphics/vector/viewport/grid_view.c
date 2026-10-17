#include "editors/graphics/vector/viewport/grid.h"
#include <math.h>
#include <stdint.h>

static bool emit(VeRegion r, double a, double b, double c, VeGridLine *line) {
    double left = r.x, right = r.x + r.w, top = r.y, bottom = r.y + r.h;
    if (b == 0) {
        double x = c / a;
        if (x < left || x > right) {
            return false;
        }
        *line = (VeGridLine){{(float)x, (float)top}, {(float)x, (float)bottom}};
        return true;
    }
    if (a == 0) {
        double y = c / b;
        if (y < top || y > bottom) {
            return false;
        }
        *line = (VeGridLine){{(float)left, (float)y}, {(float)right, (float)y}};
        return true;
    }
    double first = (c - b * top) / a, last = (c - b * bottom) / a;
    left = fmax(left, fmin(first, last));
    right = fmin(right, fmax(first, last));
    if (right - left < 1e-6) {
        return false;
    }
    *line = (VeGridLine){{(float)left, (float)fmax(top, fmin(bottom, (c - a * left) / b))},
                         {(float)right, (float)fmax(top, fmin(bottom, (c - a * right) / b))}};
    return true;
}

int ve_grid_lines(VeRegion r, float width, float height, bool iso, float zoom, VeGridLine *lines,
                  int capacity) {
    if (!lines || capacity < 4 || capacity > 65536 || !isfinite(width) || !isfinite(height) ||
        width < 1 || height < 1 || width > 100000 || height > 100000 || !isfinite(zoom) ||
        zoom < .02f || zoom > 32 || !isfinite(r.x) || !isfinite(r.y) || !isfinite(r.w) ||
        !isfinite(r.h) || fabsf(r.x) > 100000 || fabsf(r.y) > 100000 || r.w <= 0 || r.h <= 0 ||
        r.w > 100000 || r.h > 100000) {
        return 0;
    }
    double spacing = fmax(6, (r.w + r.h) * zoom / fmax(1, capacity / 2 - 2));
    int count = 0;
    for (int family = 0; family < 2; family++) {
        double a = iso ? (family ? -1 : 1) * (double)height / width : (family ? 0 : 1);
        double b = iso || family ? 1 : 0;
        double step = iso || family ? height : width;
        while (step * zoom / hypot(a, b) < spacing) {
            step *= 2;
        }
        double c0 = a * r.x + b * r.y, c1 = a * (r.x + r.w) + b * (r.y + r.h);
        double c2 = a * r.x + b * (r.y + r.h), c3 = a * (r.x + r.w) + b * r.y;
        int64_t first = (int64_t)ceil(fmin(fmin(c0, c1), fmin(c2, c3)) / step);
        int64_t last = (int64_t)floor(fmax(fmax(c0, c1), fmax(c2, c3)) / step);
        int iterations = 0;
        for (int64_t k = first; k <= last && count < capacity && iterations < capacity;
             k++, iterations++) {
            if (emit(r, a, b, k * step, &lines[count])) {
                count++;
            }
        }
    }
    return count;
}

bool ve_grid_snap(VePoint point, float width, float height, bool isometric, float zoom,
                  float tolerance, VePoint *result) {
    if (!result || !isfinite(point.x) || !isfinite(point.y) || !isfinite(width) ||
        !isfinite(height) || width < 1 || height < 1 || width > 100000 || height > 100000 ||
        !isfinite(zoom) || zoom < .02f || zoom > 32 || !isfinite(tolerance) || tolerance < 0 ||
        tolerance > 100) {
        return false;
    }
    double x = round((double)point.x / width) * width;
    double y = round((double)point.y / height) * height;
    double distance = hypot(x - point.x, y - point.y);
    if (isometric) {
        /* The diamond lattice is the union of two offset rectangular lattices. */
        double ox = (round((double)point.x / width - .5) + .5) * width;
        double oy = (round((double)point.y / height - .5) + .5) * height;
        double other = hypot(ox - point.x, oy - point.y);
        if (other < distance) {
            x = ox;
            y = oy;
            distance = other;
        }
    }
    if (distance * zoom > tolerance) {
        return false;
    }
    *result = (VePoint){(float)x, (float)y};
    return true;
}
