#include "script_editor.h"
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define LINE_HEIGHT 18
#define CHAR_WIDTH 8
#define GUTTER 54
static bool inside(float x, float y, SDL_FRect r) {
    return x >= r.x && y >= r.y && x < r.x + r.w && y < r.y + r.h;
}
static void color(SDL_Renderer *r, Uint32 c) {
    SDL_SetRenderDrawColor(r, (Uint8)(c >> 24), (Uint8)(c >> 16), (Uint8)(c >> 8), (Uint8)c);
}
static void rect(SDL_Renderer *r, SDL_FRect b, Uint32 c) {
    color(r, c);
    SDL_RenderFillRect(r, &b);
}
static void text(SDL_Renderer *r, float x, float y, const char *s, Uint32 c) {
    color(r, c);
    SDL_RenderDebugText(r, x, y, s);
}
static void layout(PeScriptEditor *e, SDL_FRect a) {
    e->body = (SDL_FRect){a.x + GUTTER, a.y + 38, a.w - GUTTER - 12, a.h - 86};
}
static void reveal(PeScriptEditor *e, SDL_FRect area) {
    layout(e, area);
    int line, col;
    pe_text_location(&e->document, e->document.cursor, &line, &col);
    int rows = (int)e->body.h / LINE_HEIGHT, columns = (int)e->body.w / CHAR_WIDTH;
    if (rows < 1)
        rows = 1;
    if (columns < 1)
        columns = 1;
    if (line < e->top_line)
        e->top_line = line;
    if (line >= e->top_line + rows)
        e->top_line = line - rows + 1;
    if (col < e->left_column)
        e->left_column = col;
    if (col >= e->left_column + columns)
        e->left_column = col - columns + 1;
}
void se_check(PeScriptEditor *e) {
    e->diagnostic = pe_js_check(e->document.text, e->path);
    e->checked_revision = e->document.revision;
    snprintf(e->status, sizeof e->status, "%s", e->diagnostic.message);
}
bool se_init(PeScriptEditor *e, const char *root) {
    memset(e, 0, sizeof *e);
    if (strlen(root) >= sizeof e->root)
        return false;
    strcpy(e->root, root);
    pe_text_init(&e->document, "");
    snprintf(e->path, sizeof e->path, "%s/scripts/player.js", root);
    SDL_PathInfo info;
    if (!SDL_GetPathInfo(e->path, &info) || info.type != SDL_PATHTYPE_FILE)
        snprintf(e->path, sizeof e->path, "%s/scripts/new_script.js", root);
    if (SDL_GetPathInfo(e->path, &info) && info.type == SDL_PATHTYPE_FILE) {
        if (!pe_text_load(&e->document, e->path, e->status, sizeof e->status))
            return false;
        e->exists = true;
    }
    e->highlighted_revision = UINT64_MAX;
    e->checked_revision = UINT64_MAX;
    e->goal_column = -1;
    se_check(e);
    return true;
}
void se_free(PeScriptEditor *e) {
    pe_text_free(&e->document);
}
void se_focus(PeScriptEditor *e, SDL_Window *w, bool focus) {
    if (w)
        e->window = w;
    e->focused = focus;
    e->selecting = false;
    if (focus) {
        if (e->window)
            SDL_StartTextInput(e->window);
    } else {
        if (e->window)
            SDL_StopTextInput(e->window);
        e->show_completion = false;
    }
}
bool se_open(PeScriptEditor *e, const char *path) {
    if (strlen(path) >= sizeof e->path) {
        snprintf(e->status, sizeof e->status, "Script path too long");
        return false;
    }
    if (!pe_text_load(&e->document, path, e->status, sizeof e->status))
        return false;
    strcpy(e->path, path);
    e->exists = true;
    e->top_line = e->left_column = 0;
    e->highlighted_revision = e->checked_revision = UINT64_MAX;
    e->show_completion = false;
    e->goal_column = -1;
    se_check(e);
    return true;
}
bool se_save(PeScriptEditor *e) {
    if (!e->exists) {
        char folder[1024];
        snprintf(folder, sizeof folder, "%s/scripts", e->root);
        if (!SDL_CreateDirectory(folder)) {
            snprintf(e->status, sizeof e->status, "%s", SDL_GetError());
            return false;
        }
    }
    if (!pe_text_save(&e->document, e->path, e->status, sizeof e->status))
        return false;
    e->exists = true;
    se_check(e);
    snprintf(e->status, sizeof e->status, "Script saved%s",
             e->diagnostic.valid ? "" : " (contains syntax errors)");
    return true;
}
bool se_dirty(const PeScriptEditor *e) {
    return pe_text_dirty(&e->document) || (!e->exists && e->document.length > 0);
}
void se_changed(PeScriptEditor *e, SDL_FRect area, bool completion) {
    e->changed_at = SDL_GetTicks();
    e->goal_column = -1;
    reveal(e, area);
    e->show_completion = false;
    if (completion) {
        e->completions = pe_js_complete(&e->document);
        e->show_completion =
            e->completions.count > 0 && e->completions.end - e->completions.start >= 2;
        e->completion_index = 0;
    }
    snprintf(e->status, sizeof e->status, "%s",
             e->checked_revision == e->document.revision
                 ? e->diagnostic.message
                 : "Editing JavaScript; syntax check after a short pause");
}
static void changed_result(PeScriptEditor *e, SDL_FRect area, bool ok, bool completion) {
    if (ok)
        se_changed(e, area, completion);
    else
        snprintf(e->status, sizeof e->status,
                 "Edit rejected: invalid UTF-8, file limit (64 KiB) or out of memory");
}
static void caret(PeScriptEditor *e, size_t offset, bool shift, SDL_FRect area) {
    e->document.cursor = offset;
    if (!shift)
        e->document.anchor = offset;
    e->show_completion = false;
    reveal(e, area);
}
static void complete(PeScriptEditor *e, SDL_FRect area) {
    changed_result(e, area, pe_js_accept(&e->document, &e->completions, e->completion_index),
                   false);
}
static void copy(PeScriptEditor *e, bool cut, SDL_FRect area) {
    size_t a = e->document.cursor, b = e->document.anchor;
    if (a > b) {
        size_t t = a;
        a = b;
        b = t;
    }
    if (a == b)
        return;
    char *selected = malloc(b - a + 1);
    if (!selected)
        return;
    memcpy(selected, e->document.text + a, b - a);
    selected[b - a] = 0;
    bool ok = SDL_SetClipboardText(selected);
    free(selected);
    if (ok && cut)
        changed_result(e, area, pe_text_insert(&e->document, ""), false);
}
static void newline(PeScriptEditor *e, SDL_FRect area) {
    size_t start = pe_text_line_start(&e->document, e->document.cursor), n = 0;
    char indent[512];
    indent[n++] = '\n';
    while (start < e->document.cursor &&
           (e->document.text[start] == ' ' || e->document.text[start] == '\t') &&
           n < sizeof indent - 6)
        indent[n++] = e->document.text[start++];
    size_t p = e->document.cursor;
    while (p > start && e->document.text[p - 1] == ' ')
        p--;
    if (p && e->document.text[p - 1] == '{')
        for (int i = 0; i < 4; i++)
            indent[n++] = ' ';
    indent[n] = 0;
    changed_result(e, area, pe_text_insert(&e->document, indent), false);
}
bool se_event(PeScriptEditor *e, SDL_Window *window, const SDL_Event *event, SDL_FRect area) {
    layout(e, area);
    PeText *d = &e->document;
    if (event->type == SDL_EVENT_WINDOW_FOCUS_LOST) {
        e->selecting = false;
        e->show_completion = false;
        return false;
    }
    if (event->type == SDL_EVENT_TEXT_INPUT) {
        if (e->focused)
            changed_result(e, area, pe_text_insert(d, event->text.text), true);
        return true;
    }
    if (event->type == SDL_EVENT_KEY_DOWN) {
        SDL_Keycode key = event->key.key;
        bool ctrl = (event->key.mod & (SDL_KMOD_CTRL | SDL_KMOD_GUI)) != 0,
             shift = (event->key.mod & SDL_KMOD_SHIFT) != 0;
        if (ctrl && key == SDLK_S) {
            se_save(e);
            return true;
        }
        if (ctrl && (key == SDLK_Y || (key == SDLK_Z && shift))) {
            if (pe_text_redo(d))
                se_changed(e, area, false);
            return true;
        }
        if (ctrl && key == SDLK_Z) {
            if (pe_text_undo(d))
                se_changed(e, area, false);
            return true;
        }
        if (!e->focused)
            return true;
        if (ctrl && key == SDLK_SPACE) {
            e->completions = pe_js_complete(d);
            e->show_completion = e->completions.count > 0;
            e->completion_index = 0;
            return true;
        }
        if (e->show_completion) {
            if (key == SDLK_ESCAPE) {
                e->show_completion = false;
                return true;
            }
            if (key == SDLK_DOWN || key == SDLK_UP) {
                int delta = key == SDLK_DOWN ? 1 : -1;
                e->completion_index =
                    (e->completion_index + delta + e->completions.count) % e->completions.count;
                return true;
            }
            if (key == SDLK_RETURN || key == SDLK_TAB) {
                complete(e, area);
                return true;
            }
        }
        if (ctrl && key == SDLK_A) {
            d->anchor = 0;
            d->cursor = d->length;
            reveal(e, area);
        } else if (ctrl && key == SDLK_C)
            copy(e, false, area);
        else if (ctrl && key == SDLK_X)
            copy(e, true, area);
        else if (ctrl && key == SDLK_V) {
            char *clipboard = SDL_GetClipboardText();
            if (clipboard) {
                size_t out = 0;
                for (size_t i = 0; clipboard[i]; i++) {
                    if (clipboard[i] == '\r') {
                        clipboard[out++] = '\n';
                        if (clipboard[i + 1] == '\n')
                            i++;
                    } else
                        clipboard[out++] = clipboard[i];
                }
                clipboard[out] = 0;
                changed_result(e, area, pe_text_insert(d, clipboard), false);
                SDL_free(clipboard);
            }
        } else if (key == SDLK_F7)
            se_check(e);
        else if (key == SDLK_BACKSPACE)
            changed_result(e, area, pe_text_backspace(d), true);
        else if (key == SDLK_DELETE)
            changed_result(e, area, pe_text_delete(d), true);
        else if (key == SDLK_RETURN)
            newline(e, area);
        else if (key == SDLK_TAB)
            changed_result(e, area, pe_text_insert(d, "    "), false);
        else if (key == SDLK_LEFT || key == SDLK_RIGHT) {
            size_t p;
            if (!shift && d->cursor != d->anchor)
                p = key == SDLK_LEFT ? (d->cursor < d->anchor ? d->cursor : d->anchor)
                                     : (d->cursor > d->anchor ? d->cursor : d->anchor);
            else
                p = key == SDLK_LEFT ? pe_text_previous(d, d->cursor) : pe_text_next(d, d->cursor);
            e->goal_column = -1;
            caret(e, p, shift, area);
        } else if (key == SDLK_HOME || key == SDLK_END) {
            e->goal_column = -1;
            caret(e,
                  ctrl ? (key == SDLK_HOME ? 0 : d->length)
                       : (key == SDLK_HOME ? pe_text_line_start(d, d->cursor)
                                           : pe_text_line_end(d, d->cursor)),
                  shift, area);
        } else if (key == SDLK_UP || key == SDLK_DOWN || key == SDLK_PAGEUP ||
                   key == SDLK_PAGEDOWN) {
            int line, col;
            pe_text_location(d, d->cursor, &line, &col);
            if (e->goal_column < 0)
                e->goal_column = col;
            int amount =
                (key == SDLK_PAGEUP || key == SDLK_PAGEDOWN) ? (int)e->body.h / LINE_HEIGHT : 1;
            line += (key == SDLK_UP || key == SDLK_PAGEUP) ? -amount : amount;
            caret(e, pe_text_offset(d, line, e->goal_column), shift, area);
        } else if (key == SDLK_ESCAPE)
            e->show_completion = false;
        return true;
    }
    if (event->type == SDL_EVENT_MOUSE_WHEEL &&
        inside(event->wheel.mouse_x, event->wheel.mouse_y, area)) {
        if (SDL_GetModState() & SDL_KMOD_SHIFT) {
            e->left_column -= (int)event->wheel.y * 4;
            if (e->left_column < 0)
                e->left_column = 0;
        } else {
            e->top_line -= (int)event->wheel.y * 3;
            int max_line, col;
            pe_text_location(d, d->length, &max_line, &col);
            if (e->top_line < 0)
                e->top_line = 0;
            if (e->top_line > max_line)
                e->top_line = max_line;
        }
        e->show_completion = false;
        return true;
    }
    if (event->type == SDL_EVENT_MOUSE_BUTTON_DOWN && event->button.button == SDL_BUTTON_LEFT) {
        float x = event->button.x, y = event->button.y;
        if (e->show_completion && inside(x, y, e->popup)) {
            int index = (int)(y - e->popup.y) / 22;
            if (index < e->completions.count) {
                e->completion_index = index;
                complete(e, area);
            }
            return true;
        }
        if (inside(x, y, e->body)) {
            se_focus(e, window, true);
            e->goal_column = -1;
            int line = e->top_line + (int)(y - e->body.y) / LINE_HEIGHT,
                col = e->left_column + (int)(x - e->body.x) / CHAR_WIDTH;
            caret(e, pe_text_offset(d, line, col), (SDL_GetModState() & SDL_KMOD_SHIFT) != 0, area);
            e->selecting = true;
            return true;
        }
        if (!inside(x, y, area) && y >= area.y - 32)
            se_focus(e, window, false);
    }
    if (event->type == SDL_EVENT_MOUSE_MOTION && e->selecting) {
        int line = e->top_line + (int)((event->motion.y - e->body.y) / LINE_HEIGHT),
            col = e->left_column + (int)((event->motion.x - e->body.x) / CHAR_WIDTH);
        if (line < 0)
            line = 0;
        if (col < 0)
            col = 0;
        caret(e, pe_text_offset(d, line, col), true, area);
        return true;
    }
    if (event->type == SDL_EVENT_MOUSE_BUTTON_UP && event->button.button == SDL_BUTTON_LEFT &&
        e->selecting) {
        e->selecting = false;
        return true;
    }
    return false;
}
void se_draw(PeScriptEditor *e, SDL_Renderer *r, SDL_FRect area, float mx, float my, bool click) {
    layout(e, area);
    PeText *d = &e->document;
    if (e->highlighted_revision != d->revision) {
        pe_js_highlight(d->text, e->tokens, d->length);
        e->highlighted_revision = d->revision;
    }
    if (e->checked_revision != d->revision && SDL_GetTicks() - e->changed_at >= 300)
        se_check(e);
    rect(r, area, UI_CANVAS);
    rect(r, (SDL_FRect){area.x, area.y, area.w, 30}, UI_PANEL);
    const char *filename = strrchr(e->path, '/');
    filename = filename ? filename + 1 : e->path;
    char title[96];
    snprintf(title, sizeof title, "JS  %.55s%s", filename, se_dirty(e) ? " *" : "");
    text(r, area.x + 14, area.y + 11, title, UI_TEXT);
    SDL_FRect check = {area.x + area.w - 158, area.y + 3, 146, 24};
    bool hover = inside(mx, my, check);
    ui_draw_button(r, check, ICON_CHECK, "Check (F7)", false, true, hover);
    if (click && hover)
        se_check(e);
    SDL_Rect clip = {(int)area.x, (int)e->body.y, (int)area.w, (int)e->body.h};
    SDL_SetRenderClipRect(r, &clip);
    int caret_line, caret_col;
    pe_text_location(d, d->cursor, &caret_line, &caret_col);
    rect(r, (SDL_FRect){area.x, e->body.y, GUTTER - 10, e->body.h}, 0x1d1d1dff);
    if (caret_line >= e->top_line) {
        float cy = e->body.y + (caret_line - e->top_line) * LINE_HEIGHT;
        rect(r, (SDL_FRect){e->body.x, cy, e->body.w, LINE_HEIGHT}, 0x252525ff);
    }
    size_t selected_start = d->cursor < d->anchor ? d->cursor : d->anchor,
           selected_end = d->cursor > d->anchor ? d->cursor : d->anchor;
    size_t p = pe_text_offset(d, e->top_line, 0);
    int rows = (int)e->body.h / LINE_HEIGHT + 1, line = e->top_line;
    static const Uint32 palette[] = {UI_TEXT,    0xffa650ff, 0xc5c98aff,
                                     0x7d8876ff, 0xd9ad86ff, 0xcab0d8ff};
    for (int row = 0; row < rows; row++, line++) {
        float y = e->body.y + row * LINE_HEIGHT;
        char number[16];
        snprintf(number, sizeof number, "%4d", line + 1);
        text(r, area.x + 8, y + 5, number, line == caret_line ? UI_ACCENT : UI_DISABLED);
        if (e->diagnostic.line == line + 1 && !e->diagnostic.valid)
            rect(r, (SDL_FRect){area.x + GUTTER - 7, y + 4, 3, 10}, UI_ACCENT);
        SDL_Rect body_clip = {(int)e->body.x, (int)e->body.y, (int)e->body.w, (int)e->body.h};
        SDL_SetRenderClipRect(r, &body_clip);
        int col = 0;
        while (p < d->length && d->text[p] != '\n') {
            size_t next = pe_text_next(d, p);
            int width = d->text[p] == '\t' ? 4 - col % 4 : 1;
            float x = e->body.x + (col - e->left_column) * CHAR_WIDTH;
            if (x + width * CHAR_WIDTH >= e->body.x && x < e->body.x + e->body.w) {
                if (p >= selected_start && p < selected_end)
                    rect(r, (SDL_FRect){x, y, (float)(width * CHAR_WIDTH), LINE_HEIGHT},
                         0x5a3d26ff);
                char glyph[2] = {
                    d->text[p] == '\t' ? ' ' : ((unsigned char)d->text[p] < 128 ? d->text[p] : '?'),
                    0};
                text(r, x, y + 5, glyph, palette[e->tokens[p]]);
            }
            col += width;
            p = next;
        }
        SDL_SetRenderClipRect(r, &clip);
        if (p == d->length)
            break;
        if (p >= selected_start && p < selected_end)
            rect(r, (SDL_FRect){e->body.x + (col - e->left_column) * CHAR_WIDTH, y, 8, LINE_HEIGHT},
                 0x5a3d26ff);
        p++;
    }
    SDL_Rect caret_clip = {(int)e->body.x, (int)e->body.y, (int)e->body.w, (int)e->body.h};
    SDL_SetRenderClipRect(r, &caret_clip);
    if (e->focused && SDL_GetTicks() % 1000 < 650) {
        float cx = e->body.x + (caret_col - e->left_column) * CHAR_WIDTH,
              cy = e->body.y + (caret_line - e->top_line) * LINE_HEIGHT;
        rect(r, (SDL_FRect){cx, cy + 2, 2, LINE_HEIGHT - 3}, UI_ACCENT);
    }
    SDL_SetRenderClipRect(r, NULL);
    SDL_FRect diagnostic = {area.x, area.y + area.h - 40, area.w, 40};
    rect(r, diagnostic, UI_PANEL);
    ui_icon(r, e->diagnostic.valid ? ICON_CHECK : ICON_COLLISION, area.x + 12, diagnostic.y + 11,
            16, e->diagnostic.valid ? UI_MUTED : UI_ACCENT);
    char status[320];
    if (e->checked_revision != d->revision)
        snprintf(status, sizeof status, "Checking JavaScript...");
    else if (e->diagnostic.valid)
        snprintf(status, sizeof status, "Syntax OK | compile only; scripts are not executed");
    else
        snprintf(status, sizeof status, "Line %d: %.220s", e->diagnostic.line,
                 e->diagnostic.message);
    SDL_Rect status_clip = {(int)area.x + 36, (int)diagnostic.y, (int)area.w - 44, 40};
    SDL_SetRenderClipRect(r, &status_clip);
    text(r, area.x + 36, diagnostic.y + 15, status, e->diagnostic.valid ? UI_MUTED : UI_ACCENT);
    SDL_SetRenderClipRect(r, NULL);
    if (click && inside(mx, my, diagnostic) && !e->diagnostic.valid && e->diagnostic.line > 0) {
        se_focus(e, e->window, true);
        caret(e,
              pe_text_offset(d, e->diagnostic.line - 1,
                             e->diagnostic.column > 0 ? e->diagnostic.column - 1 : 0),
              false, area);
    }
    if (e->show_completion && e->completions.count) {
        float x = e->body.x + (caret_col - e->left_column) * CHAR_WIDTH,
              y = e->body.y + (caret_line - e->top_line + 1) * LINE_HEIGHT;
        float height = e->completions.count * 22.f;
        if (x + 216 > area.x + area.w)
            x = area.x + area.w - 216;
        if (y + height > diagnostic.y)
            y = fmaxf(e->body.y, y - height - LINE_HEIGHT);
        e->popup = (SDL_FRect){x, y, 212, height};
        ui_round_rect(r, e->popup, 5, UI_BORDER);
        for (int i = 0; i < e->completions.count; i++) {
            SDL_FRect item = {x + 1, y + i * 22 + 1, 210, 21};
            rect(r, item, i == e->completion_index ? UI_ACCENT_BG : UI_SURFACE);
            text(r, x + 10, y + i * 22 + 7, e->completions.values[i],
                 i == e->completion_index ? UI_ACCENT : UI_TEXT);
        }
    }
}
