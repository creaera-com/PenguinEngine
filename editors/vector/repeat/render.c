/** @file @brief Reuse the source's live display values without modifying document/selection. */
#include "render.h"
#include "editors/vector/curve/tool.h"
#include "editors/vector/gradient/tool.h"
#include "editors/vector/workspace/selection.h"
#include "repeat.h"

void vector_repeat_render(const VectorDocument *d, const VectorShape *source,
                          void (*paint)(void *, VectorShape *), void *context) {
    if (!source->repeat.enabled) {
        return;
    }
    VectorShape base = vector_curve_display_shape(d, source);
    base.gradient = *vector_gradient_display(d, source);
    base.rect = vector_rectangle_display_rect(d, &base);
    base.angle = vector_rectangle_display_angle(d, &base);
    if (d->fill_preview.active && d->fill_preview.rectangle_id == source->id) {
        base.fill = d->fill_preview.color;
        base.gradient.kind = VECTOR_GRADIENT_SOLID;
    }
    for (unsigned y = 0; y < base.repeat.rows; y++) {
        for (unsigned x = 0; x < base.repeat.columns; x++) {
            if (!x && !y) {
                continue;
            }
            VectorShape instance;
            if (vector_repeat_instance(&base, &base.repeat, x, y, &instance)) {
                instance.id = 0; // No caret, selection outline or live transform is applied twice.
                paint(context, &instance);
            }
        }
    }
}
