#include "editors/graphics/vector/document/document.h"
#include <stdlib.h>
#include <string.h>

bool ve_history_init(VeHistory *h, const VeDocument *d) {
    memset(h, 0, sizeof *h);
    h->states[0] = ve_json(d);
    if (!h->states[0]) {
        return false;
    }
    h->count = 1;
    h->next = 1;
    return true;
}

void ve_history_free(VeHistory *h) {
    for (int i = 0; i < h->count; i++) {
        free(h->states[i]);
    }
    memset(h, 0, sizeof *h);
}

bool ve_history_commit(VeHistory *h, const VeDocument *d) {
    char *json = ve_json(d);
    if (!json) {
        return false;
    }
    if (h->count && !strcmp(json, h->states[h->position])) {
        free(json);
        return false;
    }
    for (int i = h->position + 1; i < h->count; i++) {
        free(h->states[i]);
    }
    h->count = h->position + 1;
    if (h->count == VE_HISTORY) {
        free(h->states[0]);
        memmove(h->states, h->states + 1, (VE_HISTORY - 1) * sizeof *h->states);
        memmove(h->ids, h->ids + 1, (VE_HISTORY - 1) * sizeof *h->ids);
        h->count--;
        h->position--;
    }
    h->position = h->count++;
    h->states[h->position] = json;
    h->ids[h->position] = h->next++;
    return true;
}

bool ve_history_step(VeHistory *h, VeDocument *d, bool redo) {
    int next = h->position + (redo ? 1 : -1);
    char error[128];
    if (next < 0 || next >= h->count || !ve_from_json(d, h->states[next], error, sizeof error)) {
        return false;
    }
    h->position = next;
    return true;
}

bool ve_dirty(const VeHistory *h) {
    return h->count && h->ids[h->position] != h->saved;
}
