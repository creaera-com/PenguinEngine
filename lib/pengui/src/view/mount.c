/** @file @brief Replace a slot between frames while retaining explicitly identified outer state. */
#include "internal.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static bool contains(const PUILayout *layout, int root, int needle) {
    if (root == needle) {
        return true;
    }
    for (int k = layout->nodes[root].child; k >= 0; k = layout->nodes[k].next) {
        if (contains(layout, k, needle)) {
            return true;
        }
    }
    return false;
}

static void retain_state(PUIView *old, PUIView *next, const char *slot) {
    int replaced = -1;
    for (int i = 0; i < old->layout->count; i++) {
        if (!strcmp(old->layout->nodes[i].name, slot)) {
            replaced = i;
        }
    }
    for (int i = 0; i < next->layout->count; i++) {
        const PUINode *node = &next->layout->nodes[i];
        if (node->generated_id) {
            continue;
        }
        for (int j = 0; j < old->layout->count; j++) {
            const PUINode *previous = &old->layout->nodes[j];
            if (previous->generated_id || node->id != previous->id ||
                node->type != previous->type ||
                (replaced >= 0 && contains(old->layout, replaced, j))) {
                continue;
            }
            Widget initial = next->state[i];
            if (initial.image) {
                SDL_DestroyTexture(initial.image);
            }
            next->state[i] = old->state[j];
            old->state[j].image = NULL;
            next->state[i].group = initial.group;
            next->state[i].commit = false;
            /* Highlight indices and generated tab IDs may change in the composed tree. */
            if (node->type == PUI_NODE_DROPDOWN) {
                const char *value = pui_view_value(old, previous->name);
                next->state[i].selected = initial.selected;
                next->state[i].highlighted = initial.highlighted;
                if (value) {
                    pui_view_set_value(next, node->name, value);
                }
            }
            if (node->type == PUI_NODE_TABS) {
                bool compatible = initial.tabs.count == next->state[i].tabs.count;
                for (int k = 0; k < initial.tabs.count; k++) {
                    bool found = false;
                    for (int t = 0; t < next->state[i].tabs.count; t++) {
                        found |= initial.tabs.items[k].id == next->state[i].tabs.items[t].id;
                    }
                    compatible &= found;
                }
                if (!compatible) {
                    next->state[i].tabs = initial.tabs;
                }
            }
            break;
        }
    }
    /* The group container may have an automatic ID while its buttons have explicit IDs.
     * Reconcile the container with those retained member values before the next frame. */
    for (int i = 0; i < next->layout->count; i++) {
        const PUINode *node = &next->layout->nodes[i];
        if (node->type != PUI_NODE_BUTTON_GROUP) {
            continue;
        }
        for (int k = node->child; k >= 0; k = next->layout->nodes[k].next) {
            if (!next->layout->nodes[k].generated_id && next->state[k].checked) {
                next->state[i].selected = next->layout->nodes[k].id;
                break;
            }
        }
        for (int k = node->child; k >= 0; k = next->layout->nodes[k].next) {
            next->state[k].checked = next->layout->nodes[k].id == next->state[i].selected;
        }
    }
}

bool pui_view_replace_slot(PUIView *view, PUIContext *context, const char *slot,
                           const PUILayout *content, char *error, size_t capacity) {
    if (!view || !context) {
        if (error && capacity) {
            snprintf(error, capacity, "view and input context are required");
        }
        return false;
    }
    PUILayout *layout = pui_layout_compose(view->layout, slot, content, error, capacity);
    if (!layout) {
        return false;
    }
    PUIView *next = pui_view_new(layout, view->renderer);
    PUIView *previous = malloc(sizeof *previous);
    if (!next || !previous) {
        pui_view_free(next);
        pui_layout_free(layout);
        free(previous);
        if (error && capacity) {
            snprintf(error, capacity, "out of memory constructing replacement view");
        }
        return false;
    }
    next->owned_layout = layout;
    retain_state(view, next, slot);
    pui_view_blur(view, context);
    next->pointer_inside = view->pointer_inside;
    *previous = *view;
    *view = *next;
    free(next);
    pui_view_free(previous);
    pui_cancel(context);
    pui_focus(context, 0);
    return true;
}
