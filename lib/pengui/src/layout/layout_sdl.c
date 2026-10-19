/** @file @brief Natural sizes, rows, columns and container placement. */
#include "view/internal.h"
#include <math.h>
#include <string.h>

float pui_view_node_height(const PUIView *v, int i) {
    const PUINode *n = &v->layout->nodes[i];
    if (n->height > 0) {
        return n->height;
    }
    if (n->type == PUI_NODE_COLOR_PICKER) {
        return 280;
    }
    if (n->type == PUI_NODE_LABEL) {
        return 24;
    }
    if (n->type == PUI_NODE_BUTTON_GROUP) {
        int count = 0;
        for (int k = n->child; k >= 0; k = v->layout->nodes[k].next) {
            count++;
        }
        return 2 * n->padding + (n->axis == PUI_AXIS_Y ? count * 30 + (count - 1) * n->gap : 30);
    }
    if (n->type == PUI_NODE_BUTTON || n->type == PUI_NODE_INPUT || n->type == PUI_NODE_TOGGLE ||
        n->type == PUI_NODE_CHECKBOX || n->type == PUI_NODE_DROPDOWN ||
        n->type == PUI_NODE_POPUP_BUTTON) {
        return 30;
    }
    if (n->type == PUI_NODE_TOOLBAR) {
        return 42;
    }
    if (n->type == PUI_NODE_SEPARATOR) {
        return 18;
    }
    if (n->type == PUI_NODE_SECTION) {
        float height = 30;
        if (v->state[i].open) {
            height += n->padding * 2;
            for (int k = n->child; k >= 0; k = v->layout->nodes[k].next) {
                height += fmaxf(30, pui_view_node_height(v, k)) + n->gap;
            }
        }
        return height;
    }
    return 0;
}

void pui_view_draw_children(PUIView *v, PUIContext *pui, int i, PUIRect area, bool row,
                            PUIAction action, void *context) {
    const PUINode *n = &v->layout->nodes[i];
    float fixed = 0;
    int count = 0, flexible = 0;
    for (int k = n->child; k >= 0; k = v->layout->nodes[k].next) {
        const PUINode *child = &v->layout->nodes[k];
        if (child->type == PUI_NODE_RADIAL_MENU) {
            continue;
        }
        float size = row ? child->width : pui_view_node_height(v, k);
        if (row && !size && child->type == PUI_NODE_BUTTON) {
            size = pui_tab_content_width(pui_icon_from_name(child->icon), child->text);
        } else if (row && child->type == PUI_NODE_SEPARATOR) {
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
        const PUINode *child = &v->layout->nodes[k];
        if (child->type == PUI_NODE_RADIAL_MENU) {
            continue;
        }
        float size = row ? child->width : pui_view_node_height(v, k);
        if (row && !size && child->type == PUI_NODE_BUTTON) {
            size = pui_tab_content_width(pui_icon_from_name(child->icon), child->text);
        } else if (row && child->type == PUI_NODE_SEPARATOR) {
            size = 18;
        }
        if (!size) {
            size = share;
        }
        pui_view_draw_node(v, pui, k,
                           (PUIRect){area.x + (row ? offset : 0), area.y + (row ? 0 : offset),
                                     row ? size : area.w, row ? area.h : size},
                           action, context);
        offset += size + n->gap;
    }
}

void pui_view_draw_container(PUIView *v, PUIContext *pui, int i, PUIRect area, PUIAction action,
                             void *context) {
    const PUINode *n = &v->layout->nodes[i];
    if (n->type == PUI_NODE_PANEL || n->type == PUI_NODE_TOOLBAR) {
        pui_panel(v->renderer, pui_frect(area), PUI_PANEL);
    }
    if (n->type == PUI_NODE_PANEL && *n->text) {
        pui_text(v->renderer, area.x + 12, area.y + 10, n->text, PUI_MUTED);
        area.y += 32;
        area.h -= 32;
    }
    area.x += n->padding;
    area.y += n->padding;
    area.w = fmaxf(0, area.w - 2 * n->padding);
    area.h = fmaxf(0, area.h - 2 * n->padding);
    pui_view_draw_children(v, pui, i, area, n->type == PUI_NODE_ROW || n->type == PUI_NODE_TOOLBAR,
                           action, context);
}
