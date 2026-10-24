/** @file @brief Text focus is explicit; edits coalesce while the typing session remains active. */
#include "tool.h"
#include "editors/vector/document/selection.h"
#include "editors/vector/geometry/rotation.h"
#include "editors/vector/object/commands.h"
#include <math.h>
#include <string.h>

void vector_text_tool_end(VectorDocument *d) {
    if (d) {
        if (d->text_tool.selecting) {
            SDL_CaptureMouse(false);
        }
        d->text_tool = (VectorTextTool){0};
    }
}

void vector_text_tool_begin(VectorDocument *d, unsigned id, SDL_Window *window) {
    VectorShape *s = vector_rectangle_find(d, id);
    if (!s || !s->text.enabled ||
        vector_rectangle_effective_flags(d, s) & VECTOR_OBJECT_STATE_MASK) {
        return;
    }
    vector_document_select(d, id);
    d->text_tool = (VectorTextTool){.editing_id = id, .history_start = d->history.cursor};
    pui_text_edit_init(&d->text_tool.edit, s->text.content, true);
    SDL_StartTextInput(window);
}

static bool inside(VectorDocument *d, VectorShape *s, float sx, float sy, float *x, float *y) {
    VectorArtboard *board = vector_artboard_find(d, s->parent_id);
    if (!board) {
        return false;
    }
    *x = (sx - d->viewport.bounds.x - d->viewport.x) / d->viewport.zoom - board->rect.x;
    *y = (sy - d->viewport.bounds.y - d->viewport.y) / d->viewport.zoom - board->rect.y;
    vector_rotate_point(x, y, s->rect.x + s->rect.w / 2, s->rect.y + s->rect.h / 2, -s->angle);
    *x -= s->rect.x;
    *y -= s->rect.y;
    return *x >= 0 && *y >= 0 && *x <= s->rect.w && *y <= s->rect.h;
}

static size_t hit(VectorTextRenderer *renderer, VectorShape *s, float x, float y) {
    VectorTextLayout layout;
    vector_text_measure(renderer, s, &layout);
    float best = 1e30f;
    size_t offset = layout.visible_end;
    for (int i = 0; i <= layout.count; i++) {
        float gx = i < layout.count ? layout.glyphs[i].x : layout.end_x;
        float gy = i < layout.count ? layout.glyphs[i].y : layout.end_y;
        float distance = fabsf(gy + s->text.size / 2 - y) * 10000 + fabsf(gx - x);
        if (distance < best) {
            best = distance;
            offset = i < layout.count ? layout.glyphs[i].byte : layout.visible_end;
        }
    }
    return offset;
}

static bool insert(VectorTextTool *tool, VectorShape *s, const char *text) {
    size_t a = tool->edit.cursor < tool->edit.anchor ? tool->edit.cursor : tool->edit.anchor;
    size_t b = tool->edit.cursor > tool->edit.anchor ? tool->edit.cursor : tool->edit.anchor;
    size_t n = strlen(s->text.content), added = strlen(text);
    if (a > n || b > n || n - (b - a) + added >= sizeof s->text.content) {
        return false;
    }
    memmove(s->text.content + a + added, s->text.content + b, n - b + 1);
    memcpy(s->text.content + a, text, added);
    if (!vector_text_valid(&s->text)) {
        return false;
    }
    tool->edit.cursor = tool->edit.anchor = a + added;
    return true;
}

bool vector_text_tool_event(VectorDocument *d, VectorTextRenderer *renderer, PUIView *view,
                            const SDL_Event *e) {
    if (!d) {
        return false;
    }
    VectorShape *s = vector_rectangle_find(d, d->text_tool.editing_id);
    if (e->type == SDL_EVENT_WINDOW_FOCUS_LOST) {
        vector_text_tool_end(d);
        return false;
    }
    bool click = e->type == SDL_EVENT_MOUSE_BUTTON_DOWN && e->button.button == SDL_BUTTON_LEFT;
    char canvas[96];
    snprintf(canvas, sizeof canvas, "vector_document_%u.canvas", d->id);
    bool on_canvas = click && pui_view_hit(view, canvas, e->button.x, e->button.y);
    if (!s && on_canvas && e->button.clicks >= 2) {
        for (int i = d->rectangle_count - 1; i >= 0; i--) {
            VectorShape *candidate = &d->rectangles[i];
            float x, y;
            if (candidate->text.enabled &&
                !(vector_rectangle_effective_flags(d, candidate) & VECTOR_OBJECT_STATE_MASK) &&
                inside(d, candidate, e->button.x, e->button.y, &x, &y)) {
                vector_text_tool_begin(d, candidate->id, SDL_GetWindowFromID(e->button.windowID));
                return true;
            }
        }
    }
    if (!s) {
        return false;
    }
    if (vector_rectangle_effective_flags(d, s) & VECTOR_OBJECT_STATE_MASK ||
        d->active_rectangle != s->id) {
        vector_text_tool_end(d);
        return false;
    }
    VectorTextTool *tool = &d->text_tool;
    if (click) {
        float x, y;
        if (!on_canvas || !inside(d, s, e->button.x, e->button.y, &x, &y)) {
            vector_text_tool_end(d);
            return false;
        }
        size_t offset = hit(renderer, s, x, y);
        if (!(SDL_GetModState() & SDL_KMOD_SHIFT)) {
            tool->edit.anchor = offset;
        }
        tool->edit.cursor = offset;
        if (e->button.clicks >= 2) {
            size_t column = 0, at = 0;
            while (at < offset && s->text.content[at]) {
                vector_text_codepoint(s->text.content, &at);
                column++;
            }
            pui_text_edit_select_word(&tool->edit, s->text.content, column);
        }
        tool->selecting = true;
        SDL_CaptureMouse(true);
        return true;
    }
    if (tool->selecting && e->type == SDL_EVENT_MOUSE_MOTION) {
        float x, y;
        inside(d, s, e->motion.x, e->motion.y, &x, &y);
        tool->edit.cursor = hit(renderer, s, x, y);
        return true;
    }
    if (tool->selecting && e->type == SDL_EVENT_MOUSE_BUTTON_UP) {
        tool->selecting = false;
        SDL_CaptureMouse(false);
        return true;
    }
    if (e->type != SDL_EVENT_TEXT_INPUT && e->type != SDL_EVENT_KEY_DOWN) {
        return false;
    }
    VectorShape after = *s;
    VectorTextTool before = *tool;
    bool changed = false;
    if (e->type == SDL_EVENT_TEXT_INPUT) {
        changed = insert(tool, &after, e->text.text);
    } else {
        SDL_Keycode key = e->key.key;
        bool command = (e->key.mod & (SDL_KMOD_CTRL | SDL_KMOD_GUI)) != 0,
             shift = (e->key.mod & SDL_KMOD_SHIFT) != 0;
        if (key == SDLK_ESCAPE || (key == SDLK_RETURN && command)) {
            vector_text_tool_end(d);
            SDL_StopTextInput(SDL_GetWindowFromID(e->key.windowID));
            return true;
        }
        if (command && (key == SDLK_Z || key == SDLK_Y)) {
            vector_text_tool_end(d);
            return false;
        }
        if (command && key == SDLK_A) {
            pui_text_edit_key(&tool->edit, after.text.content, PUI_TEXT_SELECT_ALL, false);
            return true;
        }
        if (command && (key == SDLK_C || key == SDLK_X)) {
            size_t a = tool->edit.cursor < tool->edit.anchor ? tool->edit.cursor
                                                             : tool->edit.anchor,
                   b = tool->edit.cursor > tool->edit.anchor ? tool->edit.cursor
                                                             : tool->edit.anchor;
            if (a != b) {
                char copy[VECTOR_TEXT_CAPACITY];
                memcpy(copy, after.text.content + a, b - a);
                copy[b - a] = 0;
                if (SDL_SetClipboardText(copy) && key == SDLK_X) {
                    changed =
                        pui_text_edit_key(&tool->edit, after.text.content, PUI_TEXT_DELETE, false);
                }
            }
        } else if (command && key == SDLK_V) {
            char *text = SDL_GetClipboardText();
            if (text) {
                // Normalize CRLF and lone CR from native clipboards to document newlines.
                size_t w = 0;
                for (size_t i = 0; text[i]; i++) {
                    if (text[i] == '\r') {
                        text[w++] = '\n';
                        if (text[i + 1] == '\n') {
                            i++;
                        }
                    } else {
                        text[w++] = text[i];
                    }
                }
                text[w] = 0;
                changed = insert(tool, &after, text);
                SDL_free(text);
            }
        } else if (key == SDLK_RETURN || key == SDLK_KP_ENTER) {
            changed = insert(tool, &after, "\n");
        } else if (key == SDLK_TAB) {
            changed = insert(tool, &after, "\t");
        } else if (key == SDLK_LEFT || key == SDLK_RIGHT || key == SDLK_HOME || key == SDLK_END ||
                   key == SDLK_BACKSPACE || key == SDLK_DELETE) {
            PUITextKey operation = key == SDLK_LEFT        ? PUI_TEXT_LEFT
                                   : key == SDLK_RIGHT     ? PUI_TEXT_RIGHT
                                   : key == SDLK_HOME      ? PUI_TEXT_HOME
                                   : key == SDLK_END       ? PUI_TEXT_END
                                   : key == SDLK_BACKSPACE ? PUI_TEXT_BACKSPACE
                                                           : PUI_TEXT_DELETE;
            changed = pui_text_edit_key(&tool->edit, after.text.content, operation, shift);
        } else if (key == SDLK_UP || key == SDLK_DOWN) {
            VectorTextLayout layout;
            vector_text_measure(renderer, s, &layout);
            float x = layout.end_x, y = layout.end_y;
            for (int i = 0; i < layout.count; i++) {
                if (layout.glyphs[i].byte >= tool->edit.cursor) {
                    x = layout.glyphs[i].x;
                    y = layout.glyphs[i].y;
                    break;
                }
            }
            tool->edit.cursor =
                hit(renderer, s, x,
                    y + s->text.size / 2 +
                        (key == SDLK_UP ? -1 : 1) * s->text.size * s->text.line_height);
            if (!shift) {
                tool->edit.anchor = tool->edit.cursor;
            }
        }
    }
    if (changed && !vector_shape_update_typing(d, &after, tool->history_start)) {
        *tool = before;
        pui_view_set_text(view, "status",
                          "Textaenderung nicht moeglich (History voll oder Text zu lang)");
    }
    return true;
}
