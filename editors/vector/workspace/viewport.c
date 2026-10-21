/** @file @brief Time-based viewport zoom, independent of rendering and native events. */
#include "viewport.h"
#include <math.h>

void vector_viewport_update(VectorViewport *v, double now_ms) {
    if (!v->zooming) {
        return;
    }
    float t = (float)fmax(0, fmin(1, (now_ms - v->zoom_started_ms) / 140));
    float eased = t * t * (3 - 2 * t);
    float zoom = v->zoom_from * powf(v->zoom_target / v->zoom_from, eased);
    v->x = v->zoom_anchor_x - (v->zoom_anchor_x - v->x) * zoom / v->zoom;
    v->y = v->zoom_anchor_y - (v->zoom_anchor_y - v->y) * zoom / v->zoom;
    v->zoom = zoom;
    if (t >= 1) {
        v->zooming = false;
    }
}

void vector_viewport_zoom(VectorViewport *v, float steps, float x, float y, double now_ms) {
    if (!isfinite(steps) || !isfinite(x) || !isfinite(y) || steps == 0) {
        return;
    }
    vector_viewport_update(v, now_ms);
    float target = v->zooming ? v->zoom_target : v->zoom;
    v->zoom_target = fmaxf(.1f, fminf(4, target * powf(1.1f, steps)));
    v->zoom_from = v->zoom;
    v->zoom_anchor_x = x;
    v->zoom_anchor_y = y;
    v->zoom_started_ms = now_ms;
    v->zooming = v->zoom_target != v->zoom;
}
