/** @file @brief Composite HSL/RGBA picker using ordinary focusable text fields. */
#include "core/internal.h"
#include "view/internal.h"
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct {
    PUIView *view;
    int index;
} ColorBinding;

static int fields(const PUIView *view, int index, int out[9]) {
    int count = 0;
    for (int i = view->layout->nodes[index].child; i >= 0 && count < 9;
         i = view->layout->nodes[i].next) {
        out[count++] = i;
    }
    return count;
}

static void sync_fields(PUIView *view, int index, int skip) {
    int nodes[9];
    if (fields(view, index, nodes) != 9) {
        return;
    }
    PUIColorPickerState *color = &view->state[index].color;
    float values[8] = {color->hsla.h,
                       color->hsla.s * 100,
                       color->hsla.l * 100,
                       color->hsla.a * 100,
                       (float)(color->rgba >> 24),
                       (float)((color->rgba >> 16) & 255),
                       (float)((color->rgba >> 8) & 255),
                       (float)(color->rgba & 255)};
    for (int j = 0; j < 9; j++) {
        if (nodes[j] == skip) {
            continue;
        }
        char text[32];
        if (j == 8) {
            pui_color_format_hex(color->rgba, text);
        } else {
            snprintf(text, sizeof text, j < 4 ? "%.2f" : "%.0f", (double)values[j]);
        }
        pui_view_set_text(view, view->layout->nodes[nodes[j]].name, text);
    }
}

static void assign(PUIColorPickerState *color, uint32_t rgba) {
    float hue = color->hsla.h;
    color->rgba = rgba;
    color->hsla = pui_color_to_hsl(rgba);
    if (color->hsla.s == 0) {
        color->hsla.h = hue;
    }
}

void pui_widget_color_picker_init(PUIView *view, int index) {
    PUIColorPickerState *color = &view->state[index].color;
    *color = (PUIColorPickerState){.mode = view->layout->nodes[index].color_mode, .drag = -1};
    uint32_t rgba = 0xffffffff;
    pui_color_parse_hex(view->layout->nodes[index].value, &rgba);
    assign(color, rgba);
    color->committed = rgba;
    sync_fields(view, index, -1);
}

void pui_widget_color_picker_cancel(PUIView *view, int index) {
    PUIColorPickerState *color = &view->state[index].color;
    if (color->drag >= 0) {
        SDL_CaptureMouse(false);
    }
    assign(color, color->committed);
    color->drag = -1;
    color->notify_commit = false;
    color->notify_preview = true;
    color->invalid = false;
    sync_fields(view, index, -1);
}

static int find(const PUIView *view, const char *id) {
    if (view && id) {
        for (int i = 0; i < view->layout->count; i++) {
            if (view->layout->nodes[i].type == PUI_NODE_COLOR_PICKER &&
                !strcmp(view->layout->nodes[i].name, id)) {
                return i;
            }
        }
    }
    return -1;
}

bool pui_view_color(const PUIView *view, const char *id, uint32_t *rgba) {
    int i = find(view, id);
    if (i < 0 || !rgba) {
        return false;
    }
    *rgba = view->state[i].color.rgba;
    return true;
}

bool pui_view_color_editing(const PUIView *view, const char *id) {
    int i = find(view, id);
    return i >= 0 && view->state[i].color.drag >= 0;
}

bool pui_view_set_color(PUIView *view, const char *id, uint32_t rgba) {
    int i = find(view, id);
    if (i < 0) {
        return false;
    }
    PUIColorPickerState *color = &view->state[i].color;
    if (color->drag >= 0) {
        SDL_CaptureMouse(false);
    }
    assign(color, rgba);
    color->committed = rgba;
    color->drag = -1;
    color->notify_preview = color->notify_commit = color->invalid = false;
    sync_fields(view, i, -1);
    return true;
}

bool pui_widget_color_picker_owns(const PUIView *view, int input) {
    for (int i = 0; i < view->layout->count; i++) {
        if (view->layout->nodes[i].type != PUI_NODE_COLOR_PICKER) {
            continue;
        }
        for (int j = view->layout->nodes[i].child; j >= 0; j = view->layout->nodes[j].next) {
            if (j == input) {
                return true;
            }
        }
    }
    return false;
}

bool pui_widget_color_picker_event(PUIView *view, PUIContext *input, const SDL_Event *event) {
    bool escape = event->type == SDL_EVENT_KEY_DOWN && event->key.key == SDLK_ESCAPE;
    if (!escape && event->type != SDL_EVENT_WINDOW_FOCUS_LOST) {
        return false;
    }
    bool consumed = false;
    for (int i = 0; i < view->layout->count; i++) {
        if (view->layout->nodes[i].type != PUI_NODE_COLOR_PICKER) {
            continue;
        }
        bool focused = false;
        for (int j = view->layout->nodes[i].child; j >= 0; j = view->layout->nodes[j].next) {
            focused |= j == view->focused;
        }
        if (view->state[i].color.drag >= 0 || focused) {
            pui_widget_color_picker_cancel(view, i);
            view->submit = -1;
            pui_cancel(input);
            pui_view_blur(view, input);
            consumed = true;
        }
    }
    return consumed;
}

static bool number(const char *text, float maximum, float *out, bool integer) {
    if (!text || !*text) {
        return false;
    }
    char *end;
    errno = 0;
    float value = strtof(text, &end);
    if (errno || end == text || *end || !isfinite(value) || value < 0 || value > maximum ||
        (integer && value != floorf(value))) {
        return false;
    }
    *out = value;
    return true;
}

static void submit_field(PUIView *view, int index, int field) {
    int nodes[9];
    if (fields(view, index, nodes) != 9) {
        return;
    }
    PUIColorPickerState *color = &view->state[index].color;
    const char *text = view->state[nodes[field]].text;
    float value = 0;
    uint32_t rgba = color->rgba;
    bool valid = field == 8 ? pui_color_parse_hex(text, &rgba)
                            : number(text,
                                     field == 0  ? 360
                                     : field < 4 ? 100
                                                 : 255,
                                     &value, field >= 4);
    if (valid) {
        if (field < 4) {
            if (field == 0) {
                color->hsla.h = fmodf(value, 360);
            } else if (field == 1) {
                color->hsla.s = value / 100;
            } else if (field == 2) {
                color->hsla.l = value / 100;
            } else {
                color->hsla.a = value / 100;
            }
            color->rgba = pui_color_from_hsl(color->hsla);
        } else {
            if (field < 8) {
                unsigned shift = (7 - field) * 8;
                rgba = (rgba & ~(255u << shift)) | ((uint32_t)value << shift);
            }
            assign(color, rgba);
        }
        color->notify_commit |= color->rgba != color->committed;
        color->committed = color->rgba;
    }
    color->invalid = !valid;
    sync_fields(view, index, view->focused);
}

static void field_action(void *context, const char *action, const char *widget) {
    (void)action;
    ColorBinding *binding = context;
    int nodes[9];
    if (fields(binding->view, binding->index, nodes) != 9) {
        return;
    }
    for (int i = 0; i < 9; i++) {
        if (!strcmp(binding->view->layout->nodes[nodes[i]].name, widget)) {
            submit_field(binding->view, binding->index, i);
            return;
        }
    }
}

static void checker(SDL_Renderer *renderer, PUIRect rect) {
    for (float y = rect.y; y < rect.y + rect.h; y += 6) {
        for (float x = rect.x; x < rect.x + rect.w; x += 6) {
            int column = (int)((x - rect.x) / 6), row = (int)((y - rect.y) / 6);
            pui_panel(
                renderer,
                (SDL_FRect){x, y, fminf(6, rect.x + rect.w - x), fminf(6, rect.y + rect.h - y)},
                (row + column) % 2 ? 0x888888ff : 0x555555ff);
        }
    }
}

static void fill(SDL_Renderer *renderer, PUIRect rect, uint32_t rgba) {
    SDL_SetRenderDrawColor(renderer, rgba >> 24, (rgba >> 16) & 255, (rgba >> 8) & 255, rgba & 255);
    SDL_FRect r = {rect.x, rect.y, rect.w, rect.h};
    SDL_RenderFillRect(renderer, &r);
}

static uint32_t sample(const PUIColorPickerState *color, int channel, float t) {
    if (color->mode == 0 && channel < 3) {
        PUIHSLColor value = color->hsla;
        value.a = 1;
        if (channel == 0) {
            value.h = t * 360;
            value.s = 1;
            value.l = .5f;
        } else if (channel == 1) {
            value.s = t;
        } else {
            value.l = t;
        }
        return pui_color_from_hsl(value);
    }
    unsigned shift = (3 - channel) * 8;
    uint32_t rgba = (color->rgba & ~(255u << shift)) | ((uint32_t)lroundf(t * 255) << shift);
    return channel == 3 ? rgba : rgba | 255u;
}

static float component(const PUIColorPickerState *color, int channel) {
    if (color->mode == 0) {
        return channel == 0   ? color->hsla.h / 360
               : channel == 1 ? color->hsla.s
               : channel == 2 ? color->hsla.l
                              : color->hsla.a;
    }
    return ((color->rgba >> ((3 - channel) * 8)) & 255) / 255.f;
}

static void slider(PUIView *view, PUIContext *input, int index, int channel, PUIRect bar) {
    PUIColorPickerState *color = &view->state[index].color;
    char name[96];
    snprintf(name, sizeof name, "%s.slider_%d", view->layout->nodes[index].name, channel);
    PUIId id = pui_id(name);
    PUIRect hit = {bar.x, bar.y - 3, bar.w, bar.h + 6};
    if (input->blocked && color->drag >= 0) {
        pui_widget_color_picker_cancel(view, index);
        pui_cancel(input);
    }
    if (color->drag < 0 && input->active == id) {
        input->active = 0;
    }
    if (color->drag == channel && input->active != id) {
        pui_widget_color_picker_cancel(view, index);
    }
    if (!input->blocked && pui_context_acquire(input, id, hit)) {
        color->drag = channel;
        SDL_CaptureMouse(true);
        pui_focus(input, id);
    }
    if (color->drag == channel) {
        input->active_seen = true;
        if (input->down || input->released) {
            float x = input->released ? input->release_x : input->x;
            float t = fmaxf(0, fminf(1, (x - bar.x) / fmaxf(1, bar.w - 1)));
            uint32_t before = color->rgba;
            if (color->mode == 0) {
                if (channel == 0) {
                    color->hsla.h = t * 359.999f;
                } else if (channel == 1) {
                    color->hsla.s = t;
                } else if (channel == 2) {
                    color->hsla.l = t;
                } else {
                    color->hsla.a = t;
                }
                color->rgba = pui_color_from_hsl(color->hsla);
            } else {
                unsigned shift = (3 - channel) * 8;
                assign(color,
                       (color->rgba & ~(255u << shift)) | ((uint32_t)lroundf(t * 255) << shift));
            }
            if (color->rgba != before) {
                color->notify_preview = true;
            }
            color->invalid = false;
            sync_fields(view, index, -1);
        }
        if (input->released) {
            color->notify_commit = color->rgba != color->committed;
            color->committed = color->rgba;
            color->drag = -1;
            color->notify_preview = true;
            input->active = 0;
            SDL_CaptureMouse(false);
        }
    }
    if (pui_hovered(input, hit) || color->drag == channel) {
        pui_view_request_cursor(view, SDL_SYSTEM_CURSOR_EW_RESIZE, color->drag == channel ? 2 : 1);
    }
    checker(view->renderer, bar);
    SDL_BlendMode blend;
    SDL_GetRenderDrawBlendMode(view->renderer, &blend);
    SDL_SetRenderDrawBlendMode(view->renderer, SDL_BLENDMODE_BLEND);
    int steps = (int)fmaxf(1, fminf(256, ceilf(bar.w)));
    for (int i = 0; i < steps; i++) {
        fill(view->renderer, (PUIRect){bar.x + i * bar.w / steps, bar.y, bar.w / steps, bar.h},
             sample(color, channel, steps > 1 ? (float)i / (steps - 1) : 0));
    }
    SDL_SetRenderDrawBlendMode(view->renderer, blend);
    float x = bar.x + component(color, channel) * fmaxf(0, bar.w - 1);
    SDL_SetRenderDrawColor(view->renderer, 255, 255, 255, 255);
    SDL_RenderLine(view->renderer, x, bar.y - 2, x, bar.y + bar.h + 1);
}

void pui_widget_color_picker_draw(PUIView *view, PUIContext *input, int index, PUIRect area,
                                  PUIAction action, void *context) {
    PUIColorPickerState *color = &view->state[index].color;
    const PUINode *node = &view->layout->nodes[index];
    int nodes[9];
    if (fields(view, index, nodes) != 9) {
        return;
    }
    // A field being left must commit before a mode switch can hide it.
    for (int j = 0; j < 9; j++) {
        if (view->submit == nodes[j]) {
            submit_field(view, index, j);
            view->submit = -1;
        }
    }
    float mode_width = fminf(65, fmaxf(24, (area.w - 58) / 2));
    for (int mode = 0; mode < 2; mode++) {
        PUIRect button = {area.x + mode * (mode_width + 4), area.y, mode_width, 26};
        PUIId id = node->id ^ (0x434f0000u + (unsigned)mode);
        if (pui_button(input, id, button, !input->blocked && color->drag < 0)) {
            color->mode = mode;
            pui_view_blur(view, input);
            sync_fields(view, index, -1);
        }
        pui_draw_button(view->renderer, pui_frect(button), ICON_NONE, mode ? "RGBA" : "HSL",
                        color->mode == mode, !input->blocked, pui_hovered(input, button));
    }
    PUIRect swatch = {area.x + area.w - 44, area.y, 44, 26};
    checker(view->renderer, swatch);
    SDL_BlendMode blend;
    SDL_GetRenderDrawBlendMode(view->renderer, &blend);
    SDL_SetRenderDrawBlendMode(view->renderer, SDL_BLENDMODE_BLEND);
    fill(view->renderer, swatch, color->rgba);
    SDL_SetRenderDrawBlendMode(view->renderer, blend);
    static const char *labels[2][4] = {{"Farbton (H)", "Saettigung %", "Luminanz %", "Deckkraft %"},
                                       {"Rot", "Gruen", "Blau", "Alpha"}};
    ColorBinding binding = {view, index};
    for (int j = 0; j < 4; j++) {
        float y = area.y + 36 + j * 48;
        pui_text(view->renderer, area.x, y + 5, labels[color->mode][j],
                 input->blocked ? PUI_DISABLED : PUI_TEXT);
        int field = nodes[color->mode * 4 + j];
        pui_view_draw_node(view, input, field,
                           (PUIRect){area.x + fmaxf(0, area.w - 70), y, fminf(70, area.w), 26},
                           field_action, &binding);
        slider(view, input, index, j, (PUIRect){area.x, y + 30, area.w, 10});
    }
    pui_text(view->renderer, area.x, area.y + 237, "Hex", PUI_TEXT);
    pui_view_draw_node(view, input, nodes[8],
                       (PUIRect){area.x + 36, area.y + 232, fmaxf(0, area.w - 36), 28},
                       field_action, &binding);
    if (color->invalid) {
        pui_text(view->renderer, area.x, area.y + 264, "Ungueltiger Farbwert", 0xc97565ff);
    }
    bool preview = color->notify_preview, commit = color->notify_commit;
    color->notify_preview = color->notify_commit = false;
    if (preview && action && *node->preview_action) {
        action(context, node->preview_action, node->name);
    }
    if (commit && action && *node->action) {
        action(context, node->action, node->name);
    }
}

void pui_widget_color_picker_finish_frame(PUIView *view, PUIContext *input, PUIAction action,
                                          void *context) {
    for (int i = 0; i < view->layout->count; i++) {
        const PUINode *node = &view->layout->nodes[i];
        if (node->type != PUI_NODE_COLOR_PICKER || view->state[i].color.drag < 0 ||
            (view->visible[i] && view->enabled[i])) {
            continue;
        }
        pui_widget_color_picker_cancel(view, i);
        pui_cancel(input);
        view->state[i].color.notify_preview = false;
        if (action && *node->preview_action) {
            action(context, node->preview_action, node->name);
        }
    }
}
