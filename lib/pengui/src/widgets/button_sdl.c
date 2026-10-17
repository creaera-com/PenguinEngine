/** @file @brief Button drawing, image buttons and YAML action dispatch. */
#include "view/internal.h"
#include <math.h>
#include <string.h>

void pui_draw_button_themed(SDL_Renderer *r, const PUITheme *theme, SDL_FRect rect, PUIIcon icon,
                            const char *label, bool active, bool enabled, bool hover) {
    pui_panel(r, rect,
              !enabled ? theme->panel
              : active ? theme->accent
              : hover  ? theme->hover
                       : theme->surface);
    Uint32 foreground = !enabled ? theme->disabled : active ? theme->canvas : theme->text;
    bool label_present = label && *label;
    if (icon != ICON_NONE) {
        pui_icon(r, icon, label_present ? rect.x + 9 : rect.x + (rect.w - 16) * .5f,
                 rect.y + (rect.h - 16) * .5f, 16, foreground);
    }
    if (label_present) {
        pui_render_color(r, foreground);
        pui_text(r, rect.x + (icon == ICON_NONE ? 9 : 31),
                 rect.y + (rect.h - PUI_FONT_HEIGHT) * .5f, label, foreground);
    }
}

void pui_draw_button(SDL_Renderer *r, SDL_FRect rect, PUIIcon icon, const char *label, bool active,
                     bool enabled, bool hover) {
    PUITheme theme = pui_theme;
    pui_draw_button_themed(r, &theme, rect, icon, label, active, enabled, hover);
}

void pui_draw_image_button(SDL_Renderer *renderer, SDL_FRect rect, SDL_Texture *image,
                           const char *label, float padding, bool active, bool enabled, bool hover,
                           bool pressed, bool focused) {
    bool selected = active || (enabled && pressed);
    pui_draw_button(renderer, rect, ICON_NONE, NULL, selected, enabled, hover);
    bool text = label && *label;
    float width, height;
    if (image && SDL_GetTextureSize(image, &width, &height)) {
        PUIRect slot = {text ? rect.x + 5 : rect.x, rect.y, text ? 24 : rect.w, rect.h};
        PUIRect fit = pui_image_fit(slot, width, height, padding);
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
        Uint32 foreground = !enabled   ? pui_theme.disabled
                            : selected ? pui_theme.canvas
                                       : pui_theme.text;
        pui_text(renderer, rect.x + 31, rect.y + (rect.h - PUI_FONT_HEIGHT) * .5f, label,
                 foreground);
    }
    if (enabled && focused) {
        pui_render_color(renderer, pui_theme.accent);
        SDL_RenderRect(renderer, &rect);
    }
}

void pui_widget_button_draw(PUIView *v, PUIContext *pui, int i, PUIRect area, PUIAction action,
                            void *context) {
    const PUINode *n = &v->layout->nodes[i];
    Widget *s = &v->state[i];
    bool hover = pui_hovered(pui, area);
    PUIIcon button_icon = *n->image ? ICON_IMAGE : pui_icon_from_name(n->icon);
    if (v->window && !strcmp(n->action, "window.maximize") &&
        (SDL_GetWindowFlags(v->window) & SDL_WINDOW_MAXIMIZED)) {
        button_icon = ICON_RESTORE;
    }
    if (s->image) {
        pui_draw_image_button(v->renderer, pui_frect(area), s->image, n->text, n->image_padding,
                              false, !pui->blocked, hover, pui->active == n->id && pui->down,
                              pui->focus == n->id);
    } else {
        pui_draw_button(v->renderer, pui_frect(area), button_icon, n->text, false, !pui->blocked,
                        hover);
    }
    if (pui_button(pui, n->id, area, !pui->blocked) && action && *n->action) {
        action(context, n->action, n->name);
    }
}
