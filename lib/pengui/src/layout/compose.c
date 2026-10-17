/** @file @brief Transactional composition of independently parsed layouts at named slots. */
#include "pengui/pengui.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct {
    PUILayout *out;
    const PUILayout *base, *content;
    int slot;
    char *error;
    size_t capacity;
    bool failed;
} Composition;

static int fail(Composition *c, const char *message) {
    if (!c->failed && c->error && c->capacity) {
        snprintf(c->error, c->capacity, "%s", message);
    }
    c->failed = true;
    return -1;
}

static int copy_node(Composition *c, const PUILayout *source, int index, int depth) {
    if (c->out->count >= PUI_LAYOUT_NODES || depth >= PUI_LAYOUT_DEPTH) {
        return fail(c, "composed layout exceeds node/depth limit");
    }
    int target = c->out->count++;
    PUINode *n = &c->out->nodes[target];
    *n = source->nodes[index];
    n->child = n->next = -1;
    if (n->generated_id) {
        snprintf(n->name, sizeof n->name, "_node_%d", target);
        n->id = pui_id(n->name);
    }
    if (*n->image) {
        const char *image = source->nodes[index].image;
        bool absolute = image[0] == '/' || image[0] == '\\' || (image[0] && image[1] == ':');
        if (snprintf(n->image, sizeof n->image, "%s%s", absolute ? "" : source->directory, image) >=
            (int)sizeof n->image) {
            return fail(c, "composed image path exceeds capacity");
        }
    }
    if (source == c->base && index == c->slot) {
        if (c->content) {
            n->child = copy_node(c, c->content, c->content->root, depth + 1);
        }
    } else {
        int previous = -1;
        for (int k = source->nodes[index].child; k >= 0; k = source->nodes[k].next) {
            int child = copy_node(c, source, k, depth + 1);
            if (child < 0) {
                return -1;
            }
            if (previous < 0) {
                n->child = child;
            } else {
                c->out->nodes[previous].next = child;
            }
            previous = child;
        }
    }
    return c->failed ? -1 : target;
}

PUILayout *pui_layout_compose(const PUILayout *base, const char *slot, const PUILayout *content,
                              char *error, size_t capacity) {
    if (error && capacity) {
        *error = 0;
    }
    Composition c = {
        .base = base, .content = content, .slot = -1, .error = error, .capacity = capacity};
    if (!base || !slot) {
        fail(&c, "layout and slot are required");
        return NULL;
    }
    for (int i = 0; i < base->count; i++) {
        if (base->nodes[i].type == PUI_NODE_SLOT && !strcmp(base->nodes[i].name, slot)) {
            c.slot = i;
            break;
        }
    }
    if (c.slot < 0) {
        fail(&c, "slot not found");
        return NULL;
    }
    c.out = malloc(sizeof *c.out);
    if (!c.out) {
        fail(&c, "out of memory");
        return NULL;
    }
    *c.out = *base;
    c.out->count = 0;
    *c.out->directory = 0; /* Every copied image now carries its own source directory. */
    c.out->root = copy_node(&c, base, base->root, 0);
    for (int i = 0; i < c.out->count && !c.failed; i++) {
        for (int j = 0; j < i; j++) {
            if (c.out->nodes[i].id == c.out->nodes[j].id) {
                fail(&c, "duplicate widget ID across composed layouts");
                break;
            }
        }
    }
    if (c.failed) {
        pui_layout_free(c.out);
        return NULL;
    }
    return c.out;
}
