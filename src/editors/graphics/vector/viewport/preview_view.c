#include "editors/graphics/vector/viewport/preview.h"
#include <math.h>

static bool valid(VeRegion r) {
    return isfinite(r.x) && isfinite(r.y) && isfinite(r.w) && isfinite(r.h) && r.w >= 1 &&
           r.h >= 1 && isfinite(r.x + r.w) && isfinite(r.y + r.h);
}

bool ve_preview_window(VeRegion selection, VeRegion viewport, float cx, float cy,
                       VePreviewWindow *result) {
    if (!result || !valid(selection) || !valid(viewport) || !isfinite(cx) || !isfinite(cy)) {
        return false;
    }
    float w = floorf(fminf(selection.w, viewport.w));
    float h = floorf(fminf(selection.h, viewport.h));
    float x = fmaxf(selection.x, fminf(selection.x + selection.w - w, floorf(cx - w * .5f)));
    float y = fmaxf(selection.y, fminf(selection.y + selection.h - h, floorf(cy - h * .5f)));
    *result = (VePreviewWindow){.source = {x, y, w, h},
                                .destination = {floorf(viewport.x + (viewport.w - w) * .5f),
                                                floorf(viewport.y + (viewport.h - h) * .5f), w, h}};
    return true;
}
