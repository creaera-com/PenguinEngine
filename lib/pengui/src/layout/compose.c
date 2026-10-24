/** @file @brief Transactional composition of independently parsed layouts at named slots. */
#include "pengui/pengui.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct {
    PUILayout *out;
    const PUILayout *base, *content;
    int slot;
    int removed;
    bool removing;
    const char *tab_id, *title;
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
    if (c->tab_id && source == c->content && !n->generated_id) {
        if (snprintf(n->name, sizeof n->name, "%s.%s", c->tab_id, source->nodes[index].name) >=
            (int)sizeof n->name) {
            return fail(c, "prefixed widget ID exceeds capacity");
        }
        n->id = pui_id(n->name);
    }
    if (c->tab_id && source == c->content && *n->filter_input) {
        char reference[sizeof n->filter_input];
        if (snprintf(reference, sizeof reference, "%s.%s", c->tab_id, n->filter_input) >=
            (int)sizeof reference) {
            return fail(c, "prefixed filter input ID exceeds capacity");
        }
        snprintf(n->filter_input, sizeof n->filter_input, "%s", reference);
    }
    if (c->tab_id && source == c->content && *n->color_input) {
        char reference[sizeof n->color_input];
        if (snprintf(reference, sizeof reference, "%s.%s", c->tab_id, n->color_input) >=
            (int)sizeof reference) {
            return fail(c, "prefixed color input ID exceeds capacity");
        }
        snprintf(n->color_input, sizeof n->color_input, "%s", reference);
    }
    if (c->tab_id && source == c->content && *n->modal_target) {
        char reference[sizeof n->modal_target];
        if (snprintf(reference, sizeof reference, "%s.%s", c->tab_id, n->modal_target) >=
            (int)sizeof reference) {
            return fail(c, "prefixed modal target ID exceeds capacity");
        }
        snprintf(n->modal_target, sizeof n->modal_target, "%s", reference);
    }
    if (*n->image) {
        const char *image = source->nodes[index].image;
        bool absolute = image[0] == '/' || image[0] == '\\' || (image[0] && image[1] == ':');
        if (snprintf(n->image, sizeof n->image, "%s%s", absolute ? "" : source->directory, image) >=
            (int)sizeof n->image) {
            return fail(c, "composed image path exceeds capacity");
        }
    }
    if (!c->tab_id && source == c->base && index == c->slot) {
        if (c->content) {
            n->child = copy_node(c, c->content, c->content->root, depth + 1);
        }
    } else {
        int previous = -1;
        for (int k = source->nodes[index].child; k >= 0; k = source->nodes[k].next) {
            if (c->removing && source == c->base && k == c->removed) {
                continue;
            }
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
    if (c->tab_id && source == c->base && index == c->slot) {
        if (c->out->count >= PUI_LAYOUT_NODES || depth + 1 >= PUI_LAYOUT_DEPTH) {
            return fail(c, "composed layout exceeds node/depth limit");
        }
        int tab = c->out->count++;
        PUINode *added = &c->out->nodes[tab];
        *added = (PUINode){.type = PUI_NODE_TAB,
                           .radius = -1,
                           .transition_ms = -1,
                           .enabled = true,
                           .open = true,
                           .child = -1,
                           .next = -1,
                           .id = pui_id(c->tab_id)};
        snprintf(added->name, sizeof added->name, "%s", c->tab_id);
        snprintf(added->text, sizeof added->text, "%s", c->title);
        if (n->child < 0) {
            n->child = tab;
        } else {
            int previous = n->child;
            while (c->out->nodes[previous].next >= 0) {
                previous = c->out->nodes[previous].next;
            }
            c->out->nodes[previous].next = tab;
        }
        added->child = copy_node(c, c->content, c->content->root, depth + 2);
    }
    return c->failed ? -1 : target;
}

static PUILayout *build(Composition *c) {
    c->out = malloc(sizeof *c->out);
    if (!c->out) {
        fail(c, "out of memory");
        return NULL;
    }
    *c->out = *c->base;
    c->out->count = 0;
    *c->out->directory = 0; /* Every copied image now carries its own source directory. */
    c->out->root = copy_node(c, c->base, c->base->root, 0);
    for (int i = 0; i < c->out->count && !c->failed; i++) {
        for (int j = 0; j < i; j++) {
            if (c->out->nodes[i].id == c->out->nodes[j].id) {
                fail(c, "duplicate widget ID across composed layouts");
                break;
            }
        }
    }
    if (c->failed) {
        pui_layout_free(c->out);
        return NULL;
    }
    return c->out;
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
    return build(&c);
}

PUILayout *pui_layout_add_tab(const PUILayout *base, const char *tabs, const char *tab_id,
                              const char *title, const PUILayout *content, char *error,
                              size_t capacity) {
    if (error && capacity) {
        *error = 0;
    }
    Composition c = {.base = base,
                     .content = content,
                     .slot = -1,
                     .tab_id = tab_id,
                     .title = title,
                     .error = error,
                     .capacity = capacity};
    if (!base || !tabs || !tab_id || !*tab_id || !title || !content ||
        strlen(tab_id) >= sizeof base->nodes[0].name ||
        strlen(title) >= sizeof base->nodes[0].text) {
        fail(&c, "valid layout, tab ID, title and content are required");
        return NULL;
    }
    for (int i = 0; i < base->count; i++) {
        if (base->nodes[i].type == PUI_NODE_TABS && !strcmp(base->nodes[i].name, tabs)) {
            c.slot = i;
            int count = 0;
            for (int k = base->nodes[i].child; k >= 0; k = base->nodes[k].next) {
                count++;
            }
            if (count >= PUI_MAX_TABS) {
                fail(&c, "tab limit reached");
                return NULL;
            }
            break;
        }
    }
    if (c.slot < 0) {
        fail(&c, "tabs node not found");
        return NULL;
    }
    return build(&c);
}

PUILayout *pui_layout_remove_tab(const PUILayout *base, const char *tabs, const char *tab_id,
                                 char *error, size_t capacity) {
    if (error && capacity) {
        *error = 0;
    }
    Composition c = {.base = base,
                     .slot = -1,
                     .removed = -1,
                     .removing = true,
                     .error = error,
                     .capacity = capacity};
    if (!base || !tabs || !tab_id) {
        fail(&c, "layout, tabs and tab ID required");
        return NULL;
    }
    for (int i = 0; i < base->count; i++) {
        if (base->nodes[i].type == PUI_NODE_TABS && !strcmp(base->nodes[i].name, tabs)) {
            for (int k = base->nodes[i].child; k >= 0; k = base->nodes[k].next) {
                if (!strcmp(base->nodes[k].name, tab_id)) {
                    c.removed = k;
                    break;
                }
            }
        }
    }
    if (c.removed < 0) {
        fail(&c, "tab not found");
        return NULL;
    }
    return build(&c);
}
