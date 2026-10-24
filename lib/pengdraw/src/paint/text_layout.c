/** @file @brief Greedy word wrapping and paragraph alignment without rendering resources. */
#include "pengdraw/pengdraw.h"
#include <math.h>
#include <string.h>

unsigned pdraw_text_codepoint(const char *text, size_t *offset) {
    const unsigned char *p = (const unsigned char *)text;
    unsigned c = p[(*offset)++];
    if (c < 128) {
        return c;
    }
    int n = c < 224 ? 1 : c < 240 ? 2 : 3;
    c &= (1u << (6 - n)) - 1;
    for (int i = 0; i < n; i++) {
        c = c * 64 + (p[(*offset)++] & 63);
    }
    return c;
}

static float step(const PDrawText *t, const char *s, size_t at, PDrawTextAdvance advance,
                  void *ctx) {
    unsigned code = pdraw_text_codepoint(s, &at), next = 0;
    if (s[at] && s[at] != '\n') {
        next = pdraw_text_codepoint(s, &at);
    }
    return fmaxf(.1f,
                 advance(ctx, code == '\t' ? ' ' : code, next, t->size) * (code == '\t' ? 4 : 1) +
                     t->letter_spacing);
}

void pdraw_text_layout(const PDrawText *t, float width, float height, PDrawTextAdvance advance,
                       void *ctx, PDrawTextLayout *out) {
    if (!out) {
        return;
    }
    *out = (PDrawTextLayout){0};
    if (!pdraw_text_valid(t) || !t->enabled || !advance || width <= 0 || height <= 0) {
        return;
    }
    float col_width = (width - (t->columns - 1) * t->column_gap) / t->columns;
    if (col_width <= 0) {
        out->overflow = *t->content != 0;
        return;
    }
    const char *s = t->content;
    size_t pos = 0;
    float y = 0;
    unsigned column = 0;
    while (s[pos]) {
        if (y + t->size > height + .001f) {
            column++;
            y = 0;
        }
        if (column >= t->columns) {
            out->overflow = true;
            break;
        }
        size_t start = pos, end = pos, last_space = (size_t)-1;
        float used = 0;
        while (s[end] && s[end] != '\n') {
            float w = step(t, s, end, advance, ctx);
            if (used + w > col_width && end > start) {
                break;
            }
            if (s[end] == ' ' || s[end] == '\t') {
                last_space = end;
            }
            pdraw_text_codepoint(s, &end);
            used += w;
        }
        bool paragraph_end = !s[end] || s[end] == '\n';
        if (!paragraph_end && last_space != (size_t)-1 && last_space > start) {
            end = last_space;
        }
        size_t visible = end;
        while (visible > start && (s[visible - 1] == ' ' || s[visible - 1] == '\t')) {
            visible--;
        }
        float line_width = 0;
        int spaces = 0;
        for (size_t at = start; at < visible;) {
            line_width += step(t, s, at, advance, ctx);
            spaces += s[at] == ' ';
            pdraw_text_codepoint(s, &at);
        }
        float x = column * (col_width + t->column_gap);
        if (t->alignment == PDRAW_TEXT_CENTER) {
            x += fmaxf(0, col_width - line_width) / 2;
        }
        if (t->alignment == PDRAW_TEXT_RIGHT) {
            x += fmaxf(0, col_width - line_width);
        }
        float extra = t->alignment == PDRAW_TEXT_JUSTIFY && !paragraph_end && spaces
                          ? fmaxf(0, col_width - line_width) / spaces
                          : 0;
        for (size_t at = start; at < end;) {
            size_t byte = at;
            unsigned code = pdraw_text_codepoint(s, &at);
            float w = step(t, s, byte, advance, ctx) + (code == ' ' ? extra : 0);
            out->glyphs[out->count++] = (PDrawTextGlyph){code, byte, at, x, y, w};
            x += w;
        }
        out->end_x = x;
        out->end_y = y;
        pos = end;
        if (s[pos] == '\n') {
            // Keep explicit breaks addressable by caret navigation and mouse selection.
            out->glyphs[out->count++] = (PDrawTextGlyph){'\n', pos, pos + 1, x, y, 0};
            pos++;
        } else {
            while (s[pos] == ' ' || s[pos] == '\t') {
                pos++;
            }
        }
        out->visible_end = pos;
        y += t->size * t->line_height;
        if (!s[pos] && pos && s[pos - 1] == '\n') {
            out->end_x = column * (col_width + t->column_gap);
            out->end_y = y;
        }
    }
}
