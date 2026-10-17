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

void ui_draw_tree_guides(SDL_Renderer *r, const int *depths, int count, int row, UiRect area,
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
    ui_render_color(r, rgba);
    for (int level = 1; level < depth && (level - 1) * indent < max_indent; level++) {
        if (ui_tree_continues(depths, count, row, level)) {
            tree_vertical(r, area.x + (level - 1) * indent + marker_center, area.y, 0, area.h,
                          phase);
        }
    }
    if (depth > 0 && (depth - 1) * indent < max_indent) {
        float x = area.x + (depth - 1) * indent + marker_center;
        tree_vertical(r, x, area.y, 0,
                      ui_tree_continues(depths, count, row, depth) ? area.h : center + 1, phase);
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

static int tree_rows(UiView *v, int i, int depth, int *rows, int *depths, int count, bool enabled) {
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

void ui_widget_tree_draw(UiView *v, UiContext *ui, int i, UiRect area, UiAction action,
                         void *context) {
    const UiNode *n = &v->layout->nodes[i];
    Widget *s = &v->state[i];
    int rows[UI_LAYOUT_NODES], depths[UI_LAYOUT_NODES];
    int count = tree_rows(v, i, 0, rows, depths, 0, !ui->blocked);
    s->scroll.content = count * 28.f;
    s->scroll.viewport = area.h;
    ui_scroll_clamp(&s->scroll);
    UiRect track = {area.x + area.w - 8, area.y, 8, area.h};
    ui_scrollbar(ui, n->id ^ 0x65000000u, track, &s->scroll, UI_AXIS_Y);
    for (int k = 0; k < count; k++) {
        int index = rows[k];
        const UiNode *item = &v->layout->nodes[index];
        UiRect r = {area.x, area.y + k * 28 - s->scroll.offset, area.w - 12, 28};
        if (r.y + r.h <= area.y || r.y >= area.y + area.h) {
            continue;
        }
        bool enabled = v->enabled[index];
        ui_panel(v->renderer, ui_frect(r), k % 2 ? UI_SURFACE : UI_HOVER);
        float x = r.x + 8 + depths[k] * 18;
        ui_draw_tree_guides(v->renderer, depths, count, k, r, 18, 13, area.w - 24, x + 12,
                            UI_MUTED);
        UiRect button = {x, r.y, fmaxf(0, r.w - (x - r.x)), r.h};
        UiIcon marker = item->child >= 0 ? (v->state[index].open ? ICON_DOWN : ICON_RIGHT)
                                         : ui_icon_from_name(item->icon);
        Uint32 foreground = enabled ? UI_TEXT : UI_DISABLED;
        ui_icon(v->renderer, marker, x + 4, r.y + 6, 16, foreground);
        ui_text(v->renderer, x + 26, r.y + 6, item->text, foreground);
        if (ui_button(ui, item->id, button, enabled)) {
            if (item->child >= 0) {
                v->state[index].open = !v->state[index].open;
            }
            if (action && *item->action) {
                action(context, item->action, item->name);
            }
        }
    }
    ui_draw_scrollbar(v->renderer, ui, n->id ^ 0x65000000u, track, &s->scroll, UI_AXIS_Y);
}

bool ui_widget_tree_scroll(UiView *view, int i, const SDL_Event *event) {
    if (view->layout->nodes[i].type != UI_NODE_TREE) {
        return false;
    }
    ui_scroll_by(&view->state[i].scroll, -event->wheel.y * 48);
    return true;
}
