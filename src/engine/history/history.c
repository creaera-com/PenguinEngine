#include "penguin/engine.h"
#include <math.h>
#include <stdlib.h>
#include <string.h>

void pe_history_init(PeHistory *h) {
    memset(h, 0, sizeof *h);
    h->next_revision = 1;
}
static void command_free(PeCommand *c) {
    free(c->before);
    free(c->after);
    memset(c, 0, sizeof *c);
}
void pe_history_free(PeHistory *h) {
    for (int i = 0; i < h->count; i++)
        command_free(&h->commands[i]);
    free(h->pending);
    pe_history_init(h);
}
bool pe_history_begin(PeHistory *h, const PeScene *s) {
    if (h->pending)
        return false;
    h->pending = malloc(sizeof *s);
    if (!h->pending)
        return false;
    *h->pending = *s;
    return true;
}
void pe_history_cancel(PeHistory *h, PeScene *s) {
    if (h->pending) {
        *s = *h->pending;
        free(h->pending);
        h->pending = NULL;
    }
}
bool pe_history_commit(PeHistory *h, PeScene *s) {
    if (!h->pending)
        return false;
    if (!memcmp(h->pending, s, sizeof *s)) {
        free(h->pending);
        h->pending = NULL;
        return true;
    }
    PeScene *after = malloc(sizeof *after);
    if (!after) {
        pe_history_cancel(h, s);
        return false;
    }
    *after = *s;
    for (int i = h->cursor; i < h->count; i++)
        command_free(&h->commands[i]);
    h->count = h->cursor;
    if (h->count == PE_HISTORY) {
        command_free(&h->commands[0]);
        memmove(h->commands, h->commands + 1, sizeof(PeCommand) * (PE_HISTORY - 1));
        h->count--;
        h->cursor--;
    }
    PeCommand *c = &h->commands[h->count++];
    *c = (PeCommand){h->pending, after, h->revision, h->next_revision++};
    h->pending = NULL;
    h->revision = c->after_revision;
    h->cursor = h->count;
    return true;
}
bool pe_undo(PeHistory *h, PeScene *s) {
    if (h->pending || h->cursor == 0)
        return false;
    PeCommand *c = &h->commands[--h->cursor];
    *s = *c->before;
    h->revision = c->before_revision;
    return true;
}
bool pe_redo(PeHistory *h, PeScene *s) {
    if (h->pending || h->cursor == h->count)
        return false;
    PeCommand *c = &h->commands[h->cursor++];
    *s = *c->after;
    h->revision = c->after_revision;
    return true;
}
void pe_history_saved(PeHistory *h) {
    h->saved_revision = h->revision;
}
bool pe_history_dirty(const PeHistory *h) {
    return h->revision != h->saved_revision || h->pending != NULL;
}
