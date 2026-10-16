#include "app/editor_internal.h"

void editor_field_start(Field f, const char *value) {
    app.field = f;
    app.field_inline = app.field_invalid = app.field_selecting = false;
    app.field_id = ui_id("modal_text");
    ui_focus(&app.ui, app.field_id);
    snprintf(app.input, sizeof app.input, "%s", value);
    ui_text_edit_init(&app.field_edit, app.input, true);
    app.field_blink = SDL_GetTicks();
    SDL_StartTextInput(app.window);
}

void editor_field_stop(void) {
    app.field = FIELD_NONE;
    app.field_inline = app.field_invalid = app.field_selecting = false;
    app.needs_layout = true;
    ui_cancel(&app.ui);
    ui_focus(&app.ui, editor_scripts_active() && (*app.script).focused ? UI_SCRIPT_DOCUMENT : 0);
    if (editor_scripts_active() && (*app.script).focused) {
        SDL_StartTextInput(app.window);
    } else {
        SDL_StopTextInput(app.window);
    }
}

void editor_field_start_inline(Field field, const char *value, UiId id, UiRect rect) {
    editor_field_start(field, value);
    app.field_inline = app.field_seen = true;
    app.field_entity = app.selected;
    app.field_id = id;
    app.field_rect = rect;
    ui_focus(&app.ui, id);
    ui_cancel(&app.ui);
}

void editor_draw_input(Field field, const char *value, const char *input, UiRect cell,
                       bool enabled) {
    UiId id = ui_id("component_input") ^ app.selected * 31u ^ (UiId)field * 16777619u;
    if (!id) {
        id = 1;
    }
    editor_register_ui_hit(cell);
    bool focused = app.field_inline && app.field == field && app.field_entity == app.selected;
    if (!focused && ui_button(&app.ui, id, cell, enabled)) {
        editor_field_start_inline(field, input, id, cell);
        focused = true;
    }
    UiTextEdit idle = {0};
    if (focused) {
        app.field_seen = true;
        app.field_rect = cell;
    }
    ui_draw_text_input(app.renderer, (SDL_FRect){cell.x, cell.y, cell.w, cell.h},
                       focused ? app.input : value, focused ? &app.field_edit : &idle, focused,
                       enabled, focused && app.field_invalid,
                       (SDL_GetTicks() - app.field_blink) % 1000 < 500);
}

static void field_click(float x, bool extend) {
    float column = (x - app.field_rect.x - 7 + app.field_edit.scroll) / UI_FONT_WIDTH;
    ui_text_edit_click(&app.field_edit, app.input, (size_t)fmaxf(0, floorf(column + .5f)), extend);
    app.field_blink = SDL_GetTicks();
}

bool editor_field_event(SDL_Event *e) {
    if (app.field == FIELD_NONE) {
        return false;
    }
    if (e->type == SDL_EVENT_WINDOW_FOCUS_LOST || e->type == SDL_EVENT_QUIT ||
        e->type == SDL_EVENT_WINDOW_CLOSE_REQUESTED) {
        editor_field_stop();
        return false;
    }
    if (e->type == SDL_EVENT_TEXT_INPUT) {
        if (ui_text_edit_insert(&app.field_edit, app.input, sizeof app.input, e->text.text)) {
            app.field_invalid = false;
        }
        app.field_blink = SDL_GetTicks();
        return true;
    }
    if (e->type == SDL_EVENT_KEY_DOWN) {
        SDL_Keycode key = e->key.key;
        bool ctrl = (e->key.mod & (SDL_KMOD_CTRL | SDL_KMOD_GUI)) != 0;
        bool shift = (e->key.mod & SDL_KMOD_SHIFT) != 0;
        app.field_blink = SDL_GetTicks();
        if (key == SDLK_ESCAPE) {
            editor_field_stop();
        } else if (key == SDLK_RETURN || key == SDLK_KP_ENTER) {
            editor_field_apply();
        } else if (ctrl && key == SDLK_A) {
            ui_text_edit_key(&app.field_edit, app.input, UI_TEXT_SELECT_ALL, false);
        } else if (ctrl && key == SDLK_V) {
            char *text = SDL_GetClipboardText();
            if (text) {
                if (ui_text_edit_insert(&app.field_edit, app.input, sizeof app.input, text)) {
                    app.field_invalid = false;
                }
                SDL_free(text);
            }
        } else if (ctrl && (key == SDLK_C || key == SDLK_X)) {
            size_t a = app.field_edit.cursor < app.field_edit.anchor ? app.field_edit.cursor
                                                                     : app.field_edit.anchor;
            size_t b = app.field_edit.cursor > app.field_edit.anchor ? app.field_edit.cursor
                                                                     : app.field_edit.anchor;
            if (a < b && b <= strlen(app.input)) {
                char text[sizeof app.input];
                memcpy(text, app.input + a, b - a);
                text[b - a] = 0;
                if (SDL_SetClipboardText(text) && key == SDLK_X) {
                    ui_text_edit_key(&app.field_edit, app.input, UI_TEXT_DELETE, false);
                    app.field_invalid = false;
                }
            }
        } else {
            UiTextKey action;
            switch (key) {
            case SDLK_LEFT:
                action = ctrl ? UI_TEXT_HOME : UI_TEXT_LEFT;
                break;
            case SDLK_RIGHT:
                action = ctrl ? UI_TEXT_END : UI_TEXT_RIGHT;
                break;
            case SDLK_HOME:
                action = UI_TEXT_HOME;
                break;
            case SDLK_END:
                action = UI_TEXT_END;
                break;
            case SDLK_BACKSPACE:
                action = UI_TEXT_BACKSPACE;
                break;
            case SDLK_DELETE:
                action = UI_TEXT_DELETE;
                break;
            default:
                return true;
            }
            if (ui_text_edit_key(&app.field_edit, app.input, action, shift)) {
                app.field_invalid = false;
            }
        }
        return true;
    }
    if (e->type == SDL_EVENT_KEY_UP) {
        return true;
    }
    if (e->type == SDL_EVENT_MOUSE_MOTION && app.field_selecting) {
        field_click(e->motion.x, true);
        return true;
    }
    if (e->type == SDL_EVENT_MOUSE_BUTTON_UP && app.field_selecting) {
        app.field_selecting = false;
        return true;
    }
    if (e->type == SDL_EVENT_MOUSE_BUTTON_DOWN) {
        if (ui_contains(app.field_rect, e->button.x, e->button.y)) {
            if (e->button.button == SDL_BUTTON_LEFT) {
                field_click(e->button.x, (SDL_GetModState() & SDL_KMOD_SHIFT) != 0);
                app.field_selecting = true;
            }
            return true;
        }
        if (app.field_inline) {
            editor_field_apply();
            return app.field != FIELD_NONE;
        }
    }
    if (e->type == SDL_EVENT_MOUSE_WHEEL && app.field_inline) {
        editor_field_apply();
        return app.field != FIELD_NONE;
    }
    return false;
}

void editor_draw_modal(void) {
    SDL_SetRenderDrawBlendMode(app.renderer, SDL_BLENDMODE_BLEND);
    editor_box((SDL_FRect){0, 0, (float)app.width, (float)app.height}, 0x000000a0);
    SDL_SetRenderDrawBlendMode(app.renderer, SDL_BLENDMODE_NONE);
    float x = app.width / 2.f - 310, y = app.height / 2.f - 95;
    ui_panel(app.renderer, (SDL_FRect){x, y, 620, 190}, UI_SURFACE);
    if (app.request != REQUEST_NONE) {
        editor_text(x + 20, y + 24, "Unsaved changes", UI_ACCENT);
        editor_text(x + 20, y + 54, "Save before opening another scene or closing?", UI_TEXT);
        if (button(x + 20, y + 110, 170, "Save and continue", false, true)) {
            app.deferred_save = true;
        }
        if (button(x + 210, y + 110, 170, "Discard changes", false, true)) {
            editor_request_finish();
        }
        if (button(x + 400, y + 110, 170, "Cancel", false, true)) {
            app.request = REQUEST_NONE;
        }
    } else {
        const char *title = app.field == FIELD_IMPORT  ? "Import PNG: enter full file path"
                            : app.field == FIELD_LAYER ? "Layer name"
                            : app.field == FIELD_COLOR ? "Sprite color: RRGGBBAA"
                                                       : "Enter value";
        editor_text(x + 20, y + 24, title, UI_ACCENT);
        app.field_rect = (UiRect){x + 20, y + 54, 580, 28};
        ui_draw_text_input(app.renderer, (SDL_FRect){x + 20, y + 54, 580, 28}, app.input,
                           &app.field_edit, true, true, app.field_invalid,
                           (SDL_GetTicks() - app.field_blink) % 1000 < 500);
        editor_text(x + 20, y + 94, "Enter: apply   Escape: cancel   Ctrl+A: select all", UI_MUTED);
        if (button(x + 20, y + 125, 110, "Apply", false, true)) {
            editor_field_apply();
        }
        if (button(x + 145, y + 125, 110, "Cancel", false, true)) {
            editor_field_stop();
        }
    }
    editor_text(x + 20, y + 164, app.status, UI_ACCENT);
}
