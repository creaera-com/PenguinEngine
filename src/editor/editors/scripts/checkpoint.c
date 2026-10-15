#include "editors/scripts/checkpoint.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct {
    uint64_t revision, next, saved, cursor, anchor;
    int count, position;
} TextHeader;

size_t se_checkpoint_header_size(void) {
    return sizeof(TextHeader);
}

bool se_checkpoint(PeScriptEditor *editor, const char *project_root, SDL_Window *window,
                   PeSession *io) {

    PeScriptEditor *e = editor;
    char path[1024] = {0};
    int state[3] = {e->exists, e->top_line, e->left_column};
    if (!io->reading)
        snprintf(path, sizeof path, "%s", e->path);
    if (!pe_session_block(io, path, sizeof path) || !memchr(path, 0, sizeof path) ||
        !pe_session_block(io, state, sizeof state) || state[0] < 0 || state[0] > 1 ||
        state[1] < 0 || state[2] < 0)
        return false;
    PeText *d = &e->document;
    TextHeader header = {d->revision, d->next_revision, d->saved_revision, d->cursor,
                         d->anchor,   d->count,         d->position};
    char *text = d->text;
    if (!pe_session_block(io, &header, sizeof header) || header.count < 0 ||
        header.count > PE_TEXT_HISTORY || header.position < 0 || header.position > header.count ||
        header.next <= header.revision)
        return false;
    if (io->reading)
        text = NULL;
    bool ok = pe_session_string(io, &text, PE_TEXT_MAX);
    if (io->reading) {
        ok = ok && header.cursor <= strlen(text) && header.anchor <= strlen(text) &&
             pe_text_init(d, text);
        free(text);
        if (!ok)
            return false;
        d->revision = header.revision;
        d->next_revision = header.next;
        d->saved_revision = header.saved;
        d->cursor = (size_t)header.cursor;
        d->anchor = (size_t)header.anchor;
        d->count = header.count;
        d->position = header.position;
    } else if (!ok)
        return false;
    for (int i = 0; i < d->count; i++) {
        PeTextEdit *edit = &d->edits[i];
        uint64_t values[5] = {edit->before_cursor, edit->before_anchor, edit->after_cursor,
                              edit->before_revision, edit->after_revision};
        if (!pe_session_block(io, values, sizeof values) ||
            !pe_session_string(io, &edit->before, PE_TEXT_MAX) ||
            !pe_session_string(io, &edit->after, PE_TEXT_MAX) || values[0] > strlen(edit->before) ||
            values[1] > strlen(edit->before) || values[2] > strlen(edit->after))
            return false;
        if (io->reading) {
            edit->before_cursor = (size_t)values[0];
            edit->before_anchor = (size_t)values[1];
            edit->after_cursor = (size_t)values[2];
            edit->before_revision = values[3];
            edit->after_revision = values[4];
        }
    }
    if (io->reading) {
        snprintf(e->path, sizeof e->path, "%s", path);
        snprintf(e->root, sizeof e->root, "%s", project_root);
        e->exists = state[0] != 0;
        e->top_line = state[1];
        e->left_column = state[2];
        e->window = window;
        e->highlighted_revision = e->extent_revision = e->checked_revision = UINT64_MAX;
    }
    return true;
}
