#include "penguin/ui.h"
#include <string.h>

static size_t boundary(const char *text, size_t pos) {
    size_t n = strlen(text);
    if (pos > n) {
        pos = n;
    }
    while (pos && ((unsigned char)text[pos] & 0xc0) == 0x80) {
        pos--;
    }
    return pos;
}

static size_t next(const char *text, size_t pos) {
    if (text[pos]) {
        pos++;
        while (((unsigned char)text[pos] & 0xc0) == 0x80) {
            pos++;
        }
    }
    return pos;
}

static void normalize(UiTextEdit *edit, const char *text) {
    edit->cursor = boundary(text, edit->cursor);
    edit->anchor = boundary(text, edit->anchor);
}

static bool single_line_utf8(const unsigned char *p) {
    while (*p) {
        unsigned first = *p++;
        if (first < 32 || first == 127) {
            return false;
        }
        if (first < 128) {
            continue;
        }
        int count = first >= 0xc2 && first <= 0xdf   ? 1
                    : first >= 0xe0 && first <= 0xef ? 2
                    : first >= 0xf0 && first <= 0xf4 ? 3
                                                     : 0;
        if (!count) {
            return false;
        }
        unsigned code = first & ((1u << (6 - count)) - 1);
        for (int i = 0; i < count; i++) {
            if ((*p & 0xc0) != 0x80) {
                return false;
            }
            code = (code << 6) | (*p++ & 63);
        }
        if ((count == 2 && code < 0x800) || (count == 3 && code < 0x10000) ||
            (code >= 0xd800 && code <= 0xdfff) || code > 0x10ffff) {
            return false;
        }
    }
    return true;
}

void ui_text_edit_init(UiTextEdit *edit, const char *text, bool select_all) {
    *edit = (UiTextEdit){strlen(text), select_all ? 0 : strlen(text), 0};
}

bool ui_text_edit_insert(UiTextEdit *edit, char *text, size_t capacity, const char *inserted) {
    normalize(edit, text);
    size_t a = edit->cursor < edit->anchor ? edit->cursor : edit->anchor;
    size_t b = edit->cursor > edit->anchor ? edit->cursor : edit->anchor;
    size_t n = strlen(text), added = strlen(inserted), remaining = n - (b - a);
    if (remaining >= capacity || added >= capacity - remaining ||
        !single_line_utf8((const unsigned char *)inserted)) {
        return false;
    }
    memmove(text + a + added, text + b, n - b + 1);
    memcpy(text + a, inserted, added);
    edit->cursor = edit->anchor = a + added;
    return true;
}

bool ui_text_edit_key(UiTextEdit *edit, char *text, UiTextKey key, bool shift) {
    normalize(edit, text);
    size_t a = edit->cursor < edit->anchor ? edit->cursor : edit->anchor;
    size_t b = edit->cursor > edit->anchor ? edit->cursor : edit->anchor;
    if (key == UI_TEXT_SELECT_ALL) {
        edit->anchor = 0;
        edit->cursor = strlen(text);
        return false;
    }
    if (key == UI_TEXT_BACKSPACE || key == UI_TEXT_DELETE) {
        if (a == b) {
            if (key == UI_TEXT_BACKSPACE && a) {
                a = boundary(text, a - 1);
            } else if (key == UI_TEXT_DELETE) {
                b = next(text, b);
            }
        }
        memmove(text + a, text + b, strlen(text + b) + 1);
        edit->cursor = edit->anchor = a;
        return a != b;
    }
    if (key == UI_TEXT_LEFT) {
        edit->cursor = !shift && a != b ? a : edit->cursor ? boundary(text, edit->cursor - 1) : 0;
    } else if (key == UI_TEXT_RIGHT) {
        edit->cursor = !shift && a != b ? b : next(text, edit->cursor);
    } else if (key == UI_TEXT_HOME) {
        edit->cursor = 0;
    } else if (key == UI_TEXT_END) {
        edit->cursor = strlen(text);
    }
    if (!shift) {
        edit->anchor = edit->cursor;
    }
    return false;
}

void ui_text_edit_click(UiTextEdit *edit, const char *text, size_t column, bool shift) {
    size_t pos = 0;
    while (column-- && text[pos]) {
        pos = next(text, pos);
    }
    edit->cursor = pos;
    if (!shift) {
        edit->anchor = pos;
    }
}

size_t ui_text_edit_column(const char *text, size_t offset) {
    offset = boundary(text, offset);
    size_t column = 0;
    for (size_t i = 0; i < offset; i = next(text, i)) {
        column++;
    }
    return column;
}
