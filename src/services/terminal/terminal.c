#include "services/terminal/terminal.h"
#include "vterm.h"
#include <stdlib.h>
#include <string.h>

typedef struct {
    VTermScreenCell *cells;
    int cols;
} Line;

struct PeTerminal {
    VTerm *vt;
    VTermScreen *screen;
    int rows, cols, count, head, scroll;
    Line history[PE_TERMINAL_HISTORY];
    char output[PE_TERMINAL_OUTPUT];
    size_t output_length;
    bool overflow, cursor_visible, alternate;
};

static void output(const char *bytes, size_t length, void *user) {
    PeTerminal *t = user;
    if (length > sizeof t->output - t->output_length) {
        t->overflow = true;
        return;
    }
    memcpy(t->output + t->output_length, bytes, length);
    t->output_length += length;
}

static int pushline(int cols, const VTermScreenCell *cells, void *user) {
    PeTerminal *t = user;
    if (cols < 1 || cols > 240) {
        return 0;
    }
    VTermScreenCell *copy = malloc((size_t)cols * sizeof *copy);
    if (!copy) {
        return 0;
    }
    memcpy(copy, cells, (size_t)cols * sizeof *copy);
    if (t->count == PE_TERMINAL_HISTORY) {
        free(t->history[t->head].cells);
        t->head = (t->head + 1) % PE_TERMINAL_HISTORY;
        t->count--;
    }
    t->history[(t->head + t->count++) % PE_TERMINAL_HISTORY] = (Line){copy, cols};
    if (t->scroll && t->scroll < t->count) {
        t->scroll++;
    }
    return 1;
}

static int popline(int cols, VTermScreenCell *cells, void *user) {
    PeTerminal *t = user;
    if (!t->count) {
        return 0;
    }
    Line *line = &t->history[(t->head + --t->count) % PE_TERMINAL_HISTORY];
    memset(cells, 0, (size_t)cols * sizeof *cells);
    for (int col = 0; col < cols; col++) {
        cells[col].width = 1;
        vterm_color_rgb(&cells[col].fg, 233, 233, 233);
        vterm_color_rgb(&cells[col].bg, 23, 23, 23);
    }
    int n = line->cols < cols ? line->cols : cols;
    memcpy(cells, line->cells, (size_t)n * sizeof *cells);
    free(line->cells);
    line->cells = NULL;
    if (t->scroll > t->count) {
        t->scroll = t->count;
    }
    return 1;
}

static int clear_history(void *user) {
    PeTerminal *t = user;
    while (t->count) {
        Line *line = &t->history[(t->head + --t->count) % PE_TERMINAL_HISTORY];
        free(line->cells);
        line->cells = NULL;
    }
    t->head = t->scroll = 0;
    return 1;
}

static int property(VTermProp prop, VTermValue *value, void *user) {
    PeTerminal *t = user;
    if (prop == VTERM_PROP_CURSORVISIBLE) {
        t->cursor_visible = value->boolean != 0;
    }
    if (prop == VTERM_PROP_ALTSCREEN) {
        t->alternate = value->boolean != 0;
        t->scroll = 0;
    }
    return 1;
}

static const VTermScreenCallbacks callbacks = {.settermprop = property,
                                               .sb_pushline = pushline,
                                               .sb_popline = popline,
                                               .sb_clear = clear_history};

PeTerminal *pe_terminal_new(int rows, int cols) {
    if (rows < 1 || rows > 100 || cols < 2 || cols > 240) {
        return NULL;
    }
    PeTerminal *t = calloc(1, sizeof *t);
    if (!t) {
        return NULL;
    }
    t->vt = vterm_new(rows, cols);
    if (!t->vt) {
        free(t);
        return NULL;
    }
    t->rows = rows;
    t->cols = cols;
    t->cursor_visible = true;
    vterm_set_utf8(t->vt, 1);
    vterm_output_set_callback(t->vt, output, t);
    t->screen = vterm_obtain_screen(t->vt);
    vterm_screen_set_callbacks(t->screen, &callbacks, t);
    vterm_screen_enable_altscreen(t->screen, 1);
    VTermColor fg, bg;
    vterm_color_rgb(&fg, 233, 233, 233);
    vterm_color_rgb(&bg, 23, 23, 23);
    vterm_screen_set_default_colors(t->screen, &fg, &bg);
    const uint32_t palette[16] = {0x171717, 0xcf7777, 0x9bbd87, 0xcbb583, 0x7f9fc7, 0xb49acb,
                                  0x7db8ba, 0xc7c7c7, 0x777777, 0xe79696, 0xb5d6a0, 0xe0cba1,
                                  0x9cbbe3, 0xc9b0df, 0x9bd2d2, 0xe9e9e9};
    for (int i = 0; i < 16; i++) {
        VTermColor color;
        vterm_color_rgb(&color, (uint8_t)(palette[i] >> 16), (uint8_t)(palette[i] >> 8),
                        (uint8_t)palette[i]);
        vterm_state_set_palette_color(vterm_obtain_state(t->vt), i, &color);
    }
    vterm_screen_reset(t->screen, 1);
    return t;
}

void pe_terminal_free(PeTerminal *t) {
    if (!t) {
        return;
    }
    clear_history(t);
    vterm_free(t->vt);
    free(t);
}

void pe_terminal_feed(PeTerminal *t, const char *bytes, size_t length) {
    vterm_input_write(t->vt, bytes, length);
    vterm_screen_flush_damage(t->screen);
}

bool pe_terminal_resize(PeTerminal *t, int rows, int cols) {
    if (rows < 1 || rows > 100 || cols < 2 || cols > 240) {
        return false;
    }
    if (rows != t->rows || cols != t->cols) {
        vterm_set_size(t->vt, rows, cols);
        t->rows = rows;
        t->cols = cols;
        vterm_screen_flush_damage(t->screen);
    }
    return true;
}

int pe_terminal_rows(const PeTerminal *t) {
    return t->rows;
}

int pe_terminal_cols(const PeTerminal *t) {
    return t->cols;
}

int pe_terminal_history(const PeTerminal *t) {
    return t->alternate ? 0 : t->count;
}

int pe_terminal_scroll_offset(const PeTerminal *t) {
    return t->scroll;
}

static uint32_t rgba(PeTerminal *t, VTermColor color) {
    vterm_screen_convert_color_to_rgb(t->screen, &color);
    return ((uint32_t)color.rgb.red << 24) | ((uint32_t)color.rgb.green << 16) |
           ((uint32_t)color.rgb.blue << 8) | 255u;
}

bool pe_terminal_cell(PeTerminal *t, int row, int col, PeTerminalCell *cell) {
    if (row < 0 || row >= t->rows || col < 0 || col >= t->cols) {
        return false;
    }
    VTermScreenCell raw = {0};
    int history_row = t->count - t->scroll + row;
    if (history_row < t->count) {
        Line *line = &t->history[(t->head + history_row) % PE_TERMINAL_HISTORY];
        if (col < line->cols) {
            raw = line->cells[col];
        } else {
            raw.width = 1;
            vterm_color_rgb(&raw.fg, 233, 233, 233);
            vterm_color_rgb(&raw.bg, 23, 23, 23);
        }
    } else if (!vterm_screen_get_cell(t->screen, (VTermPos){row - t->scroll, col}, &raw)) {
        return false;
    }
    memset(cell, 0, sizeof *cell);
    memcpy(cell->chars, raw.chars, sizeof cell->chars);
    cell->width = raw.chars[0] == UINT32_MAX ? 0 : raw.width;
    if (!cell->width) {
        memset(cell->chars, 0, sizeof cell->chars);
    }
    cell->fg = rgba(t, raw.attrs.reverse ? raw.bg : raw.fg);
    cell->bg = rgba(t, raw.attrs.reverse ? raw.fg : raw.bg);
    cell->bold = raw.attrs.bold;
    cell->underline = raw.attrs.underline != 0;
    if (raw.attrs.conceal) {
        memset(cell->chars, 0, sizeof cell->chars);
    }
    return true;
}

bool pe_terminal_cursor(PeTerminal *t, int *row, int *col) {
    VTermPos pos;
    vterm_state_get_cursorpos(vterm_obtain_state(t->vt), &pos);
    *row = pos.row;
    *col = pos.col;
    return t->cursor_visible && !t->scroll;
}

void pe_terminal_scroll(PeTerminal *t, int lines) {
    if (t->alternate) {
        t->scroll = 0;
        return;
    }
    if (lines > 0 && lines > t->count - t->scroll) {
        t->scroll = t->count;
    } else if (lines < 0 && lines < -t->scroll) {
        t->scroll = 0;
    } else {
        t->scroll += lines;
    }
}

void pe_terminal_key(PeTerminal *t, PeTerminalKey key, unsigned modifiers) {
    static const VTermKey keys[] = {
        VTERM_KEY_ENTER, VTERM_KEY_TAB,  VTERM_KEY_BACKSPACE, VTERM_KEY_ESCAPE,  VTERM_KEY_UP,
        VTERM_KEY_DOWN,  VTERM_KEY_LEFT, VTERM_KEY_RIGHT,     VTERM_KEY_INS,     VTERM_KEY_DEL,
        VTERM_KEY_HOME,  VTERM_KEY_END,  VTERM_KEY_PAGEUP,    VTERM_KEY_PAGEDOWN};
    VTermKey k = key >= PE_TERM_F1 && key < PE_TERM_F1 + 12
                     ? VTERM_KEY_FUNCTION(key - PE_TERM_F1 + 1)
                 : key >= 0 && key < PE_TERM_F1 ? keys[key]
                                                : VTERM_KEY_NONE;
    t->scroll = 0;
    vterm_keyboard_key(t->vt, k, (VTermModifier)(modifiers & 7));
}

void pe_terminal_unichar(PeTerminal *t, uint32_t c, unsigned modifiers) {
    t->scroll = 0;
    vterm_keyboard_unichar(t->vt, c, (VTermModifier)(modifiers & 7));
}

bool pe_terminal_paste(PeTerminal *t, const char *text, bool bracketed) {
    size_t length = strlen(text);
    if (length > PE_TERMINAL_OUTPUT || length + 12 > sizeof t->output - t->output_length) {
        return false;
    }
    t->scroll = 0;
    if (bracketed) {
        vterm_keyboard_start_paste(t->vt);
    }
    output(text, length, t);
    if (bracketed) {
        vterm_keyboard_end_paste(t->vt);
    }
    return true;
}

const char *pe_terminal_output(const PeTerminal *t, size_t *length) {
    *length = t->output_length;
    return t->output;
}

void pe_terminal_consume(PeTerminal *t, size_t length) {
    if (length > t->output_length) {
        length = t->output_length;
    }
    memmove(t->output, t->output + length, t->output_length - length);
    t->output_length -= length;
}

bool pe_terminal_overflow(PeTerminal *t) {
    bool result = t->overflow;
    t->overflow = false;
    return result;
}

size_t pe_terminal_utf8(uint32_t c, char *out) {
    if (c > 0x10ffff || (c >= 0xd800 && c <= 0xdfff)) {
        c = '?';
    }
    if (c < 0x80) {
        out[0] = (char)c;
        return 1;
    }
    if (c < 0x800) {
        out[0] = (char)(0xc0 | (c >> 6));
        out[1] = (char)(0x80 | (c & 63));
        return 2;
    }
    if (c < 0x10000) {
        out[0] = (char)(0xe0 | (c >> 12));
        out[1] = (char)(0x80 | ((c >> 6) & 63));
        out[2] = (char)(0x80 | (c & 63));
        return 3;
    }
    if (c <= 0x10ffff) {
        out[0] = (char)(0xf0 | (c >> 18));
        out[1] = (char)(0x80 | ((c >> 12) & 63));
        out[2] = (char)(0x80 | ((c >> 6) & 63));
        out[3] = (char)(0x80 | (c & 63));
        return 4;
    }
    out[0] = '?';
    return 1;
}

char *pe_terminal_copy(PeTerminal *t, int row0, int col0, int row1, int col1) {
    if (row0 > row1 || (row0 == row1 && col0 > col1)) {
        int r = row0, c = col0;
        row0 = row1;
        col0 = col1;
        row1 = r;
        col1 = c;
    }
    if (row0 < 0 || row1 >= t->rows || col0 < 0 || col1 < 0 || col0 >= t->cols || col1 >= t->cols) {
        return NULL;
    }
    size_t capacity = (size_t)(row1 - row0 + 1) * ((size_t)t->cols * 24 + 1) + 1, n = 0;
    char *result = malloc(capacity);
    if (!result) {
        return NULL;
    }
    for (int row = row0; row <= row1; row++) {
        size_t line_start = n;
        for (int col = row == row0 ? col0 : 0; col <= (row == row1 ? col1 : t->cols - 1); col++) {
            PeTerminalCell cell;
            if (!pe_terminal_cell(t, row, col, &cell)) {
                continue;
            }
            if (cell.width == 0) {
                continue;
            }
            if (!cell.chars[0]) {
                result[n++] = ' ';
            }
            for (int i = 0; i < 6 && cell.chars[i]; i++) {
                n += pe_terminal_utf8(cell.chars[i], result + n);
            }
        }
        while (n > line_start && result[n - 1] == ' ') {
            n--;
        }
        if (row < row1) {
            result[n++] = '\n';
        }
    }
    result[n] = 0;
    return result;
}
