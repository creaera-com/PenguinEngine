#include "pengdraw/pengdraw.h"
#include <math.h>
#include <string.h>

bool pdraw_text_valid(const PDrawText *t) {
    if (!t || t->enabled > 1) {
        return false;
    }
    if (!t->enabled) {
        return true;
    }
    if (!memchr(t->content, 0, sizeof t->content) || !memchr(t->font, 0, sizeof t->font) ||
        t->face > 255 || t->bold > 1 || t->italic > 1 || t->alignment > PDRAW_TEXT_JUSTIFY ||
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
