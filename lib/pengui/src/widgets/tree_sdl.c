/** @file @brief Tree rows, disclosure state, guide drawing and scrolling. */
#include "view/internal.h"
#include <math.h>
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

void pui_widget_tree_draw(PUIView *v, PUIContext *pui, int i, PUIRect area, PUIAction action,
                          void *context) {
    const PUINode *n = &v->layout->nodes[i];
    Widget *s = &v->state[i];
    int rows[PUI_LAYOUT_NODES], depths[PUI_LAYOUT_NODES];
    int count = tree_rows(v, i, 0, rows, depths, 0, !pui->blocked);
    s->scroll.content = count * 28.f;
    s->scroll.viewport = area.h;
    pui_scroll_clamp(&s->scroll);
    PUIRect track = {area.x + area.w - 8, area.y, 8, area.h};
    pui_scrollbar(pui, n->id ^ 0x65000000u, track, &s->scroll, PUI_AXIS_Y);
    for (int k = 0; k < count; k++) {
        int index = rows[k];
        const PUINode *item = &v->layout->nodes[index];
        PUIRect r = {area.x, area.y + k * 28 - s->scroll.offset, area.w - 12, 28};
        if (r.y + r.h <= area.y || r.y >= area.y + area.h) {
            continue;
        }
        bool enabled = v->enabled[index];
        if (pui_button(pui, item->id, r, enabled)) {
            s->selected = item->id;
            if (item->child >= 0) {
                v->state[index].open = !v->state[index].open;
            }
            if (action && *item->action) {
                action(context, item->action, item->name);
            }
        }
    }
    /* Resolve selection before painting so the previous row clears in the same frame. */
    for (int k = 0; k < count; k++) {
        int index = rows[k];
        const PUINode *item = &v->layout->nodes[index];
        PUIRect r = {area.x, area.y + k * 28 - s->scroll.offset, area.w - 12, 28};
        if (r.y + r.h <= area.y || r.y >= area.y + area.h) {
            continue;
        }
        bool enabled = v->enabled[index];
        pui_panel(v->renderer, pui_frect(r),
                  s->selected == item->id || (enabled && pui_hovered(pui, r)) ? PUI_HOVER
                  : k % 2                                                     ? PUI_SURFACE
                                                                              : PUI_HOVER);
        if (s->selected == item->id) {
            pui_panel(v->renderer, (SDL_FRect){r.x, r.y, 3, r.h}, PUI_ACCENT);
        }
        float x = r.x + 8 + depths[k] * 18;
        pui_draw_tree_guides(v->renderer, depths, count, k, r, 18, 13, area.w - 24, x + 12,
                             PUI_MUTED);
        PUIIcon marker = item->child >= 0 ? (v->state[index].open ? ICON_DOWN : ICON_RIGHT)
                                          : pui_icon_from_name(item->icon);
        Uint32 foreground = enabled ? PUI_TEXT : PUI_DISABLED;
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
