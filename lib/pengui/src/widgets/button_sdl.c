/** @file @brief Button drawing, image buttons and YAML action dispatch. */
#include "view/internal.h"
#include <math.h>
#include <string.h>

void ui_draw_button_themed(SDL_Renderer *r, const UiTheme *theme, SDL_FRect rect, UiIcon icon,
                           const char *label, bool active, bool enabled, bool hover) {
    ui_panel(r, rect,
             !enabled ? theme->panel
             : active ? theme->accent
             : hover  ? theme->hover
                      : theme->surface);
    Uint32 foreground = !enabled ? theme->disabled : active ? theme->canvas : theme->text;
    bool label_present = label && *label;
    if (icon != ICON_NONE) {
        ui_icon(r, icon, label_present ? rect.x + 9 : rect.x + (rect.w - 16) * .5f,
                rect.y + (rect.h - 16) * .5f, 16, foreground);
    }
    if (label_present) {
        ui_render_color(r, foreground);
        ui_text(r, rect.x + (icon == ICON_NONE ? 9 : 31), rect.y + (rect.h - UI_FONT_HEIGHT) * .5f,
                label, foreground);
    }
}

void ui_draw_button(SDL_Renderer *r, SDL_FRect rect, UiIcon icon, const char *label, bool active,
                    bool enabled, bool hover) {
    UiTheme theme = ui_theme;
    ui_draw_button_themed(r, &theme, rect, icon, label, active, enabled, hover);
}

void ui_draw_image_button(SDL_Renderer *renderer, SDL_FRect rect, SDL_Texture *image,
                          const char *label, float padding, bool active, bool enabled, bool hover,
                          bool pressed, bool focused) {
    bool selected = active || (enabled && pressed);
    ui_draw_button(renderer, rect, ICON_NONE, NULL, selected, enabled, hover);
    bool text = label && *label;
    float width, height;
    if (image && SDL_GetTextureSize(image, &width, &height)) {
        UiRect slot = {text ? rect.x + 5 : rect.x, rect.y, text ? 24 : rect.w, rect.h};
        UiRect fit = ui_image_fit(slot, width, height, padding);
        SDL_FRect target = {fit.x, fit.y, fit.w, fit.h};
        Uint8 alpha;
        SDL_GetTextureAlphaMod(image, &alpha);
        if (!enabled) {
            SDL_SetTextureAlphaMod(image, alpha / 2);
        }
        if (fit.w > 0 && fit.h > 0) {
            SDL_RenderTexture(renderer, image, NULL, &target);
        }
        SDL_SetTextureAlphaMod(image, alpha);
    }
    if (text) {
        Uint32 foreground = !enabled   ? ui_theme.disabled
                            : selected ? ui_theme.canvas
                                       : ui_theme.text;
        ui_text(renderer, rect.x + 31, rect.y + (rect.h - UI_FONT_HEIGHT) * .5f, label, foreground);
    }
    if (enabled && focused) {
        ui_render_color(renderer, ui_theme.accent);
        SDL_RenderRect(renderer, &rect);
    }
}

void ui_widget_button_draw(UiView *v, UiContext *ui, int i, UiRect area, UiAction action,
                           void *context) {
    const UiNode *n = &v->layout->nodes[i];
    Widget *s = &v->state[i];
    bool hover = ui_hovered(ui, area);
    UiIcon button_icon = *n->image ? ICON_IMAGE : ui_icon_from_name(n->icon);
    if (v->window && !strcmp(n->action, "window.maximize") &&
        (SDL_GetWindowFlags(v->window) & SDL_WINDOW_MAXIMIZED)) {
        button_icon = ICON_RESTORE;
    }
    if (s->image) {
        ui_draw_image_button(v->renderer, ui_frect(area), s->image, n->text, n->image_padding,
                             false, !ui->blocked, hover, ui->active == n->id && ui->down,
                             ui->focus == n->id);
    } else {
        ui_draw_button(v->renderer, ui_frect(area), button_icon, n->text, false, !ui->blocked,
                       hover);
    }
    if (ui_button(ui, n->id, area, !ui->blocked) && action && *n->action) {
        action(context, n->action, n->name);
    }
}
