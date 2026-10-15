#include "penguin/script.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#ifdef _WIN32
#include <windows.h>
#endif
static bool utf8(const char *s, size_t n) {
    for (size_t i = 0; i < n;) {
        unsigned char c = (unsigned char)s[i++];
        if (!c)
            return false;
        if (c < 128)
            continue;
        int count;
        uint32_t value, min;
        if (c >= 0xc2 && c <= 0xdf) {
            count = 1;
            value = c & 31;
            min = 128;
        } else if (c >= 0xe0 && c <= 0xef) {
            count = 2;
            value = c & 15;
            min = 2048;
        } else if (c >= 0xf0 && c <= 0xf4) {
            count = 3;
            value = c & 7;
            min = 65536;
        } else
            return false;
        if (i + (size_t)count > n)
            return false;
        for (int k = 0; k < count; k++) {
            c = (unsigned char)s[i++];
            if ((c & 0xc0) != 0x80)
                return false;
            value = (value << 6) | (c & 63);
        }
        if (value < min || value > 0x10ffff || (value >= 0xd800 && value <= 0xdfff))
            return false;
    }
    return true;
}
static bool boundary(const PeText *d, size_t n) {
    return n <= d->length && (n == d->length || ((unsigned char)d->text[n] & 0xc0) != 0x80);
}
bool pe_text_init(PeText *d, const char *source) {
    size_t n = strlen(source);
    if (n > PE_TEXT_MAX || !utf8(source, n))
        return false;
    memset(d, 0, sizeof *d);
    memcpy(d->text, source, n + 1);
    d->length = n;
    d->next_revision = 1;
    return true;
}
static void edit_free(PeTextEdit *e) {
    free(e->before);
    free(e->after);
    memset(e, 0, sizeof *e);
}
void pe_text_free(PeText *d) {
    for (int i = 0; i < d->count; i++)
        edit_free(&d->edits[i]);
    memset(d, 0, sizeof *d);
}
bool pe_text_replace(PeText *d, size_t start, size_t end, const char *s, size_t n) {
    if (start > end || !boundary(d, start) || !boundary(d, end) || n > PE_TEXT_MAX || !utf8(s, n))
        return false;
    size_t length = d->length - (end - start) + n;
    if (length > PE_TEXT_MAX)
        return false;
    if (end - start == n && !memcmp(d->text + start, s, n)) {
        d->cursor = d->anchor = start + n;
        return true;
    }
    char *before = malloc(d->length + 1), *after = malloc(length + 1);
    if (!before || !after) {
        free(before);
        free(after);
        return false;
    }
    memcpy(before, d->text, d->length + 1);
    memcpy(after, d->text, start);
    memcpy(after + start, s, n);
    memcpy(after + start + n, d->text + end, d->length - end + 1);
    for (int i = d->position; i < d->count; i++)
        edit_free(&d->edits[i]);
    d->count = d->position;
    if (d->count == PE_TEXT_HISTORY) {
        edit_free(&d->edits[0]);
        memmove(d->edits, d->edits + 1, (PE_TEXT_HISTORY - 1) * sizeof(PeTextEdit));
        d->count--;
        d->position--;
    }
    PeTextEdit *edit = &d->edits[d->count++];
    *edit = (PeTextEdit){before,    after,       d->cursor,         d->anchor,
                         start + n, d->revision, d->next_revision++};
    memcpy(d->text, after, length + 1);
    d->length = length;
    d->cursor = d->anchor = start + n;
    d->position = d->count;
    d->revision = edit->after_revision;
    return true;
}
bool pe_text_insert(PeText *d, const char *s) {
    size_t start = d->cursor < d->anchor ? d->cursor : d->anchor,
           end = d->cursor > d->anchor ? d->cursor : d->anchor;
    return pe_text_replace(d, start, end, s, strlen(s));
}
size_t pe_text_previous(const PeText *d, size_t p) {
    if (p > d->length)
        p = d->length;
    if (p) {
        do {
            p--;
        } while (p && ((unsigned char)d->text[p] & 0xc0) == 0x80);
    }
    return p;
}
size_t pe_text_next(const PeText *d, size_t p) {
    if (p > d->length)
        p = d->length;
    if (p < d->length) {
        do {
            p++;
        } while (p < d->length && ((unsigned char)d->text[p] & 0xc0) == 0x80);
    }
    return p;
}
bool pe_text_backspace(PeText *d) {
    if (d->cursor != d->anchor)
        return pe_text_insert(d, "");
    if (!d->cursor)
        return true;
    return pe_text_replace(d, pe_text_previous(d, d->cursor), d->cursor, "", 0);
}
bool pe_text_delete(PeText *d) {
    if (d->cursor != d->anchor)
        return pe_text_insert(d, "");
    return pe_text_replace(d, d->cursor, pe_text_next(d, d->cursor), "", 0);
}
bool pe_text_undo(PeText *d) {
    if (!d->position)
        return false;
    PeTextEdit *e = &d->edits[--d->position];
    d->length = strlen(e->before);
    memcpy(d->text, e->before, d->length + 1);
    d->cursor = e->before_cursor;
    d->anchor = e->before_anchor;
    d->revision = e->before_revision;
    return true;
}
bool pe_text_redo(PeText *d) {
    if (d->position == d->count)
        return false;
    PeTextEdit *e = &d->edits[d->position++];
    d->length = strlen(e->after);
    memcpy(d->text, e->after, d->length + 1);
    d->cursor = d->anchor = e->after_cursor;
    d->revision = e->after_revision;
    return true;
}
bool pe_text_dirty(const PeText *d) {
    return d->revision != d->saved_revision;
}
void pe_text_saved(PeText *d) {
    d->saved_revision = d->revision;
}
size_t pe_text_line_start(const PeText *d, size_t p) {
    if (p > d->length)
        p = d->length;
    while (p && d->text[p - 1] != '\n')
        p--;
    return p;
}
size_t pe_text_line_end(const PeText *d, size_t p) {
    if (p > d->length)
        p = d->length;
    while (p < d->length && d->text[p] != '\n')
        p++;
    return p;
}
void pe_text_location(const PeText *d, size_t p, int *line, int *column) {
    *line = 0;
    *column = 0;
    if (p > d->length)
        p = d->length;
    for (size_t i = 0; i < p; i = pe_text_next(d, i)) {
        if (d->text[i] == '\n') {
            (*line)++;
            *column = 0;
        } else if (d->text[i] == '\t')
            *column += 4 - *column % 4;
        else
            (*column)++;
    }
}
size_t pe_text_offset(const PeText *d, int line, int column) {
    if (line < 0)
        line = 0;
    if (column < 0)
        column = 0;
    size_t p = 0;
    int l = 0;
    while (p < d->length && l < line) {
        if (d->text[p++] == '\n')
            l++;
    }
    int col = 0;
    while (p < d->length && d->text[p] != '\n' && col < column) {
        int width = d->text[p] == '\t' ? 4 - col % 4 : 1;
        if (col + width > column)
            break;
        col += width;
        p = pe_text_next(d, p);
    }
    return p;
}
bool pe_text_load(PeText *d, const char *path, char *error, size_t size) {
    FILE *f = fopen(path, "rb");
    if (!f) {
        snprintf(error, size, "Cannot open %.180s", path);
        return false;
    }
    char *source = malloc(PE_TEXT_MAX + 2);
    if (!source) {
        fclose(f);
        snprintf(error, size, "Out of memory");
        return false;
    }
    size_t n = fread(source, 1, PE_TEXT_MAX + 1, f);
    bool ok = !ferror(f) && n <= PE_TEXT_MAX;
    fclose(f);
    if (ok)
        ok = utf8(source, n);
    if (!ok) {
        free(source);
        snprintf(error, size, "Script must be UTF-8, without NUL, max 64 KiB");
        return false;
    }
    size_t out = 0;
    for (size_t i = 0; i < n; i++) {
        if (source[i] == '\r') {
            source[out++] = '\n';
            if (i + 1 < n && source[i + 1] == '\n')
                i++;
        } else
            source[out++] = source[i];
    }
    source[out] = 0;
    pe_text_free(d);
    ok = pe_text_init(d, source);
    free(source);
    return ok;
}
bool pe_text_save(PeText *d, const char *path, char *error, size_t size) {
    char temp[1024];
    if (snprintf(temp, sizeof temp, "%s.tmp", path) >= (int)sizeof temp) {
        snprintf(error, size, "Script path too long");
        return false;
    }
    FILE *f = fopen(temp, "wb");
    if (!f) {
        snprintf(error, size, "Cannot write %.180s", temp);
        return false;
    }
    bool ok = fwrite(d->text, 1, d->length, f) == d->length;
    if (fclose(f))
        ok = false;
    if (ok) {
#ifdef _WIN32
        ok = MoveFileExA(temp, path, MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH) != 0;
#else
        ok = rename(temp, path) == 0;
#endif
    }
    if (!ok) {
        remove(temp);
        snprintf(error, size, "Script save failed; original file retained");
        return false;
    }
    pe_text_saved(d);
    return true;
}
