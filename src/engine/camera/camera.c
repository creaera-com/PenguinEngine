#include "penguin/engine.h"
#include <math.h>
#include <stdlib.h>
#include <string.h>

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
