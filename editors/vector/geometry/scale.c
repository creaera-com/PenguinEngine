/** @file @brief Affine axis-aligned scaling without document or UI dependencies. */
#include "scale.h"

PUIRect vector_scale_rect(PUIRect rect, PUIRect source, PUIRect target) {
    if (source.w <= 0 || source.h <= 0) {
        return rect;
    }
    float sx = target.w / source.w, sy = target.h / source.h;
    return (PUIRect){target.x + (rect.x - source.x) * sx, target.y + (rect.y - source.y) * sy,
                     rect.w * sx, rect.h * sy};
}
