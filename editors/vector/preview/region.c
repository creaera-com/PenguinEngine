/** @file @brief Pure geometry for a bounded, pixel-aligned 1:1 preview window. */
#include "editors/vector/artboard/artboard.h"
#include "state.h"
#include <math.h>

static bool finite_rect(PUIRect r) {
    return isfinite(r.x) && isfinite(r.y) && isfinite(r.w) && isfinite(r.h) &&
           isfinite(r.x + r.w) && isfinite(r.y + r.h);
}

bool vector_preview_valid(unsigned board, PUIRect r) {
    return finite_rect(r) && r.x >= 0 && r.y >= 0 && r.w >= (board ? 1 : 0) &&
           r.h >= (board ? 1 : 0) && r.x + r.w <= VECTOR_MAX_ARTBOARD_SIZE &&
           r.y + r.h <= VECTOR_MAX_ARTBOARD_SIZE;
}

bool vector_preview_window(PUIRect selection, PUIRect viewport, float cx, float cy,
                           VectorPreviewWindow *out) {
    if (!out || !finite_rect(selection) || !finite_rect(viewport) || selection.w < 1 ||
        selection.h < 1 || viewport.w < 1 || viewport.h < 1 || !isfinite(cx) || !isfinite(cy)) {
        return false;
    }
    float w = floorf(fminf(selection.w, viewport.w));
    float h = floorf(fminf(selection.h, viewport.h));
    float x = fmaxf(selection.x, fminf(selection.x + selection.w - w, floorf(cx - w / 2)));
    float y = fmaxf(selection.y, fminf(selection.y + selection.h - h, floorf(cy - h / 2)));
    *out = (VectorPreviewWindow){{x, y, w, h},
                                 {floorf(viewport.x + (viewport.w - w) / 2),
                                  floorf(viewport.y + (viewport.h - h) / 2), w, h}};
    return true;
}
