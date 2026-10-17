#include "internal.h"
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static SDL_FRect frect(UiRect r) {
    return (SDL_FRect){r.x, r.y, r.w, r.h};
}

UiIcon ui_view_icon(const char *name) {
    static const struct {
        const char *name;
        UiIcon value;
    } icons[] = {{"menu", ICON_MENU},       {"save", ICON_SAVE},     {"undo", ICON_UNDO},
                 {"redo", ICON_REDO},       {"folder", ICON_FOLDER}, {"file", ICON_FILE},
                 {"plus", ICON_PLUS},       {"delete", ICON_DELETE}, {"edit", ICON_EDIT},
                 {"check", ICON_CHECK},     {"close", ICON_CLOSE},   {"grid", ICON_GRID},
                 {"refresh", ICON_REFRESH}, {"image", ICON_IMAGE},   {"settings", ICON_LAYERS}};

    for (size_t i = 0; i < sizeof icons / sizeof icons[0]; i++) {
        if (!strcmp(name, icons[i].name)) {
            return icons[i].value;
        }
    }
    return ICON_NONE;
}

static float natural_height(const UiView *v, int i) {
    const UiNode *n = &v->layout->nodes[i];
    if (n->height > 0) {
        return n->height;
    }
    if (n->type == UI_NODE_LABEL) {
        return 24;
    }
    if (n->type == UI_NODE_BUTTON || n->type == UI_NODE_INPUT) {
        return 30;
    }
    if (n->type == UI_NODE_TOOLBAR) {
        return 42;
    }
    if (n->type == UI_NODE_SEPARATOR) {
        return 18;
    }
    if (n->type == UI_NODE_SECTION) {
        float height = 30;
        if (v->state[i].open) {
            height += n->padding * 2;
            for (int k = n->child; k >= 0; k = v->layout->nodes[k].next) {
                height += fmaxf(30, natural_height(v, k)) + n->gap;
            }
        }
        return height;
    }
    return 0;
}

static void draw(UiView *v, UiContext *ui, int i, UiRect area, UiAction action, void *context);

static void children(UiView *v, UiContext *ui, int i, UiRect area, bool row, UiAction action,
                     void *context) {
    const UiNode *n = &v->layout->nodes[i];
    float fixed = 0;
    int count = 0, flexible = 0;
    for (int k = n->child; k >= 0; k = v->layout->nodes[k].next) {
        const UiNode *child = &v->layout->nodes[k];
        float size = row ? child->width : natural_height(v, k);
        if (row && !size && child->type == UI_NODE_BUTTON) {
            size = ui_tab_content_width(ui_view_icon(child->icon), child->text);
        } else if (row && child->type == UI_NODE_SEPARATOR) {
            size = 18;
        }
        if (size > 0) {
            fixed += size;
        } else {
            flexible++;
        }
        count++;
    }
    float share = fmaxf(0, (row ? area.w : area.h) - fixed - n->gap * fmaxf(0, count - 1)) /
                  (flexible ? flexible : 1);
    float offset = 0;
    for (int k = n->child; k >= 0; k = v->layout->nodes[k].next) {
        const UiNode *child = &v->layout->nodes[k];
        float size = row ? child->width : natural_height(v, k);
        if (row && !size && child->type == UI_NODE_BUTTON) {
            size = ui_tab_content_width(ui_view_icon(child->icon), child->text);
        } else if (row && child->type == UI_NODE_SEPARATOR) {
            size = 18;
        }
        if (!size) {
            size = share;
        }
        draw(v, ui, k,
             (UiRect){area.x + (row ? offset : 0), area.y + (row ? 0 : offset), row ? size : area.w,
                      row ? area.h : size},
             action, context);
        offset += size + n->gap;
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

static void tree(UiView *v, UiContext *ui, int i, UiRect area, UiAction action, void *context) {
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
        ui_panel(v->renderer, frect(r), k % 2 ? UI_SURFACE : UI_HOVER);
        float x = r.x + 8 + depths[k] * 18;
        ui_draw_tree_guides(v->renderer, depths, count, k, r, 18, 13, area.w - 24, x + 12,
                            UI_MUTED);
        UiRect button = {x, r.y, fmaxf(0, r.w - (x - r.x)), r.h};
        UiIcon marker = item->child >= 0 ? (v->state[index].open ? ICON_DOWN : ICON_RIGHT)
                                         : ui_view_icon(item->icon);
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

static void draw(UiView *v, UiContext *ui, int i, UiRect area, UiAction action, void *context) {
    if (area.w <= 0 || area.h <= 0) {
        return;
    }
    const UiNode *n = &v->layout->nodes[i];
    Widget *s = &v->state[i];
    bool blocked = ui->blocked;
    ui->blocked = blocked || !n->enabled;
    ui_clip_push(ui, area);
    ui_apply_clip(v->renderer, ui);
    v->rect[i] = area;
    v->hit[i] = ui->clips[ui->clip_depth - 1];
    v->visible[i] = v->hit[i].w > 0 && v->hit[i].h > 0;
    v->enabled[i] = !ui->blocked;
    if (n->type == UI_NODE_PANEL || n->type == UI_NODE_TOOLBAR || n->type == UI_NODE_SECTION) {
        ui_panel(v->renderer, frect(area), n->type == UI_NODE_SECTION ? UI_SURFACE : UI_PANEL);
    }
    if (n->type == UI_NODE_COLUMN || n->type == UI_NODE_ROW || n->type == UI_NODE_TOOLBAR ||
        n->type == UI_NODE_PANEL || n->type == UI_NODE_TAB || n->type == UI_NODE_SECTION) {
        bool show = true;
        if (n->type == UI_NODE_SECTION) {
            UiRect header = {area.x, area.y, area.w, 28};
            ui_disclosure(ui, n->id, header, &s->open, !ui->blocked);
            ui_draw_button(v->renderer, frect(header), s->open ? ICON_DOWN : ICON_RIGHT, n->text,
                           false, !ui->blocked, ui_hovered(ui, header));
            show = s->open;
            area.y += 30;
            area.h -= 30;
        } else if (n->type == UI_NODE_PANEL && *n->text) {
            ui_text(v->renderer, area.x + 12, area.y + 10, n->text, UI_MUTED);
            area.y += 32;
            area.h -= 32;
        }
        area.x += n->padding;
        area.y += n->padding;
        area.w = fmaxf(0, area.w - 2 * n->padding);
        area.h = fmaxf(0, area.h - 2 * n->padding);
        if (show) {
            children(v, ui, i, area, n->type == UI_NODE_ROW || n->type == UI_NODE_TOOLBAR, action,
                     context);
        }
    } else if (n->type == UI_NODE_SPLIT) {
        float total = n->axis == UI_AXIS_X ? area.w : area.h;
        UiSplit split = ui_split_rects(area, n->axis, total - n->gap - s->extent, n->minimum_first,
                                       n->minimum_second, n->gap);
        s->extent = n->axis == UI_AXIS_X ? split.first.w : split.first.h;
        ui_drag_value(ui, n->id, split.grip, &s->extent, fminf(n->minimum_first, s->extent),
                      fmaxf(s->extent, total - n->gap - n->minimum_second), n->axis, 1);
        split = ui_split_rects(area, n->axis, total - n->gap - s->extent, n->minimum_first,
                               n->minimum_second, n->gap);
        draw(v, ui, n->child, split.first, action, context);
        draw(v, ui, v->layout->nodes[n->child].next, split.second, action, context);
        float x = split.grip.x + split.grip.w / 2, y = split.grip.y + split.grip.h / 2;
        for (int dot = -1; dot <= 1; dot++) {
            ui_panel(v->renderer,
                     (SDL_FRect){x + (n->axis == UI_AXIS_Y ? dot * 6 : 0) - 1,
                                 y + (n->axis == UI_AXIS_X ? dot * 6 : 0) - 1, 2, 2},
                     ui_hovered(ui, split.grip) ? UI_ACCENT : UI_MUTED);
        }
    } else if (n->type == UI_NODE_TABS) {
        UiRect bar = {area.x, area.y, area.w, 30};
        ui_tabs_update(ui, &s->tabs, bar, 100);
        for (int k = 0; k < s->tabs.count; k++) {
            const UiNode *tab = NULL;
            for (int c = n->child; c >= 0; c = v->layout->nodes[c].next) {
                if (v->layout->nodes[c].id == s->tabs.items[k].id) {
                    tab = &v->layout->nodes[c];
                    break;
                }
            }
            UiRect r = ui_tab_rect(&s->tabs, bar, 100, k);
            ui_draw_tab(v->renderer, frect(r), ui_view_icon(tab->icon), tab->text,
                        s->tabs.selected == tab->id, !ui->blocked && tab->enabled,
                        ui_hovered(ui, r));
        }
        for (int k = n->child; k >= 0; k = v->layout->nodes[k].next) {
            if (v->layout->nodes[k].id == s->tabs.selected) {
                draw(v, ui, k, (UiRect){area.x, area.y + 32, area.w, fmaxf(0, area.h - 32)}, action,
                     context);
            }
        }
    } else if (n->type == UI_NODE_BUTTON) {
        bool hover = ui_hovered(ui, area);
        if (s->image) {
            ui_draw_image_button(v->renderer, frect(area), s->image, n->text, n->image_padding,
                                 false, !ui->blocked, hover, ui->active == n->id && ui->down,
                                 ui->focus == n->id);
        } else {
            ui_draw_button(v->renderer, frect(area), *n->image ? ICON_IMAGE : ui_view_icon(n->icon),
                           n->text, false, !ui->blocked, hover);
        }
        if (ui_button(ui, n->id, area, !ui->blocked) && action && *n->action) {
            action(context, n->action, n->name);
        }
    } else if (n->type == UI_NODE_LABEL) {
        ui_text(v->renderer, area.x + 4, area.y + fmaxf(0, (area.h - 16) / 2), s->text, UI_TEXT);
    } else if (n->type == UI_NODE_SEPARATOR) {
        ui_panel(v->renderer, (SDL_FRect){area.x + area.w / 2, area.y + 4, 1, fmaxf(0, area.h - 8)},
                 UI_BORDER);
    } else if (n->type == UI_NODE_INPUT) {
        ui_draw_text_input(v->renderer, frect(area), s->text, &s->edit, v->focused == i,
                           !ui->blocked, false, SDL_GetTicks() % 1000 < 500);
        if (v->submit == i) {
            if (action && *n->action) {
                action(context, n->action, n->name);
            }
            v->submit = -1;
        }
    } else if (n->type == UI_NODE_TREE) {
        tree(v, ui, i, area, action, context);
    }
    ui->blocked = blocked;
    ui_clip_pop(ui);
    ui_apply_clip(v->renderer, ui);
}

void ui_view_draw(UiView *view, UiContext *ui, UiRect area, UiAction action, void *context) {
    memset(view->visible, 0, sizeof view->visible);
    draw(view, ui, view->layout->root, area, action, context);
    if (view->focused >= 0 && (!view->visible[view->focused] || !view->enabled[view->focused])) {
        ui_view_blur(view, ui);
    }
    ui_end_frame(ui);
}
