/** @file @brief SDL-free UTF-8 editing, cursor and selection; native UI is in text_input_sdl.c. */
#include "pengui/pengui.h"
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

static void normalize(PUITextEdit *edit, const char *text) {
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

void pui_text_edit_init(PUITextEdit *edit, const char *text, bool select_all) {
    *edit = (PUITextEdit){strlen(text), select_all ? 0 : strlen(text), 0};
}

bool pui_text_edit_insert(PUITextEdit *edit, char *text, size_t capacity, const char *inserted) {
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

bool pui_text_edit_key(PUITextEdit *edit, char *text, PUITextKey key, bool shift) {
    normalize(edit, text);
    size_t a = edit->cursor < edit->anchor ? edit->cursor : edit->anchor;
    size_t b = edit->cursor > edit->anchor ? edit->cursor : edit->anchor;
    if (key == PUI_TEXT_SELECT_ALL) {
        edit->anchor = 0;
        edit->cursor = strlen(text);
        return false;
    }
    if (key == PUI_TEXT_BACKSPACE || key == PUI_TEXT_DELETE) {
        if (a == b) {
            if (key == PUI_TEXT_BACKSPACE && a) {
                a = boundary(text, a - 1);
            } else if (key == PUI_TEXT_DELETE) {
                b = next(text, b);
            }
        }
        memmove(text + a, text + b, strlen(text + b) + 1);
        edit->cursor = edit->anchor = a;
        return a != b;
    }
    if (key == PUI_TEXT_LEFT) {
        edit->cursor = !shift && a != b ? a : edit->cursor ? boundary(text, edit->cursor - 1) : 0;
    } else if (key == PUI_TEXT_RIGHT) {
        edit->cursor = !shift && a != b ? b : next(text, edit->cursor);
    } else if (key == PUI_TEXT_HOME) {
        edit->cursor = 0;
    } else if (key == PUI_TEXT_END) {
        edit->cursor = strlen(text);
    }
    if (!shift) {
        edit->anchor = edit->cursor;
    }
    return false;
}

void pui_text_edit_click(PUITextEdit *edit, const char *text, size_t column, bool shift) {
    size_t pos = 0;
    while (column-- && text[pos]) {
        pos = next(text, pos);
    }
    edit->cursor = pos;
    if (!shift) {
        edit->anchor = pos;
    }
}

size_t pui_text_edit_column(const char *text, size_t offset) {
    offset = boundary(text, offset);
    size_t column = 0;
    for (size_t i = 0; i < offset; i = next(text, i)) {
        column++;
    }
    return column;
}

static int word_class(unsigned char c) {
    if (c == ' ') {
        return 0;
    }
    return c >= 128 || (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9') ||
                   c == '_'
               ? 1
               : 2;
}

void pui_text_edit_select_word(PUITextEdit *edit, const char *text, size_t column) {
    pui_text_edit_click(edit, text, column, false);
    size_t pos = edit->cursor;
    if (!*text) {
        return;
    }
    if (!text[pos]) {
        pos = boundary(text, pos - 1);
    }
    int kind = word_class((unsigned char)text[pos]);
    size_t start = pos, end = next(text, pos);
    while (start) {
        size_t previous = boundary(text, start - 1);
        if (word_class((unsigned char)text[previous]) != kind) {
            break;
        }
        start = previous;
    }
    while (text[end] && word_class((unsigned char)text[end]) == kind) {
        end = next(text, end);
    }
    edit->anchor = start;
    edit->cursor = end;
}
