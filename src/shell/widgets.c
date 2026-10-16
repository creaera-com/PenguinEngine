#include "application/internal.h"
const char *editor_tooltip;

Uint32 editor_widget_background(void) {
    Uint32 background = UI_SURFACE & 0xffu;
    for (int shift = 8; shift <= 24; shift += 8) {
        Uint32 channel = (UI_SURFACE >> shift) & 0xffu;
        background |= ((channel * 95 + 255 * 5) / 100) << shift;
    }
    return background;
}

Uint32 editor_tree_row_background(int row) {
    Uint32 base = editor_widget_background();
    if (!(row & 1)) {
        return base;
    }
    Uint32 background = base & 0xffu;
    for (int shift = 8; shift <= 24; shift += 8) {
        Uint32 channel = (base >> shift) & 0xffu;
        background |= ((channel * 97 + 255 * 3) / 100) << shift;
    }
    return background;
}

bool editor_inside(float x, float y, SDL_FRect r) {
    return x >= r.x && y >= r.y && x < r.x + r.w && y < r.y + r.h;
}

void editor_draw_color(uint32_t c) {
    SDL_SetRenderDrawColor(app.platform.renderer, (Uint8)(c >> 24), (Uint8)(c >> 16),
                           (Uint8)(c >> 8), (Uint8)c);
}

void editor_box(SDL_FRect r, uint32_t c) {
    editor_draw_color(c);
    SDL_RenderFillRect(app.platform.renderer, &r);
}

void editor_draw_split_grip(UiRect area, bool highlighted) {
    float x = floorf(area.x + area.w * .5f), y = floorf(area.y + area.h * .5f);
    editor_draw_color(highlighted ? UI_ACCENT : UI_MUTED);
    for (int i = -1; i <= 1; i++) {
        float cy = y + i * 6;
        SDL_RenderPoint(app.platform.renderer, x, cy - 1);
        SDL_RenderLine(app.platform.renderer, x - 1, cy, x + 1, cy);
        SDL_RenderPoint(app.platform.renderer, x, cy + 1);
    }
}

void editor_text(float x, float y, const char *label, uint32_t color) {
    editor_draw_color(color);
    ui_text(app.platform.renderer, x, y - 4, label, color);
}

void editor_register_ui_hit(UiRect rect) {
    if (app.shell.ui.clip_depth) {
        UiRect clip = app.shell.ui.clips[app.shell.ui.clip_depth - 1];
        float right = fminf(rect.x + rect.w, clip.x + clip.w),
              bottom = fminf(rect.y + rect.h, clip.y + clip.h);
        rect.x = fmaxf(rect.x, clip.x);
        rect.y = fmaxf(rect.y, clip.y);
        rect.w = right - rect.x;
        rect.h = bottom - rect.y;
    }
    if (app.shell.hit_count < PE_UI_MAX_NODES && rect.w > 0 && rect.h > 0) {
        app.shell.hits[app.shell.hit_count++] = rect;
    }
}

bool editor_icon_button_impl(UiId callsite, float x, float y, float w, UiIcon icon,
                             const char *label, const char *hint, bool active, bool enabled) {
    SDL_FRect r = {x, y, w, 25};
    UiRect hit = {x, y, w, 25};
    editor_register_ui_hit(hit);
    bool hover = ui_hovered(&app.shell.ui, hit);
    ui_draw_button(app.platform.renderer, r, icon, label, active, enabled, hover);
    if (hover && hint) {
        editor_tooltip = hint;
    }

    UiId id = ui_id("editor_button") ^ (callsite * 16777619u) ^ app.shell.widget_scope;
    return ui_button(&app.shell.ui, id ? id : 1, hit, enabled);
}

bool editor_button_impl(UiId callsite, float x, float y, float w, const char *label, bool active,
                        bool enabled) {
    return editor_icon_button_impl(callsite, x, y, w, ICON_NONE, label, NULL, active, enabled);
}

void editor_section(float x, float y, UiIcon icon, const char *title) {
    ui_icon(app.platform.renderer, icon, x, y - 4, 16, UI_ACCENT);
    editor_text(x + 24, y, title, UI_MUTED);
}

void editor_disclosure(float x, float y, float w, const char *title, bool *open) {
    UiRect rect = {x, y - 6, w, 26};
    ui_disclosure(&app.shell.ui, ui_id(title), rect, open, true);
    ui_draw_button(app.platform.renderer, (SDL_FRect){rect.x, rect.y, rect.w, rect.h},
                   *open ? ICON_DOWN : ICON_RIGHT, title, false, true,
                   ui_hovered(&app.shell.ui, rect));
}
