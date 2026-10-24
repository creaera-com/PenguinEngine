/** @file @brief Replace a slot between frames while retaining explicitly identified outer state. */
#include "internal.h"
#include <math.h>
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
        if (slot && !strcmp(old->layout->nodes[i].name, slot)) {
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
            if (node->type == PUI_NODE_INPUT && node->input_type != previous->input_type) {
                continue;
            }
            Widget initial = next->state[i];
            if (initial.image) {
                SDL_DestroyTexture(initial.image);
            }
            pui_widget_tree_clear(&next->state[i]);
            next->state[i] = old->state[j];
            next->state[i].modal_layer = NULL;
            next->state[i].modal_opacity = (PUIColorTransition){0};
            old->state[j].tree = NULL;
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
                PUITabs merged = initial.tabs;
                int count = 0;
                for (int k = 0; k < old->state[j].tabs.count; k++) {
                    for (int t = 0; t < initial.tabs.count; t++) {
                        if (old->state[j].tabs.items[k].id == initial.tabs.items[t].id) {
                            merged.items[count++] = initial.tabs.items[t];
                        }
                    }
                }
                for (int t = 0; t < initial.tabs.count; t++) {
                    bool found = false;
                    for (int k = 0; k < count; k++) {
                        found |= merged.items[k].id == initial.tabs.items[t].id;
                    }
                    if (!found) {
                        merged.items[count++] = initial.tabs.items[t];
                    }
                }
                merged.count = count;
                /* If the active tab was removed, prefer its right neighbor, then the last tab. */
                for (int k = 0; k < old->state[j].tabs.count; k++) {
                    if (old->state[j].tabs.items[k].id == old->state[j].tabs.selected && count) {
                        merged.selected = merged.items[k < count ? k : count - 1].id;
                    }
                }
                merged.scroll = old->state[j].tabs.scroll;
                for (int k = 0; k < count; k++) {
                    if (merged.items[k].id == old->state[j].tabs.selected) {
                        merged.selected = old->state[j].tabs.selected;
                    }
                }
                next->state[i].tabs = merged;
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

static bool apply_layout(PUIView *view, PUIContext *context, PUILayout *layout, const char *slot,
                         const char *tabs, const char *selected, char *error, size_t capacity) {
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
    next->canvas_renderer = view->canvas_renderer;
    next->canvas_context = view->canvas_context;
    pui_numeric_drag_cancel(view);
    retain_state(view, next, slot);
    for (int i = 0; i < next->layout->count; i++) {
        if (next->layout->nodes[i].type == PUI_NODE_COLOR_PICKER &&
            next->state[i].color.drag >= 0) {
            pui_widget_color_picker_cancel(next, i);
        }
    }
    for (int i = 0; i < next->layout->count; i++) {
        const PUINode *node = &next->layout->nodes[i];
        if (node->type != PUI_NODE_TABS) {
            continue;
        }
        for (int j = node->child; j >= 0; j = next->layout->nodes[j].next) {
            const PUINode *tab = &next->layout->nodes[j];
            for (int k = 0; k < next->state[i].tabs.count; k++) {
                if (next->state[i].tabs.items[k].id == tab->id) {
                    next->state[i].tabs.items[k].width =
                        pui_tab_content_width(pui_icon_from_name(tab->icon), next->state[j].text) +
                        PUI_TAB_GAP + ((*tab->close_action || *node->close_action) ? 24 : 0);
                }
            }
        }
    }

    if (tabs && selected) {
        for (int i = 0; i < next->layout->count; i++) {
            if (next->layout->nodes[i].type == PUI_NODE_TABS &&
                !strcmp(next->layout->nodes[i].name, tabs)) {
                next->state[i].tabs.selected = pui_id(selected);
                /* Newly appended tabs are last in the preserved visual order. */
                float width = 0;
                for (int k = 0; k < next->state[i].tabs.count; k++) {
                    width += next->state[i].tabs.items[k].width;
                }
                for (int j = 0; j < view->layout->count; j++) {
                    if (!strcmp(view->layout->nodes[j].name, tabs)) {
                        next->state[i].tabs.scroll = fmaxf(0, width - view->rect[j].w);
                    }
                }
            }
        }
    }
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
    return apply_layout(view, context, layout, slot, NULL, NULL, error, capacity);
}

bool pui_view_add_tab(PUIView *view, PUIContext *context, const char *tabs, const char *tab_id,
                      const char *title, const PUILayout *content, char *error, size_t capacity) {
    if (!view || !context) {
        if (error && capacity) {
            snprintf(error, capacity, "view and input context are required");
        }
        return false;
    }
    PUILayout *layout =
        pui_layout_add_tab(view->layout, tabs, tab_id, title, content, error, capacity);
    if (!layout) {
        return false;
    }
    return apply_layout(view, context, layout, NULL, tabs, tab_id, error, capacity);
}

bool pui_view_remove_tab(PUIView *view, PUIContext *context, const char *tabs, const char *tab_id,
                         char *error, size_t capacity) {
    if (!view || !context) {
        return false;
    }
    PUILayout *layout = pui_layout_remove_tab(view->layout, tabs, tab_id, error, capacity);
    return layout && apply_layout(view, context, layout, NULL, NULL, NULL, error, capacity);
}
