#include "shell/console/view.h"
#include "platform/terminal/process.h"
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define CONSOLE_DOCUMENT 0x10010u
#define CONSOLE_RESTART 0x10011u

struct PeConsole {
    PeTerminal *terminal;
    PeTerminalProcess *process;
    char root[1024], error[256];
    SDL_FRect area, body;
    bool spawn, attempted, visible, enabled, focused, selecting, selection, suppress_text;
    int row0, col0, row1, col1;
};

PeConsole *pe_console_new(const char *root, bool spawn) {
    if (strlen(root) >= 1024) {
        return NULL;
    }
    PeConsole *c = calloc(1, sizeof *c);
    if (!c) {
        return NULL;
    }
    c->terminal = pe_terminal_new(8, 80);
    if (!c->terminal) {
        free(c);
        return NULL;
    }
    snprintf(c->root, sizeof c->root, "%s", root);
    c->spawn = spawn;
    return c;
}

void pe_console_free(PeConsole *c) {
    if (!c) {
        return;
    }
    pe_terminal_process_free(c->process);
    pe_terminal_free(c->terminal);
    free(c);
}

static void start(PeConsole *c) {
    pe_terminal_process_free(c->process);
    c->process = NULL;
    c->attempted = true;
    *c->error = 0;
    if (c->spawn) {
        c->process =
            pe_terminal_process_start(c->root, NULL, pe_terminal_rows(c->terminal),
                                      pe_terminal_cols(c->terminal), c->error, sizeof c->error);
    }
}

void pe_console_tick(PeConsole *c) {
    if (!c || !c->process) {
        return;
    }
    char buffer[4096];
    for (int i = 0; i < 16; i++) {
        int n = pe_terminal_process_read(c->process, buffer, sizeof buffer);
        if (n <= 0) {
            break;
        }
        pe_terminal_feed(c->terminal, buffer, (size_t)n);
    }
    for (int i = 0; i < 16; i++) {
        size_t length;
        const char *bytes = pe_terminal_output(c->terminal, &length);
        if (!length) {
            break;
        }
        int written = pe_terminal_process_write(c->process, bytes, length);
        if (written <= 0) {
            break;
        }
        pe_terminal_consume(c->terminal, (size_t)written);
    }
    if (pe_terminal_overflow(c->terminal)) {
        snprintf(c->error, sizeof c->error, "Terminal input buffer full");
    }
}

void pe_console_begin_frame(PeConsole *c) {
    if (c) {
        c->visible = false;
    }
}

bool pe_console_focused(const PeConsole *c) {
    return c && c->focused;
}

PeTerminal *pe_console_terminal(PeConsole *c) {
    return c->terminal;
}

SDL_FRect pe_console_area(const PeConsole *c) {
    return c->body;
}

static void fill(SDL_Renderer *r, SDL_FRect rect, uint32_t color) {
    SDL_SetRenderDrawColor(r, (Uint8)(color >> 24), (Uint8)(color >> 16), (Uint8)(color >> 8),
                           (Uint8)color);
    SDL_RenderFillRect(r, &rect);
}

static bool selected(PeConsole *c, int row, int col) {
    if (!c->selection) {
        return false;
    }
    int cols = pe_terminal_cols(c->terminal), a = c->row0 * cols + c->col0,
        b = c->row1 * cols + c->col1;
    if (a > b) {
        int swap = a;
        a = b;
        b = swap;
    }
    int position = row * cols + col;
    return position >= a && position <= b;
}

void pe_console_draw(PeConsole *c, SDL_Renderer *r, UiContext *ui, SDL_FRect area, bool enabled) {
    c->visible = true;
    c->enabled = enabled;
    c->area = area;
    c->body = (SDL_FRect){area.x + 8, area.y + 5, fmaxf(0, area.w - 30), fmaxf(0, area.h - 30)};
    int cols = (int)(c->body.w / UI_FONT_WIDTH), rows = (int)(c->body.h / UI_FONT_HEIGHT);
    cols = cols < 2 ? 2 : cols > 240 ? 240 : cols;
    rows = rows < 1 ? 1 : rows > 100 ? 100 : rows;
    if (cols != pe_terminal_cols(c->terminal) || rows != pe_terminal_rows(c->terminal)) {
        pe_terminal_resize(c->terminal, rows, cols);
        c->selection = c->selecting = false;
        if (c->process) {
            pe_terminal_process_resize(c->process, rows, cols);
        }
    }
    if (!c->attempted) {
        start(c);
    }
    fill(r, area, 0x171717ff);
    if (!enabled) {
        c->focused = c->selecting = false;
    }
    for (int row = 0; row < rows; row++) {
        for (int col = 0; col < cols; col++) {
            PeTerminalCell cell;
            if (!pe_terminal_cell(c->terminal, row, col, &cell)) {
                continue;
            }
            SDL_FRect rect = {c->body.x + col * UI_FONT_WIDTH, c->body.y + row * UI_FONT_HEIGHT,
                              UI_FONT_WIDTH, UI_FONT_HEIGHT};
            if (selected(c, row, col)) {
                fill(r, rect, UI_HOVER);
            } else if (cell.bg != 0x171717ff) {
                fill(r, rect, cell.bg);
            }
            if (!cell.chars[0] || cell.width == 0) {
                continue;
            }
            char text[25];
            size_t n = 0;
            for (int i = 0; i < 6 && cell.chars[i]; i++) {
                n += pe_terminal_utf8(cell.chars[i], text + n);
            }
            text[n] = 0;
            ui_text(r, rect.x, rect.y, text, cell.fg);
            if (cell.bold) {
                ui_text(r, rect.x + .5f, rect.y, text, cell.fg);
            }
            if (cell.underline) {
                fill(r, (SDL_FRect){rect.x, rect.y + UI_FONT_HEIGHT - 2, UI_FONT_WIDTH, 1},
                     cell.fg);
            }
        }
    }
    int history = pe_terminal_history(c->terminal);
    int offset = pe_terminal_scroll_offset(c->terminal);
    UiScroll scroll = {(float)((history - offset) * UI_FONT_HEIGHT),
                       (float)((history + rows) * UI_FONT_HEIGHT), (float)(rows * UI_FONT_HEIGHT)};
    UiRect track = {area.x + area.w - 12, c->body.y, 8, (float)(rows * UI_FONT_HEIGHT)};
    if (ui_scrollbar(ui, PE_CONSOLE_SCROLL_ID, track, &scroll, UI_AXIS_Y)) {
        int target = history - (int)(scroll.offset / UI_FONT_HEIGHT);
        pe_terminal_scroll(c->terminal, target - offset);
        c->selection = false;
    }
    ui_draw_scrollbar(r, ui, PE_CONSOLE_SCROLL_ID, track, &scroll, UI_AXIS_Y);
    int row, col;
    if (c->focused && pe_terminal_cursor(c->terminal, &row, &col) &&
        (SDL_GetTicks() / 500) % 2 == 0) {
        fill(r,
             (SDL_FRect){c->body.x + col * UI_FONT_WIDTH,
                         c->body.y + (row + 1) * UI_FONT_HEIGHT - 2, UI_FONT_WIDTH, 2},
             UI_ACCENT);
    }
    SDL_FRect footer = {area.x, area.y + area.h - 22, area.w, 22};
    fill(r, footer, UI_PANEL);
    const char *status = *c->error   ? c->error
                         : !c->spawn ? "Konsole | shell disabled in smoke mode"
                         : pe_terminal_process_running(c->process)
                             ? "Shell | Ctrl+Shift+C: copy | Ctrl+Shift+V: paste"
                             : "Shell exited | Restart to open a new session";
    ui_text(r, footer.x + 8, footer.y + 3, status, UI_MUTED);
    UiRect restart = {area.x + area.w - 30, footer.y, 28, 22};
    ui_draw_button(r, (SDL_FRect){restart.x, restart.y, restart.w, restart.h}, ICON_REFRESH, "",
                   false, enabled, ui_hovered(ui, restart));
    if (ui_button(ui, CONSOLE_RESTART, restart, enabled)) {
        PeTerminal *fresh = pe_terminal_new(rows, cols);
        if (fresh) {
            pe_terminal_free(c->terminal);
            c->terminal = fresh;
            c->selection = false;
            start(c);
        }
    }
}

static bool inside(SDL_FRect r, float x, float y) {
    return x >= r.x && y >= r.y && x < r.x + r.w && y < r.y + r.h;
}

static void point(PeConsole *c, float x, float y, int *row, int *col) {
    int cols = pe_terminal_cols(c->terminal), rows = pe_terminal_rows(c->terminal);
    *row = (int)floorf((y - c->body.y) / UI_FONT_HEIGHT);
    *col = (int)floorf((x - c->body.x) / UI_FONT_WIDTH);
    *row = *row < 0 ? 0 : *row >= rows ? rows - 1 : *row;
    *col = *col < 0 ? 0 : *col >= cols ? cols - 1 : *col;
}

static unsigned modifiers(SDL_Keymod mod) {
    return ((mod & SDL_KMOD_SHIFT) ? 1u : 0u) | ((mod & SDL_KMOD_ALT) ? 2u : 0u) |
           ((mod & SDL_KMOD_CTRL) ? 4u : 0u);
}

static void text_input(PeConsole *c, const char *text) {
    const unsigned char *p = (const unsigned char *)text;
    while (*p) {
        uint32_t cp = *p++;
        int n = 0;
        if (cp >= 0xc0 && cp < 0xe0) {
            cp &= 31;
            n = 1;
        } else if (cp >= 0xe0 && cp < 0xf0) {
            cp &= 15;
            n = 2;
        } else if (cp >= 0xf0 && cp < 0xf5) {
            cp &= 7;
            n = 3;
        } else if (cp >= 0x80) {
            cp = '?';
        }
        for (int i = 0; i < n; i++) {
            if ((*p & 0xc0) != 0x80) {
                cp = '?';
                break;
            }
            cp = (cp << 6) | (*p++ & 63);
        }
        pe_terminal_unichar(c->terminal, cp, 0);
    }
}

bool pe_console_event(PeConsole *c, SDL_Window *window, const SDL_Event *e, bool enabled) {
    if (!c) {
        return false;
    }
    if (e->type == SDL_EVENT_WINDOW_FOCUS_LOST) {
        c->focused = c->selecting = false;
        return false;
    }
    if (!c->visible || !c->enabled || !enabled) {
        c->focused = c->selecting = false;
        return false;
    }
    if (e->type == SDL_EVENT_MOUSE_BUTTON_DOWN && e->button.button == SDL_BUTTON_LEFT) {
        bool was_focused = c->focused;
        c->focused = inside(c->body, e->button.x, e->button.y);
        if (was_focused && !c->focused) {
            SDL_StopTextInput(window);
        }
        c->selection = c->selecting = c->focused;
        if (c->focused) {
            point(c, e->button.x, e->button.y, &c->row0, &c->col0);
            c->row1 = c->row0;
            c->col1 = c->col0;
            SDL_StartTextInput(window);
            return true;
        }
    }
    if (e->type == SDL_EVENT_MOUSE_MOTION && c->selecting) {
        point(c, e->motion.x, e->motion.y, &c->row1, &c->col1);
        return true;
    }
    if (e->type == SDL_EVENT_MOUSE_BUTTON_UP && e->button.button == SDL_BUTTON_LEFT &&
        c->selecting) {
        point(c, e->button.x, e->button.y, &c->row1, &c->col1);
        c->selecting = false;
        return true;
    }
    if (e->type == SDL_EVENT_MOUSE_WHEEL && inside(c->area, e->wheel.mouse_x, e->wheel.mouse_y)) {
        pe_terminal_scroll(c->terminal, (int)(e->wheel.y * 3));
        c->selection = false;
        return true;
    }
    if (!c->focused) {
        return false;
    }
    if (e->type == SDL_EVENT_TEXT_INPUT) {
        if (c->suppress_text) {
            c->suppress_text = false;
            return true;
        }
        c->selection = false;
        text_input(c, e->text.text);
        return true;
    }
    if (e->type != SDL_EVENT_KEY_DOWN && e->type != SDL_EVENT_KEY_UP) {
        return false;
    }
    if (e->type == SDL_EVENT_KEY_UP) {
        c->suppress_text = false;
        return true;
    }
    SDL_Keycode key = e->key.key;
    SDL_Keymod mod = e->key.mod;
    bool clipboard = (mod & SDL_KMOD_GUI) || ((mod & SDL_KMOD_CTRL) && (mod & SDL_KMOD_SHIFT));
    if (clipboard && key == SDLK_C) {
        if (c->selection) {
            char *copy = pe_terminal_copy(c->terminal, c->row0, c->col0, c->row1, c->col1);
            if (copy) {
                SDL_SetClipboardText(copy);
                free(copy);
            }
        }
        return true;
    }
    if (clipboard && key == SDLK_V) {
        char *paste = SDL_GetClipboardText();
        if (paste) {
            if (!pe_terminal_paste(c->terminal, paste, true)) {
                snprintf(c->error, sizeof c->error, "Paste exceeds terminal input limit");
            }
            SDL_free(paste);
        }
        return true;
    }
    if ((mod & SDL_KMOD_SHIFT) && (key == SDLK_PAGEUP || key == SDLK_PAGEDOWN)) {
        pe_terminal_scroll(c->terminal, key == SDLK_PAGEUP ? pe_terminal_rows(c->terminal)
                                                           : -pe_terminal_rows(c->terminal));
        return true;
    }
    c->selection = false;
    const SDL_Keycode keys[] = {SDLK_RETURN, SDLK_TAB,  SDLK_BACKSPACE, SDLK_ESCAPE,  SDLK_UP,
                                SDLK_DOWN,   SDLK_LEFT, SDLK_RIGHT,     SDLK_INSERT,  SDLK_DELETE,
                                SDLK_HOME,   SDLK_END,  SDLK_PAGEUP,    SDLK_PAGEDOWN};
    for (int i = 0; i < PE_TERM_F1; i++) {
        if (key == keys[i]) {
            pe_terminal_key(c->terminal, (PeTerminalKey)i, modifiers(mod));
            return true;
        }
    }
    if (key >= SDLK_F1 && key <= SDLK_F12) {
        pe_terminal_key(c->terminal, (PeTerminalKey)(PE_TERM_F1 + key - SDLK_F1), modifiers(mod));
        return true;
    }
    if ((mod & (SDL_KMOD_CTRL | SDL_KMOD_ALT)) && !(mod & SDL_KMOD_MODE) &&
        (mod & (SDL_KMOD_CTRL | SDL_KMOD_ALT)) != (SDL_KMOD_CTRL | SDL_KMOD_ALT) && key >= 32 &&
        key < 127) {
        c->suppress_text = true;
        pe_terminal_unichar(c->terminal, (uint32_t)key, modifiers(mod));
    }
    return true;
}
