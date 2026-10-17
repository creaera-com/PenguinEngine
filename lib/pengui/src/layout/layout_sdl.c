/** @file @brief Natural sizes, rows, columns and container placement. */
#include "view/internal.h"
#include <math.h>
#include <string.h>

float ui_view_node_height(const UiView *v, int i) {
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
                height += fmaxf(30, ui_view_node_height(v, k)) + n->gap;
            }
        }
        return height;
    }
    return 0;
}

void ui_view_draw_children(UiView *v, UiContext *ui, int i, UiRect area, bool row, UiAction action,
                           void *context) {
    const UiNode *n = &v->layout->nodes[i];
    float fixed = 0;
    int count = 0, flexible = 0;
    for (int k = n->child; k >= 0; k = v->layout->nodes[k].next) {
        const UiNode *child = &v->layout->nodes[k];
        float size = row ? child->width : ui_view_node_height(v, k);
        if (row && !size && child->type == UI_NODE_BUTTON) {
            size = ui_tab_content_width(ui_icon_from_name(child->icon), child->text);
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
        float size = row ? child->width : ui_view_node_height(v, k);
        if (row && !size && child->type == UI_NODE_BUTTON) {
            size = ui_tab_content_width(ui_icon_from_name(child->icon), child->text);
        } else if (row && child->type == UI_NODE_SEPARATOR) {
            size = 18;
        }
        if (!size) {
            size = share;
        }
        ui_view_draw_node(v, ui, k,
                          (UiRect){area.x + (row ? offset : 0), area.y + (row ? 0 : offset),
                                   row ? size : area.w, row ? area.h : size},
                          action, context);
        offset += size + n->gap;
    }
}

void ui_view_draw_container(UiView *v, UiContext *ui, int i, UiRect area, UiAction action,
                            void *context) {
    const UiNode *n = &v->layout->nodes[i];
    if (n->type == UI_NODE_PANEL || n->type == UI_NODE_TOOLBAR) {
        ui_panel(v->renderer, ui_frect(area), UI_PANEL);
    }
    if (n->type == UI_NODE_PANEL && *n->text) {
        ui_text(v->renderer, area.x + 12, area.y + 10, n->text, UI_MUTED);
        area.y += 32;
        area.h -= 32;
    }
    area.x += n->padding;
    area.y += n->padding;
    area.w = fmaxf(0, area.w - 2 * n->padding);
    area.h = fmaxf(0, area.h - 2 * n->padding);
    ui_view_draw_children(v, ui, i, area, n->type == UI_NODE_ROW || n->type == UI_NODE_TOOLBAR,
                          action, context);
}
