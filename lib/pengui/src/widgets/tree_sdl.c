/** @file @brief Tree rows, disclosure state, guide drawing and scrolling. */
#include "view/internal.h"
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static void tree_vertical(SDL_Renderer *r, float x, float y, float from, float to, float phase) {
    for (int p = (int)ceilf(phase + from); p < (int)ceilf(phase + to); p++) {
        if (p % 6 < 3) {
            SDL_RenderPoint(r, floorf(x), floorf(y + p - phase));
        }
    }
}

void pui_draw_tree_guides(SDL_Renderer *r, const int *depths, int count, int row, PUIRect area,
                          float indent, float marker_center, float max_indent, float branch_end,
                          Uint32 rgba) {
    if (!depths || row < 0 || row >= count || indent <= 0 || area.w <= 0 || area.h <= 0) {
        return;
    }
    /* Skip offscreen rows; the phase stays anchored to tree content while scrolling. */
    SDL_Rect clip;
    if (SDL_RenderClipEnabled(r)) {
        SDL_GetRenderClipRect(r, &clip);
        if (area.y + area.h <= clip.y || area.y >= clip.y + clip.h) {
            return;
        }
    }
    int depth = depths[row];
    float center = area.h * .5f, phase = row * area.h;
    pui_render_color(r, rgba);
    for (int level = 1; level < depth && (level - 1) * indent < max_indent; level++) {
        if (pui_tree_continues(depths, count, row, level)) {
            tree_vertical(r, area.x + (level - 1) * indent + marker_center, area.y, 0, area.h,
                          phase);
        }
    }
    if (depth > 0 && (depth - 1) * indent < max_indent) {
        float x = area.x + (depth - 1) * indent + marker_center;
        tree_vertical(r, x, area.y, 0,
                      pui_tree_continues(depths, count, row, depth) ? area.h : center + 1, phase);
        float end = fminf(branch_end, area.x + area.w);
        for (int p = 0; p <= (int)floorf(end - x); p++) {
            if (p % 6 < 3) {
                SDL_RenderPoint(r, floorf(x + p), floorf(area.y + center));
            }
        }
    }
    if (row + 1 < count && depths[row + 1] > depth && depth * indent <= max_indent) {
        tree_vertical(r, area.x + depth * indent + marker_center, area.y, center + 8, area.h,
                      phase);
    }
}

static int tree_rows(PUIView *v, int i, int depth, int *rows, int *depths, int count,
                     bool enabled) {
    for (int k = v->layout->nodes[i].child; k >= 0; k = v->layout->nodes[k].next) {
        v->enabled[k] = enabled && v->layout->nodes[k].enabled;
        rows[count] = k;
        depths[count++] = depth;
        if (v->state[k].open) {
            count = tree_rows(v, k, depth + 1, rows, depths, count, v->enabled[k]);
        }
    }
    return count;
}

static int runtime_rows(TreeData *tree, int *rows, int *depths, bool *enabled,
                        bool parent_enabled) {
    int count = 0, hidden_depth = -1;
    bool ancestors[PUI_LAYOUT_NODES + 1] = {parent_enabled};
    for (int i = 0; i < tree->count; i++) {
        PUITreeItem *item = &tree->items[i];
        if (hidden_depth >= 0 && item->depth > hidden_depth) {
            continue;
        }
        hidden_depth = -1;
        rows[count] = i;
        depths[count] = item->depth;
        enabled[count] = ancestors[item->depth] && item->enabled;
        ancestors[item->depth + 1] = enabled[count];
        count++;
        if (!tree->open[i]) {
            hidden_depth = item->depth;
        }
    }
    return count;
}

static const PUINode *row_node(PUIView *view, TreeData *data, int index, PUINode *runtime) {
    if (!data) {
        return &view->layout->nodes[index];
    }
    const PUITreeItem *source = &data->items[index];
    *runtime = (PUINode){0};
    runtime->id = pui_id(source->id);
    snprintf(runtime->name, sizeof runtime->name, "%s", source->id);
    snprintf(runtime->text, sizeof runtime->text, "%s", source->text);
    snprintf(runtime->icon, sizeof runtime->icon, "%s", source->icon);
    snprintf(runtime->action, sizeof runtime->action, "%s", source->action);
    runtime->child =
        index + 1 < data->count && data->items[index + 1].depth > source->depth ? index + 1 : -1;
    return runtime;
}

void pui_widget_tree_draw(PUIView *v, PUIContext *pui, int i, PUIRect area, PUIAction action,
                          void *context) {
    const PUINode *n = &v->layout->nodes[i];
    Widget *s = &v->state[i];
    int rows[PUI_LAYOUT_NODES], depths[PUI_LAYOUT_NODES];
    bool row_enabled[PUI_LAYOUT_NODES];
    TreeData *data = s->tree;
    int count = data ? runtime_rows(data, rows, depths, row_enabled, !pui->blocked)
                     : tree_rows(v, i, 0, rows, depths, 0, !pui->blocked);
    s->scroll.content = count * 28.f;
    s->scroll.viewport = area.h;
    pui_scroll_clamp(&s->scroll);
    PUIRect track = {area.x + area.w - 8, area.y, 8, area.h};
    pui_scrollbar(pui, n->id ^ 0x65000000u, track, &s->scroll, PUI_AXIS_Y);
    for (int k = 0; k < count; k++) {
        int index = rows[k];
        PUINode runtime;
        const PUINode *item = row_node(v, data, index, &runtime);
        PUIRect r = {area.x, area.y + k * 28 - s->scroll.offset, area.w - 12, 28};
        if (r.y + r.h <= area.y || r.y >= area.y + area.h) {
            continue;
        }
        bool enabled = data ? row_enabled[k] : v->enabled[index];
        if (pui_button(pui, item->id, r, enabled)) {
            s->selected = item->id;
            if (item->child >= 0) {
                if (data) {
                    data->open[index] = !data->open[index];
                } else {
                    v->state[index].open = !v->state[index].open;
                }
            }
            if (action && *item->action) {
                action(context, item->action, item->name);
            }
        }
    }
    /* Resolve selection before painting so the previous row clears in the same frame. */
    for (int k = 0; k < count; k++) {
        int index = rows[k];
        PUINode runtime;
        const PUINode *item = row_node(v, data, index, &runtime);
        PUIRect r = {area.x, area.y + k * 28 - s->scroll.offset, area.w - 12, 28};
        if (r.y + r.h <= area.y || r.y >= area.y + area.h) {
            continue;
        }
        bool enabled = data ? row_enabled[k] : v->enabled[index];
        bool selected = s->selected == item->id;
        Uint32 background = selected                           ? PUI_ACCENT
                            : (enabled && pui_hovered(pui, r)) ? PUI_HOVER
                            : k % 2                            ? PUI_SURFACE
                                                               : PUI_HOVER;
        pui_panel(v->renderer, pui_frect(r), background);
        float x = r.x + 8 + depths[k] * 18;
        pui_draw_tree_guides(v->renderer, depths, count, k, r, 18, 13, area.w - 24, x + 12,
                             selected ? PUI_CANVAS : PUI_MUTED);
        PUIIcon marker =
            item->child >= 0
                ? ((data ? data->open[index] : v->state[index].open) ? ICON_DOWN : ICON_RIGHT)
                : pui_icon_from_name(item->icon);
        Uint32 foreground = !enabled ? PUI_DISABLED : selected ? PUI_CANVAS : PUI_TEXT;
        pui_icon(v->renderer, marker, x + 4, r.y + 6, 16, foreground);
        pui_text(v->renderer, x + 26, r.y + 6, item->text, foreground);
    }
    pui_draw_scrollbar(v->renderer, pui, n->id ^ 0x65000000u, track, &s->scroll, PUI_AXIS_Y);
}

bool pui_widget_tree_scroll(PUIView *view, int i, const SDL_Event *event) {
    if (view->layout->nodes[i].type != PUI_NODE_TREE) {
        return false;
    }
    pui_scroll_by(&view->state[i].scroll, -event->wheel.y * 48);
    return true;
}

void pui_widget_tree_clear(Widget *state) {
    if (state->tree) {
        free(state->tree->items);
        free(state->tree->open);
        free(state->tree);
        state->tree = NULL;
    }
}

static int find_tree(const PUIView *view, const char *name) {
    if (view && name) {
        for (int i = 0; i < view->layout->count; i++) {
            if (view->layout->nodes[i].type == PUI_NODE_TREE &&
                !strcmp(view->layout->nodes[i].name, name)) {
                return i;
            }
        }
    }
    return -1;
}

bool pui_view_set_tree_items(PUIView *view, const char *tree, const PUITreeItem *items, int count) {
    int index = find_tree(view, tree);
    if (index < 0 || count < 0 || count > PUI_LAYOUT_NODES || (count && !items)) {
        return false;
    }
    for (int i = 0; i < count; i++) {
        if (!memchr(items[i].id, 0, sizeof items[i].id) || !*items[i].id ||
            !memchr(items[i].text, 0, sizeof items[i].text) ||
            !memchr(items[i].icon, 0, sizeof items[i].icon) ||
            !memchr(items[i].action, 0, sizeof items[i].action) || items[i].depth < 0 ||
            items[i].depth >= PUI_LAYOUT_NODES ||
            items[i].depth > (i ? items[i - 1].depth + 1 : 0)) {
            return false;
        }
        for (int j = 0; j < i; j++) {
            if (pui_id(items[i].id) == pui_id(items[j].id)) {
                return false;
            }
        }
    }
    TreeData *next = calloc(1, sizeof *next);
    if (!next) {
        return false;
    }
    if (count) {
        next->items = malloc(count * sizeof *items);
        next->open = calloc(count, sizeof *next->open);
        if (!next->items || !next->open) {
            Widget temporary = {.tree = next};
            pui_widget_tree_clear(&temporary);
            return false;
        }
        memcpy(next->items, items, count * sizeof *items);
    }
    next->count = count;
    Widget *state = &view->state[index];
    bool selected_exists = false;
    for (int i = 0; i < count; i++) {
        next->open[i] = items[i].open;
        selected_exists |= state->selected == pui_id(items[i].id);
        if (state->tree) {
            for (int j = 0; j < state->tree->count; j++) {
                if (!strcmp(items[i].id, state->tree->items[j].id)) {
                    next->open[i] = state->tree->open[j];
                }
            }
        }
    }
    pui_widget_tree_clear(state);
    state->tree = next;
    if (!selected_exists) {
        state->selected = 0;
    }
    return true;
}

int pui_view_tree_item_count(const PUIView *view, const char *tree) {
    int index = find_tree(view, tree);
    return index >= 0 && view->state[index].tree ? view->state[index].tree->count : -1;
}

bool pui_view_select_tree_item(PUIView *view, const char *tree, const char *item) {
    int index = find_tree(view, tree);
    if (index < 0) {
        return false;
    }
    if (!item) {
        view->state[index].selected = 0;
        return true;
    }
    TreeData *data = view->state[index].tree;
    if (data) {
        for (int i = 0; i < data->count; i++) {
            if (data->items[i].enabled && !strcmp(data->items[i].id, item)) {
                view->state[index].selected = pui_id(item);
                return true;
            }
        }
    }
    return false;
}
