/** @file @brief Long-press numeric scrubbing; text selection stays available on ordinary drags. */
#include "view/internal.h"
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void pui_numeric_drag_cancel(PUIView *v) {
    if (v->numeric_drag.active) {
        Widget *s = &v->state[v->numeric_drag.index];
        memcpy(s->text, v->numeric_drag.original, sizeof s->text);
        pui_text_edit_init(&s->edit, s->text, false);
        SDL_CaptureMouse(false);
    }
    v->numeric_drag.pending = v->numeric_drag.active = false;
}

void pui_numeric_drag_begin(PUIView *v, const SDL_Event *e) {
    pui_numeric_drag_cancel(v);
    if (v->focused < 0 || e->button.clicks > 1 ||
        v->layout->nodes[v->focused].input_type == PUI_INPUT_STRING) {
        return;
    }
    Widget *s = &v->state[v->focused];
    char evaluated[256];
    if (!pui_text_input_evaluate(v->layout->nodes[v->focused].input_type, s->text, s->input_base,
                                 evaluated, sizeof evaluated)) {
        return;
    }
    char *end;
    double value = strtod(evaluated, &end);
    if (*end || !isfinite(value)) {
        return;
    }
    v->numeric_drag.index = v->focused;
    v->numeric_drag.pending = true;
    v->numeric_drag.started = SDL_GetTicks();
    v->numeric_drag.x = e->button.x;
    v->numeric_drag.y = e->button.y;
    v->numeric_drag.value = value;
    memcpy(v->numeric_drag.original, s->text, sizeof s->text);
}

void pui_numeric_drag_update(PUIView *v, PUIContext *pui, int index) {
    if (!v->numeric_drag.pending || v->numeric_drag.index != index) {
        return;
    }
    if (!pui->down || pui->blocked || v->focused != index ||
        pui->active != v->layout->nodes[index].id) {
        bool active = v->numeric_drag.active;
        pui_numeric_drag_cancel(v);
        if (active) {
            pui_cancel(pui);
        }
        return;
    }
    if (!v->numeric_drag.active && SDL_GetTicks() - v->numeric_drag.started >= 1000) {
        v->numeric_drag.active = true;
        v->selecting = false;
        v->numeric_drag.x = pui->x;
        SDL_CaptureMouse(true);
    }
    if (v->numeric_drag.active) {
        pui->active_seen = true;
        pui_view_request_cursor(v, SDL_SYSTEM_CURSOR_EW_RESIZE, 3);
    }
}

bool pui_numeric_drag_event(PUIView *v, PUIContext *pui, const SDL_Event *e) {
    if (!v->numeric_drag.pending) {
        return false;
    }
    bool active = v->numeric_drag.active;
    int index = v->numeric_drag.index;
    if (!v->visible[index] || !v->enabled[index]) {
        pui_numeric_drag_cancel(v);
        if (active) {
            pui_cancel(pui);
            v->selecting = false;
        }
        return active;
    }
    if (!active && (e->type == SDL_EVENT_KEY_DOWN || e->type == SDL_EVENT_TEXT_INPUT)) {
        v->numeric_drag.pending = false;
        return false;
    }
    if (e->type == SDL_EVENT_WINDOW_FOCUS_LOST ||
        (e->type == SDL_EVENT_KEY_DOWN && e->key.key == SDLK_ESCAPE)) {
        pui_numeric_drag_cancel(v);
        if (active) {
            pui_cancel(pui);
            v->selecting = false;
        }
        return active && e->type != SDL_EVENT_WINDOW_FOCUS_LOST;
    }
    if (e->type == SDL_EVENT_MOUSE_MOTION) {
        if (!active) {
            if (hypotf(e->motion.x - v->numeric_drag.x, e->motion.y - v->numeric_drag.y) > 4) {
                v->numeric_drag.pending = false;
            }
            return false;
        }
        pui_pointer(pui, e->motion.x, e->motion.y, true);
        double steps = trunc((e->motion.x - v->numeric_drag.x) / 6.0);
        double value = v->numeric_drag.value + steps;
        char text[256];
        snprintf(text, sizeof text, "%.15g", value);
        int i = v->numeric_drag.index;
        if (isfinite(value) && pui_text_input_valid(v->layout->nodes[i].input_type, text, true)) {
            snprintf(v->state[i].text, sizeof v->state[i].text, "%s", text);
            pui_text_edit_init(&v->state[i].edit, text, false);
            v->state[i].input_invalid = false;
        }
        return true;
    }
    if (e->type == SDL_EVENT_MOUSE_BUTTON_UP && e->button.button == SDL_BUTTON_LEFT) {
        v->numeric_drag.pending = v->numeric_drag.active = false;
        if (active) {
            int i = v->numeric_drag.index;
            if (strcmp(v->state[i].text, v->numeric_drag.original)) {
                v->submit = i;
            }
            SDL_CaptureMouse(false);
            pui_pointer(pui, e->button.x, e->button.y, false);
            pui->active = 0;
            return true;
        }
    }
    return active && (e->type == SDL_EVENT_KEY_DOWN || e->type == SDL_EVENT_TEXT_INPUT ||
                      e->type == SDL_EVENT_MOUSE_WHEEL);
}
