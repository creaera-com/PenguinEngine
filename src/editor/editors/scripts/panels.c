#include "app/editor_internal.h"

void editor_draw_script_inspector(void) {
    float x = (float)(app.panel_area.x + 9), y = app.panel_top + 18 - app.inspector_view.offset;
    UiRect clip = {(float)(app.panel_area.x + 5), app.panel_top + 8, (float)(app.panel_area.w - 18),
                   (float)(app.panel_area.h - 16)};
    ui_clip_push(&app.ui, clip);
    ui_apply_clip(app.renderer, &app.ui);
    editor_section(x, y, ICON_FILE, "JAVASCRIPT");
    editor_text(x, y + 28, "Parser: QuickJS-ng", UI_TEXT);
    editor_text(x, y + 48, "Compile only / no execution", UI_MUTED);
    editor_section(x, y + 90, ICON_EDIT, "EDITOR SHORTCUTS");
    const char *help[] = {
        "Ctrl+S     Save script",  "Ctrl+Z     Undo",       "Ctrl+Shift+Z  Redo",
        "Ctrl+Space  Suggestions", "Enter / Tab  Accept",   "F7         Check syntax",
        "Shift+arrows  Select",    "Ctrl+C/X/V  Clipboard", "Shift+wheel  Horizontal"};
    for (int i = 0; i < 9; i++)
        editor_text(x, y + 116 + i * 24, help[i], UI_MUTED);
    editor_text(x, y + 354, "Completion: keywords,", UI_TEXT);
    editor_text(x, y + 374, "standard JS objects and", UI_TEXT);
    editor_text(x, y + 394, "document identifiers.", UI_TEXT);
    editor_text(x, y + 426, "Type analysis / LSP later.", UI_MUTED);
    app.inspector_view.content = 460;
    app.inspector_view.viewport = (float)(app.panel_area.h - 16);
    ui_scroll_clamp(&app.inspector_view);
    ui_clip_pop(&app.ui);
    ui_apply_clip(app.renderer, &app.ui);
    UiRect track = {(float)(app.panel_area.x + app.panel_area.w - 15), app.panel_top + 8, 8,
                    app.inspector_view.viewport};
    ui_scrollbar(&app.ui, UI_INSPECTOR_SCROLL, track, &app.inspector_view, UI_AXIS_Y);
    ui_draw_scrollbar(app.renderer, &app.ui, UI_INSPECTOR_SCROLL, track, &app.inspector_view,
                      UI_AXIS_Y);
}

void editor_draw_script_tools(SDL_FRect area) {
    editor_draw_yaml_toolbar(area);
}

void editor_draw_script_content(void) {
    se_draw(&(*app.script), app.renderer, &app.ui, app.viewport, app.mx, app.my, app.click);
}
