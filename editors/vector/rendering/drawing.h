/** @file @brief Translate editor shape values to pengDraw without exposing document identity. */
#ifndef PE_VECTOR_DRAWING_H
#define PE_VECTOR_DRAWING_H
#include "editors/vector/object/shape.h"

/** @brief Copy only paint geometry and appearance; no IDs, history, tools or asset paths. */
static inline PDrawShape vector_shape_drawing(const VectorShape *s) {
    return (PDrawShape){
        .path = s->path,
        .curve = s->curve,
        .corner_radius = s->text.enabled || *s->image.source ? 0 : s->corner_radius,
        .stroke = {s->stroke.color, s->stroke.width, (PDrawStrokeAlignment)s->stroke.alignment},
        .gradient = s->gradient,
        .ellipse = s->curve_kind == VECTOR_CURVE_ELLIPSE && s->ellipse.mode == VECTOR_ELLIPSE_FULL};
}

/** @brief Explicit conversion keeps pengUI out of pengDraw's public contract. */
static inline PDrawRect vector_draw_rect(PUIRect r) {
    return (PDrawRect){r.x, r.y, r.w, r.h};
}
#endif
