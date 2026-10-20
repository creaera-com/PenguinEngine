/** @file @brief Validated gradient values and pure color sampling. */
#include "state.h"
#include <math.h>
#include <string.h>

bool vector_gradient_valid(const VectorGradient *g) {
    if (!g || g->kind > VECTOR_GRADIENT_REFLECTED || g->count > VECTOR_GRADIENT_STOPS ||
        (g->count && g->count < 2) || (g->kind && !g->count)) {
        return false;
    }
    float points[] = {g->x1, g->y1, g->x2, g->y2};
    for (unsigned i = 0; i < 4; i++) {
        if (!isfinite(points[i]) || (points[i] < -16 || points[i] > 17)) {
            return false;
        }
    }
    if (g->kind && hypotf(g->x2 - g->x1, g->y2 - g->y1) < .0001f) {
        return false;
    }
    for (unsigned i = 0; i < g->count; i++) {
        float p = g->stops[i].position;
        if (!isfinite(p) || p < 0 || p > 1 || (i && p <= g->stops[i - 1].position)) {
            return false;
        }
    }
    return !g->count || (g->stops[0].position == 0 && g->stops[g->count - 1].position == 1);
}

VectorGradient vector_gradient_default(uint32_t fill) {
    return (VectorGradient){.kind = VECTOR_GRADIENT_LINEAR,
                            .count = 2,
                            .x1 = 0,
                            .y1 = .5f,
                            .x2 = 1,
                            .y2 = .5f,
                            .stops = {{0, fill & 255 ? fill : 0xc58b5bff}, {1, 0xffffffff}}};
}

static uint32_t ramp(const VectorGradient *g, float t) {
    t = fmaxf(0, fminf(1, t));
    unsigned end = 1;
    while (end + 1 < g->count && t > g->stops[end].position) {
        end++;
    }
    VectorGradientStop a = g->stops[end - 1], b = g->stops[end];
    float u = (t - a.position) / (b.position - a.position);
    float aa = a.color & 255, ba = b.color & 255;
    float alpha = aa * (1 - u) + ba * u;
    uint32_t result = (uint32_t)lroundf(alpha);
    for (int shift = 8; shift <= 24; shift += 8) {
        float color = alpha > 0 ? (((a.color >> shift) & 255) * aa * (1 - u) +
                                   ((b.color >> shift) & 255) * ba * u) /
                                      alpha
                                : 0;
        result |= (uint32_t)lroundf(fminf(255, fmaxf(0, color))) << shift;
    }
    return result;
}

uint32_t vector_gradient_sample(const VectorGradient *g, float x, float y, float w, float h) {
    if (!g || g->count < 2 || g->count > VECTOR_GRADIENT_STOPS || w <= 0 || h <= 0) {
        return 0;
    }
    float dx = (g->x2 - g->x1) * w, dy = (g->y2 - g->y1) * h;
    float px = x - g->x1 * w, py = y - g->y1 * h, length = dx * dx + dy * dy;
    float t = length > 0 ? (px * dx + py * dy) / length : 0;
    if (g->kind == VECTOR_GRADIENT_RADIAL) {
        t = length > 0 ? hypotf(px, py) / sqrtf(length) : 0;
    } else if (g->kind == VECTOR_GRADIENT_REFLECTED) {
        t = fabsf(t);
    } else if (g->kind == VECTOR_GRADIENT_CONICAL) {
        t = (atan2f(py, px) - atan2f(dy, dx)) / 6.283185307f;
        t -= floorf(t);
    }
    return ramp(g, t);
}

int vector_gradient_add_stop(VectorGradient *g) {
    if (!vector_gradient_valid(g) || g->count < 2 || g->count == VECTOR_GRADIENT_STOPS) {
        return -1;
    }
    unsigned at = 1;
    for (unsigned i = 2; i < g->count; i++) {
        if (g->stops[i].position - g->stops[i - 1].position >
            g->stops[at].position - g->stops[at - 1].position) {
            at = i;
        }
    }
    float p = (g->stops[at].position + g->stops[at - 1].position) / 2;
    uint32_t color = ramp(g, p);
    memmove(&g->stops[at + 1], &g->stops[at], (g->count - at) * sizeof *g->stops);
    g->stops[at] = (VectorGradientStop){p, color};
    g->count++;
    return (int)at;
}

bool vector_gradient_remove_stop(VectorGradient *g, unsigned index) {
    if (!vector_gradient_valid(g) || g->count <= 2 || index >= g->count) {
        return false;
    }
    memmove(&g->stops[index], &g->stops[index + 1], (g->count - index - 1) * sizeof *g->stops);
    g->stops[--g->count] = (VectorGradientStop){0};
    g->stops[0].position = 0;
    g->stops[g->count - 1].position = 1;
    return true;
}
