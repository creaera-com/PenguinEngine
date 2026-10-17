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
    for (int i = 0; i < layout->count; i++) {
        const PUINode *n = &layout->nodes[i];
        Widget *state = &view->state[i];
        state->open = n->open;
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
        if (n->type == PUI_NODE_TABS) {
            pui_widget_tabs_init(view, i);
        }
    }
    return view;
}

void pui_view_free(PUIView *view) {
    if (view) {
        if (view->window && view->focused >= 0) {
            SDL_StopTextInput(view->window);
        }
        for (int i = 0; i < view->layout->count; i++) {
            if (view->state[i].image) {
                SDL_DestroyTexture(view->state[i].image);
            }
        }
        free(view);
    }
}

const char *pui_view_text(const PUIView *view, const char *id) {
    if (!view || !id) {
        return NULL;
    }
    for (int i = 0; i < view->layout->count; i++) {
        if (view->layout->nodes[i].type == PUI_NODE_INPUT &&
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
        if (!strcmp(n->name, id) && (n->type == PUI_NODE_INPUT || n->type == PUI_NODE_LABEL)) {
            char copy[256] = {0};
            PUITextEdit edit = {0};
            if (n->type == PUI_NODE_INPUT &&
                !pui_text_edit_insert(&edit, copy, sizeof copy, text)) {
                return false;
            }
            if (n->type == PUI_NODE_LABEL) {
                snprintf(copy, sizeof copy, "%s", text);
            }
            memcpy(view->state[i].text, copy, sizeof copy);
            pui_text_edit_init(&view->state[i].edit, view->state[i].text, false);
            return true;
        }
    }
    return false;
}
