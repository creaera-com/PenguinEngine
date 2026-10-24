/** @file @brief Text values and commands do not depend on font rendering or native input. */
#include "text.h"
#include "editors/vector/object/commands.h"
#include <math.h>
#include <string.h>

bool vector_text_shape(PUIRect rect, VectorShape *shape) {
    if (!shape) {
        return false;
    }
    VectorShape s = {
        .id = 1,
        .parent_id = 1,
        .rect = rect,
        .fill = 0x242424ff,
        .text = {.enabled = 1, .size = 24, .line_height = 1.2f, .columns = 1, .column_gap = 16}};
    snprintf(s.name, sizeof s.name, "Text");
    snprintf(s.text.content, sizeof s.text.content, "Text");
    if (!vector_rectangle_valid(s)) {
        return false;
    }
    s.id = s.parent_id = 0;
    *shape = s;
    return true;
}

bool vector_text_add(VectorDocument *d, unsigned parent, PUIRect rect) {
    VectorShape s;
    if (!vector_text_shape(rect, &s)) {
        return false;
    }
    s.parent_id = parent;
    snprintf(s.name, sizeof s.name, "Text %u", d->next_rectangle_id);
    return vector_shape_add(d, &s);
}

bool vector_text_update(VectorDocument *d, unsigned id, const VectorText *text) {
    VectorShape *s = vector_rectangle_find(d, id);
    if (!s || !s->text.enabled || !text || !text->enabled) {
        return false;
    }
    VectorShape after = *s;
    after.text = *text;
    return vector_shape_update(d, &after);
}
