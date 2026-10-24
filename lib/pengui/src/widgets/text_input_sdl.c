/** @file @brief Text-field drawing, focus, clipboard and native input events. */
#include "core/internal.h"
#include "view/internal.h"
#include <math.h>
#include <string.h>

static void draw_text_input(SDL_Renderer *r, SDL_FRect rect, const char *text, PUITextEdit *edit,
                            bool focused, bool enabled, bool cursor_visible, float radius,
                            Uint32 border, Uint32 background) {
    radius = fmaxf(0, fminf(radius, fminf(rect.w, rect.h) * .5f));
    pui_rounded_panel(r, rect, border, radius, false);
    SDL_FRect fill = {rect.x + 1, rect.y + 1, fmaxf(0, rect.w - 2), fmaxf(0, rect.h - 2)};
    pui_rounded_panel(r, fill, background, fmaxf(0, radius - 1), false);
    SDL_Rect previous;
    bool clipped = SDL_RenderClipEnabled(r);
    SDL_GetRenderClipRect(r, &previous);
    SDL_Rect inner = {(int)ceilf(rect.x + 5), (int)ceilf(rect.y + 2), (int)fmaxf(0, rect.w - 10),
                      (int)fmaxf(0, rect.h - 4)};
    if (clipped) {
        SDL_GetRectIntersection(&inner, &previous, &inner);
    }
    SDL_SetRenderClipRect(r, &inner);
    float cursor = (float)pui_text_edit_column(text, edit->cursor) * PUI_FONT_WIDTH;
    float available = fmaxf(0, rect.w - 14);
    if (focused) {
        edit->scroll = fmaxf(0, fminf(edit->scroll, cursor));
        if (cursor - edit->scroll > available) {
            edit->scroll = cursor - available;
        }
    } else {
        edit->scroll = 0;
    }
    float x = rect.x + 7 - edit->scroll, y = rect.y + (rect.h - PUI_FONT_HEIGHT) * .5f;
    if (focused && edit->cursor != edit->anchor) {
        float anchor = (float)pui_text_edit_column(text, edit->anchor) * PUI_FONT_WIDTH;
        pui_panel(
            r, (SDL_FRect){x + fminf(cursor, anchor), y, fabsf(cursor - anchor), PUI_FONT_HEIGHT},
            PUI_HOVER);
    }
    pui_text(r, x, y, text, enabled ? PUI_TEXT : PUI_DISABLED);
    if (focused && cursor_visible) {
        pui_render_color(r, PUI_ACCENT);
        SDL_RenderLine(r, x + cursor, rect.y + 4, x + cursor, rect.y + rect.h - 4);
    }
    SDL_SetRenderClipRect(r, clipped ? &previous : NULL);
}

/* Stateless drawing uses the final focus shape; views retain animation state per field. */
void pui_draw_text_input(SDL_Renderer *r, SDL_FRect rect, const char *text, PUITextEdit *edit,
                         bool focused, bool enabled, bool invalid, bool cursor_visible) {
    draw_text_input(r, rect, text, edit, focused, enabled, cursor_visible,
                    focused ? 0 : pui_theme.button_radius,
                    invalid   ? 0xc97565ffu
                    : focused ? PUI_ACCENT
                              : PUI_BORDER,
                    PUI_CANVAS);
}

/* Column positions are converted by the core into valid UTF-8 byte boundaries. */
static void select_at(PUIView *view, int input, float x, bool extend) {
    Widget *s = &view->state[input];
    float column = (x - view->rect[input].x - 7 + s->edit.scroll) / PUI_FONT_WIDTH;
    if (extend && view->selecting && view->selection_clicks >= 2) {
        if (view->selection_clicks >= 3) {
            pui_text_edit_key(&s->edit, s->text, PUI_TEXT_SELECT_ALL, false);
            return;
        }
        column = fminf((float)strlen(s->text), fmaxf(0, floorf(column)));
        pui_text_edit_select_word(&s->edit, s->text, (size_t)column);
        size_t start = s->edit.anchor, end = s->edit.cursor;
        s->edit.anchor =
            start < view->selection_start ? view->selection_end : view->selection_start;
        s->edit.cursor = start < view->selection_start ? start
                         : end > view->selection_end   ? end
                                                       : view->selection_end;
        return;
    }
    column = fminf((float)strlen(s->text), fmaxf(0, floorf(column + .5f)));
    pui_text_edit_click(&s->edit, s->text, (size_t)column, extend);
}

void pui_widget_text_input_draw(PUIView *v, PUIContext *pui, int i, PUIRect area, PUIAction action,
                                void *context) {
    const PUINode *n = &v->layout->nodes[i];
    Widget *s = &v->state[i];
    pui_numeric_drag_update(v, pui, i);
    bool scrubbing = v->numeric_drag.active && v->numeric_drag.index == i;
    bool selecting = v->selecting && v->focused == i && pui->active == n->id && !pui->blocked;
    if (selecting) {
        pui->active_seen = true;
    }
    if (selecting && pui->down) {
        /* Re-evaluate while held: keeping the cursor visible scrolls long text, which
         * advances the selection on following frames even without more mouse motion. */
        float x = fmaxf(area.x - PUI_FONT_WIDTH, fminf(pui->x, area.x + area.w + PUI_FONT_WIDTH));
        select_at(v, i, x, true);
    }
    if (selecting || ((!pui->active || pui->released) && pui_hovered(pui, area))) {
        pui_view_request_cursor(v, SDL_SYSTEM_CURSOR_TEXT, selecting ? 2 : 1);
    }
    bool focused = v->focused == i;
    float radius = n->radius >= 0 ? n->radius : pui_theme.button_radius;
    float duration = n->transition_ms >= 0 ? n->transition_ms : pui_theme.button_transition_ms;
    Uint32 border = pui_color_transition(&s->button_focus,
                                         s->input_invalid ? 0xc97565ffu
                                         : focused        ? PUI_ACCENT
                                                          : PUI_BORDER,
                                         v->frame_ms, duration);
    draw_text_input(v->renderer, pui_frect(area), s->text, &s->edit, focused, !pui->blocked,
                    !scrubbing && SDL_GetTicks() % 1000 < 500, radius, border,
                    scrubbing ? PUI_HOVER : PUI_CANVAS);
    if (v->submit == i) {
        if (n->input_type == PUI_INPUT_STRING) {
            s->input_invalid = !pui_text_input_valid(n->input_type, s->text, true);
        } else {
            char result[sizeof s->text];
            s->input_invalid = !pui_text_input_evaluate(n->input_type, s->text, s->input_base,
                                                        result, sizeof result);
            if (!s->input_invalid) {
                pui_view_set_text(v, n->name, result);
            }
        }
        if (!s->input_invalid && action && *n->action) {
            action(context, n->action, n->name);
        }
        v->submit = -1;
        if (v->numeric_drag.index == i) {
            v->numeric_drag.committing = false;
        }
    }
}

void pui_view_blur(PUIView *view, PUIContext *pui) {
    bool scrubbing = view->numeric_drag.active;
    pui_numeric_drag_cancel(view);
    if (scrubbing) {
        pui_cancel(pui);
    }
    if (view->focused >= 0) {
        if (view->selecting && pui->active == view->layout->nodes[view->focused].id) {
            pui->active = 0;
        }
        view->selecting = false;
        if (view->window) {
            SDL_StopTextInput(view->window);
        }
        view->focused = -1;
        pui_focus(pui, 0);
    }
}

static void insert_text(PUIView *view, const char *text) {
    Widget *state = &view->state[view->focused];
    if (pui_text_edit_insert_typed(&state->edit, state->text, sizeof state->text, text,
                                   view->layout->nodes[view->focused].input_type)) {
        state->input_invalid = false;
    }
}

static void input_key(PUIView *view, PUITextKey key, bool shift) {
    Widget *state = &view->state[view->focused];
    PUITextEdit edit = state->edit;
    char candidate[sizeof state->text];
    memcpy(candidate, state->text, sizeof candidate);
    bool changed = pui_text_edit_key(&edit, candidate, key, shift);
    if (pui_text_input_expression_valid(view->layout->nodes[view->focused].input_type, candidate)) {
        memcpy(state->text, candidate, sizeof candidate);
        state->edit = edit;
        if (changed) {
            state->input_invalid = false;
        }
    }
}

static bool text_key(PUIView *view, PUIContext *pui, const SDL_KeyboardEvent *key) {
    Widget *state = &view->state[view->focused];
    bool ctrl = (key->mod & (SDL_KMOD_CTRL | SDL_KMOD_GUI)) != 0;
    bool shift = (key->mod & SDL_KMOD_SHIFT) != 0;
    PUITextKey action;
    switch (key->key) {
    case SDLK_LEFT:
        action = ctrl ? PUI_TEXT_HOME : PUI_TEXT_LEFT;
        break;
    case SDLK_RIGHT:
        action = ctrl ? PUI_TEXT_END : PUI_TEXT_RIGHT;
        break;
    case SDLK_HOME:
        action = PUI_TEXT_HOME;
        break;
    case SDLK_END:
        action = PUI_TEXT_END;
        break;
    case SDLK_BACKSPACE:
        action = PUI_TEXT_BACKSPACE;
        break;
    case SDLK_DELETE:
        action = PUI_TEXT_DELETE;
        break;
    case SDLK_A:
        if (!ctrl) {
            return false;
        }
        action = PUI_TEXT_SELECT_ALL;
        break;
    case SDLK_ESCAPE:
        pui_view_blur(view, pui);
        return true;
    case SDLK_RETURN:
    case SDLK_KP_ENTER:
        view->submit = view->focused;
        return true;
    case SDLK_V:
        if (ctrl) {
            char *text = SDL_GetClipboardText();
            if (text) {
                insert_text(view, text);
                SDL_free(text);
            }
            return true;
        }
        return false;
    case SDLK_C:
    case SDLK_X:
        if (ctrl) {
            size_t a =
                state->edit.cursor < state->edit.anchor ? state->edit.cursor : state->edit.anchor;
            size_t b =
                state->edit.cursor > state->edit.anchor ? state->edit.cursor : state->edit.anchor;
            if (a < b && b <= strlen(state->text)) {
                char text[256];
                memcpy(text, state->text + a, b - a);
                text[b - a] = 0;
                if (SDL_SetClipboardText(text) && key->key == SDLK_X) {
                    input_key(view, PUI_TEXT_DELETE, false);
                }
            }
            return true;
        }
        return false;
    default:
        return false;
    }
    input_key(view, action, shift);
    return true;
}

void pui_widget_text_input_pointer(PUIView *view, PUIContext *pui, SDL_Window *window,
                                   const SDL_Event *event) {

    if (pui->active) {
        return;
    }
    int input = -1;
    for (int i = 0; i < view->layout->count; i++) {
        if (view->visible[i] && view->enabled[i] && view->layout->nodes[i].type == PUI_NODE_INPUT &&
            pui_contains(view->hit[i], event->button.x, event->button.y)) {
            input = i;
        }
    }
    if (input != view->focused) {
        if (view->focused >= 0 && pui_widget_color_picker_owns(view, view->focused)) {
            view->submit = view->focused;
        }
        pui_view_blur(view, pui);
    }
    if (input >= 0) {
        view->focused = input;
        pui_focus(pui, view->layout->nodes[input].id);
        SDL_StartTextInput(window);
        select_at(view, input, event->button.x, (SDL_GetModState() & SDL_KMOD_SHIFT) != 0);
        Widget *s = &view->state[input];
        view->selection_clicks = event->button.clicks;
        if (event->button.clicks >= 3) {
            pui_text_edit_key(&s->edit, s->text, PUI_TEXT_SELECT_ALL, false);
        } else if (event->button.clicks == 2) {
            float column =
                (event->button.x - view->rect[input].x - 7 + s->edit.scroll) / PUI_FONT_WIDTH;
            column = fminf((float)strlen(s->text), fmaxf(0, floorf(column)));
            pui_text_edit_select_word(&s->edit, s->text, (size_t)column);
        }
        view->selection_start = s->edit.anchor;
        view->selection_end = s->edit.cursor;
        pui_context_acquire(pui, view->layout->nodes[input].id, view->hit[input]);
        view->selecting = pui->active == view->layout->nodes[input].id;
    }
}

bool pui_widget_text_input_drag(PUIView *view, PUIContext *pui, const SDL_Event *event) {
    if (!view->selecting || view->focused < 0) {
        return false;
    }
    int input = view->focused;
    if (!view->visible[input] || !view->enabled[input] ||
        pui->active != view->layout->nodes[input].id) {
        view->selecting = false;
        return false;
    }
    bool release = event->type == SDL_EVENT_MOUSE_BUTTON_UP;
    float x = release ? event->button.x : event->motion.x;
    select_at(view, input, x, true);
    if (release) {
        /* Releasing over another widget finishes this selection without activating it. */
        pui->active = 0;
        view->selecting = false;
    }
    return true;
}

/* YAML order, restricted to fields visible and enabled in the last rendered frame. */
static bool focus_next_input(PUIView *view, PUIContext *pui, bool backwards) {
    int count = view->layout->count;
    int start = view->focused >= 0 ? view->focused : backwards ? 0 : count - 1;
    for (int step = 1; step <= count; step++) {
        int i = (start + (backwards ? -step : step) + count) % count;
        if (view->layout->nodes[i].type != PUI_NODE_INPUT || !view->visible[i] ||
            !view->enabled[i]) {
            continue;
        }
        if (i != view->focused) {
            /* Commit the field being left before handing focus to its neighbour. */
            if (view->focused >= 0) {
                view->submit = view->focused;
            }
            pui_view_blur(view, pui);
            view->focused = i;
            pui_text_edit_init(&view->state[i].edit, view->state[i].text, true);
        }
        pui_focus(pui, view->layout->nodes[i].id);
        if (view->window) {
            SDL_StartTextInput(view->window);
        }
        return true;
    }
    return false;
}

bool pui_widget_text_input_event(PUIView *view, PUIContext *pui, const SDL_Event *event) {
    if (event->type == SDL_EVENT_KEY_DOWN && event->key.key == SDLK_TAB &&
        !(event->key.mod & (SDL_KMOD_CTRL | SDL_KMOD_GUI | SDL_KMOD_ALT))) {
        if (event->key.repeat) {
            return true;
        }
        return focus_next_input(view, pui, (event->key.mod & SDL_KMOD_SHIFT) != 0);
    }
    if (view->focused < 0) {
        return false;
    }
    if (event->type == SDL_EVENT_TEXT_INPUT) {
        insert_text(view, event->text.text);
        return true;
    }
    return event->type == SDL_EVENT_KEY_DOWN && text_key(view, pui, &event->key);
}
