/** @file @brief Text values and commands do not depend on font rendering or native input. */
#include "text.h"
#include "editors/vector/object/commands.h"
#include <math.h>
#include <string.h>

bool vector_text_valid(const VectorText *t) {
    if (t->enabled > 1) {
        return false;
    }
    if (!t->enabled) {
        return true;
    }
    if (!memchr(t->content, 0, sizeof t->content) || !memchr(t->font, 0, sizeof t->font) ||
        t->face > 255 || t->bold > 1 || t->italic > 1 || t->alignment > VECTOR_TEXT_JUSTIFY ||
        t->columns < 1 || t->columns > 16 || !isfinite(t->size) || t->size < 1 || t->size > 1024 ||
        !isfinite(t->line_height) || t->line_height < .5f || t->line_height > 5 ||
        !isfinite(t->letter_spacing) || t->letter_spacing < -100 || t->letter_spacing > 1000 ||
        !isfinite(t->column_gap) || t->column_gap < 0 || t->column_gap > 10000) {
        return false;
    }
    // Validate UTF-8 explicitly, permitting paragraph breaks and tabs.
    const unsigned char *p = (const unsigned char *)t->content;
    while (*p) {
        unsigned c = *p++;
        if (c == '\n' || c == '\t') {
            continue;
        }
        if (c < 32 || c == 127) {
            return false;
        }
        if (c < 128) {
            continue;
        }
        int n = c >= 0xc2 && c <= 0xdf   ? 1
                : c >= 0xe0 && c <= 0xef ? 2
                : c >= 0xf0 && c <= 0xf4 ? 3
                                         : 0;
        if (!n) {
            return false;
        }
        unsigned value = c & ((1u << (6 - n)) - 1);
        for (int i = 0; i < n; i++) {
            if ((*p & 0xc0) != 0x80) {
                return false;
            }
            value = value * 64 + (*p++ & 63);
        }
        if ((n == 2 && value < 0x800) || (n == 3 && value < 0x10000) || value > 0x10ffff ||
            (value >= 0xd800 && value <= 0xdfff)) {
            return false;
        }
    }
    return true;
}

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
