/** @file @brief View lifetime, widget state ownership and value access. */
#include "internal.h"
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

PUIView *pui_view_new(const PUILayout *layout, SDL_Renderer *renderer) {
    if (!layout || !renderer) {
        return NULL;
    }
    PUIView *view = calloc(1, sizeof *view);
    if (!view) {
        return NULL;
    }
    view->layout = layout;
    view->renderer = renderer;
    view->window = SDL_GetRenderWindow(renderer);
    view->focused = view->submit = -1;
    view->pointer_inside = true;
    for (int i = 0; i < layout->count; i++) {
        const PUINode *n = &layout->nodes[i];
        Widget *state = &view->state[i];
        state->open = n->open;
        state->checked = n->checked;
        state->preferred[0] = n->initial;
        state->preferred[1] = n->initial_second;
        state->preferred[2] = n->initial_last;
        snprintf(state->text, sizeof state->text, "%s", n->text);
        pui_text_edit_init(&state->edit, state->text, false);
        if (*n->image) {
            char path[1600];
            if (snprintf(path, sizeof path, "%s%s", layout->directory, n->image) <
                (int)sizeof path) {
                state->image = pui_image_load(renderer, path);
            }
        }
        if (n->type == PUI_NODE_DROPDOWN) {
            pui_widget_dropdown_init(view, i);
        }
        if (n->type == PUI_NODE_TABS) {
            pui_widget_tabs_init(view, i);
        }
    }
    for (int i = 0; i < layout->count; i++) {
        if (layout->nodes[i].type == PUI_NODE_COLOR_PICKER) {
            pui_widget_color_picker_init(view, i);
        }
    }
    for (int i = 0; i < layout->count; i++) {
        if (layout->nodes[i].type == PUI_NODE_BUTTON_GROUP) {
            pui_widget_button_group_init(view, i);
        }
    }
    return view;
}

void pui_view_free(PUIView *view) {
    if (view) {
        pui_view_free_cursors(view);
        if (view->window && view->focused >= 0) {
            SDL_StopTextInput(view->window);
        }
        for (int i = 0; i < view->layout->count; i++) {
            if (view->layout->nodes[i].type == PUI_NODE_COLOR_PICKER &&
                view->state[i].color.drag >= 0) {
                SDL_CaptureMouse(false);
            }
            pui_widget_tree_clear(&view->state[i]);
            if (view->state[i].image) {
                SDL_DestroyTexture(view->state[i].image);
            }
        }
        pui_layout_free(view->owned_layout);
        free(view);
    }
}

const char *pui_view_text(const PUIView *view, const char *id) {
    if (!view || !id) {
        return NULL;
    }
    for (int i = 0; i < view->layout->count; i++) {
        if ((view->layout->nodes[i].type == PUI_NODE_INPUT ||
             view->layout->nodes[i].type == PUI_NODE_LABEL ||
             view->layout->nodes[i].type == PUI_NODE_TAB) &&
            !strcmp(view->layout->nodes[i].name, id)) {
            return view->state[i].text;
        }
    }
    return NULL;
}

bool pui_view_set_text(PUIView *view, const char *id, const char *text) {
    if (!view || !id || !text || strlen(text) >= sizeof view->state[0].text) {
        return false;
    }
    for (int i = 0; i < view->layout->count; i++) {
        const PUINode *n = &view->layout->nodes[i];
        if (!strcmp(n->name, id) &&
            (n->type == PUI_NODE_INPUT || n->type == PUI_NODE_LABEL || n->type == PUI_NODE_TAB)) {
            char copy[256] = {0};
            PUITextEdit edit = {0};
            if (n->type == PUI_NODE_INPUT &&
                !pui_text_edit_insert(&edit, copy, sizeof copy, text)) {
                return false;
            }
            if (n->type == PUI_NODE_LABEL || n->type == PUI_NODE_TAB) {
                snprintf(copy, sizeof copy, "%s", text);
            }
            if (n->type == PUI_NODE_TAB) {
                for (int j = 0; j < view->layout->count; j++) {
                    if (view->layout->nodes[j].type != PUI_NODE_TABS) {
                        continue;
                    }
                    for (int k = 0; k < view->state[j].tabs.count; k++) {
                        if (view->state[j].tabs.items[k].id == n->id) {
                            view->state[j].tabs.items[k].width =
                                pui_tab_content_width(pui_icon_from_name(n->icon), copy) +
                                PUI_TAB_GAP;
                        }
                    }
                }
            }
            memcpy(view->state[i].text, copy, sizeof copy);
            pui_text_edit_init(&view->state[i].edit, view->state[i].text, false);
            return true;
        }
    }
    return false;
}

static int checkable_node(const PUIView *view, const char *id) {
    if (!view || !id) {
        return -1;
    }
    for (int i = 0; i < view->layout->count; i++) {
        const PUINode *n = &view->layout->nodes[i];
        if (!strcmp(n->name, id) &&
            (n->type == PUI_NODE_TOGGLE || n->type == PUI_NODE_CHECKBOX || view->state[i].group)) {
            return i;
        }
    }
    return -1;
}

bool pui_view_checked(const PUIView *view, const char *id) {
    int i = checkable_node(view, id);
    return i >= 0 && view->state[i].checked;
}

bool pui_view_set_checked(PUIView *view, const char *id, bool checked) {
    int i = checkable_node(view, id);
    if (i < 0) {
        return false;
    }
    int group = view->state[i].group - 1;
    if (group >= 0) {
        if (!checked) {
            return !view->state[i].checked;
        }
        if (!view->layout->nodes[i].enabled) {
            return false;
        }
        view->state[group].selected = view->layout->nodes[i].id;
        for (int k = view->layout->nodes[group].child; k >= 0; k = view->layout->nodes[k].next) {
            view->state[k].checked = k == i;
        }
    } else {
        view->state[i].checked = checked;
    }
    return true;
}

void pui_view_set_canvas_renderer(PUIView *view, PUICanvasRenderer draw, void *context) {
    if (view) {
        view->canvas_renderer = draw;
        view->canvas_context = context;
    }
}

bool pui_view_hit(const PUIView *view, const char *widget, float x, float y) {
    if (view && widget) {
        for (int i = 0; i < view->layout->count; i++) {
            if (!strcmp(view->layout->nodes[i].name, widget)) {
                return view->visible[i] && view->enabled[i] && pui_contains(view->hit[i], x, y);
            }
        }
    }
    return false;
}

const char *pui_view_tab_at(const PUIView *view, const char *tabs, int index) {
    if (!view || !tabs || index < 0) {
        return NULL;
    }
    for (int i = 0; i < view->layout->count; i++) {
        const PUINode *node = &view->layout->nodes[i];
        if (node->type == PUI_NODE_TABS && !strcmp(node->name, tabs)) {
            if (index >= view->state[i].tabs.count) {
                return NULL;
            }
            PUIId id = view->state[i].tabs.items[index].id;
            for (int k = node->child; k >= 0; k = view->layout->nodes[k].next) {
                if (view->layout->nodes[k].id == id) {
                    return view->layout->nodes[k].name;
                }
            }
        }
    }
    return NULL;
}
