#include "app/editor_internal.h"

void editor_field_start(Field f, const char *value) {
    app.field = f;
    ui_focus(&app.ui, ui_id("modal_text"));
    snprintf(app.input, sizeof app.input, "%s", value);
    SDL_StartTextInput(app.window);
}

void editor_field_stop(void) {
    app.field = FIELD_NONE;
    ui_focus(&app.ui, editor_scripts_active() && (*app.script).focused ? UI_SCRIPT_DOCUMENT : 0);
    if (editor_scripts_active() && (*app.script).focused)
        SDL_StartTextInput(app.window);
    else
        SDL_StopTextInput(app.window);
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
        if (button(x + 20, y + 110, 170, "Save and continue", false, true))
            app.deferred_save = true;
        if (button(x + 210, y + 110, 170, "Discard changes", false, true))
            editor_request_finish();
        if (button(x + 400, y + 110, 170, "Cancel", false, true))
            app.request = REQUEST_NONE;
    } else {
        const char *title = app.field == FIELD_IMPORT  ? "Import PNG: enter full file path"
                            : app.field == FIELD_LAYER ? "Layer name"
                                                       : "Enter value";
        editor_text(x + 20, y + 24, title, UI_ACCENT);
        editor_box((SDL_FRect){x + 20, y + 54, 580, 28}, UI_CANVAS);
        size_t n = strlen(app.input);
        editor_text(x + 26, y + 64, app.input + (n > 69 ? n - 69 : 0), UI_TEXT);
        editor_text(x + 20, y + 94, "Enter: apply   Escape: cancel   Ctrl+A: clear", UI_MUTED);
        if (button(x + 20, y + 125, 110, "Apply", false, true))
            editor_field_apply();
        if (button(x + 145, y + 125, 110, "Cancel", false, true))
            editor_field_stop();
    }
    editor_text(x + 20, y + 164, app.status, UI_ACCENT);
}
