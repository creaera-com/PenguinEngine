/** @file @brief Button drawing, image buttons and YAML action dispatch. */
#include "view/internal.h"
#include <math.h>
#include <string.h>

static int rounded_points(SDL_FRect r, float radius, SDL_FPoint points[37]) {
    radius = fmaxf(0, fminf(radius, fminf(r.w, r.h) / 2));
    int count = 0;
    for (int corner = 0; corner < 4; corner++) {
        float cx = r.x + (corner == 0 || corner == 3 ? r.w - radius : radius);
        float cy = r.y + (corner < 2 ? r.h - radius : radius);
        for (int step = 0; step <= 8; step++) {
            float angle = (corner * 90 + step * 90.f / 8) * .017453293f;
            points[count++] = (SDL_FPoint){cx + cosf(angle) * radius, cy + sinf(angle) * radius};
        }
    }
    points[count] = points[0];
    return count;
}

static void background(SDL_Renderer *r, SDL_FRect rect, float radius, uint32_t color,
                       uint32_t focus) {
    pui_rounded_panel(r, rect, color, radius, false);

    if (focus & 255) {
        pui_render_color(r, focus);
        SDL_FRect outline = {rect.x + .5f, rect.y + .5f, fmaxf(0, rect.w - 1),
                             fmaxf(0, rect.h - 1)};
        if (radius <= 0) {
            SDL_RenderRect(r, &outline);
        } else {
            SDL_FPoint points[37];
            int n = rounded_points(outline, fmaxf(0, radius - .5f), points);
            SDL_RenderLines(r, points, n + 1);
        }
    }
}

static void content(SDL_Renderer *r, SDL_FRect rect, PUIIcon icon, const char *label,
                    uint32_t foreground) {
    bool text = label && *label;
    if (icon != ICON_NONE) {
        pui_icon(r, icon, text ? rect.x + 9 : rect.x + (rect.w - 16) * .5f,
                 rect.y + (rect.h - 16) * .5f, 16, foreground);
    }
    if (text) {
        pui_text(r, rect.x + (icon == ICON_NONE ? 9 : 31),
                 rect.y + (rect.h - PUI_FONT_HEIGHT) * .5f, label, foreground);
    }
}

void pui_draw_button_themed(SDL_Renderer *r, const PUITheme *theme, SDL_FRect rect, PUIIcon icon,
                            const char *label, bool active, bool enabled, bool hover) {
    Uint32 bg = !enabled ? theme->panel
                : active ? theme->accent
                : hover  ? theme->hover
                         : theme->surface;
    Uint32 fg = !enabled ? theme->disabled : active ? theme->canvas : theme->text;
    background(r, rect, theme->button_radius, bg, 0);
    content(r, rect, icon, label, fg);
}

void pui_draw_button(SDL_Renderer *r, SDL_FRect rect, PUIIcon icon, const char *label, bool active,
                     bool enabled, bool hover) {
    PUITheme theme = pui_theme;
    pui_draw_button_themed(r, &theme, rect, icon, label, active, enabled, hover);
}

static void image_content(SDL_Renderer *renderer, SDL_FRect rect, SDL_Texture *image,
                          const char *label, float padding, bool enabled) {
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
}

void pui_draw_image_button(SDL_Renderer *renderer, SDL_FRect rect, SDL_Texture *image,
                           const char *label, float padding, bool active, bool enabled, bool hover,
                           bool pressed, bool focused) {
    bool selected = active || (enabled && pressed);
    pui_draw_button(renderer, rect, ICON_NONE, NULL, selected, enabled, hover);
    bool text = label && *label;
    image_content(renderer, rect, image, label, padding, enabled);
    if (text) {
        Uint32 foreground = !enabled   ? pui_theme.disabled
                            : selected ? pui_theme.canvas
                                       : pui_theme.text;
        pui_text(renderer, rect.x + 31, rect.y + (rect.h - PUI_FONT_HEIGHT) * .5f, label,
                 foreground);
    }
    if (enabled && focused) {
        SDL_FPoint points[37];
        int count = rounded_points(rect, pui_theme.button_radius, points);
        pui_render_color(renderer, pui_theme.accent);
        SDL_RenderLines(renderer, points, count + 1);
    }
}

/* Keep the button rim visible; the checkerboard makes alpha readable on dark themes. */
static void color_swatch(SDL_Renderer *renderer, PUIRect area, uint32_t rgba) {
    SDL_FRect rect = {area.x + 3, area.y + 3, fmaxf(0, area.w - 6), fmaxf(0, area.h - 6)};
    if ((rgba & 255) < 255) {
        pui_render_color(renderer, 0xb0b0b0ff);
        SDL_RenderFillRect(renderer, &rect);
        pui_render_color(renderer, 0x969696ff);
        SDL_FRect cells[256];
        int count = 0;
        for (int y = 0; y * 6 < rect.h; y++) {
            for (int x = 0; x * 6 < rect.w; x++) {
                if ((x + y) % 2) {
                    cells[count++] =
                        (SDL_FRect){rect.x + x * 6, rect.y + y * 6, fminf(6, rect.w - x * 6),
                                    fminf(6, rect.h - y * 6)};
                    if (count == 256) {
                        SDL_RenderFillRects(renderer, cells, count);
                        count = 0;
                    }
                }
            }
        }
        SDL_RenderFillRects(renderer, cells, count);
    }
    pui_render_color(renderer, rgba);
    SDL_RenderFillRect(renderer, &rect);
}

void pui_widget_button_paint(PUIView *v, PUIContext *pui, int i, PUIRect area, bool active,
                             PUIIcon icon) {
    const PUINode *n = &v->layout->nodes[i];
    Widget *s = &v->state[i];
    bool enabled = !pui->blocked, focus = enabled && pui->focus == n->id;
    bool selected = active || (enabled && pui->active == n->id && pui->down);
    bool hover = enabled && pui_hovered(pui, area);
    float radius = n->radius >= 0 ? n->radius : pui_theme.button_radius;
    float duration = n->transition_ms >= 0 ? n->transition_ms : pui_theme.button_transition_ms;
    uint32_t bg = !enabled           ? pui_theme.panel
                  : selected         ? pui_theme.accent
                  : (hover || focus) ? pui_theme.hover
                                     : pui_theme.surface;
    uint32_t fg = !enabled ? pui_theme.disabled : selected ? pui_theme.canvas : pui_theme.text;
    uint32_t border = focus ? pui_theme.accent : pui_theme.accent & 0xffffff00u;
    bg = pui_color_transition(&s->button_background, bg, v->frame_ms, duration);
    fg = pui_color_transition(&s->button_foreground, fg, v->frame_ms, duration);
    border = pui_color_transition(&s->button_focus, border, v->frame_ms, duration);
    SDL_BlendMode blend;
    SDL_GetRenderDrawBlendMode(v->renderer, &blend);
    SDL_SetRenderDrawBlendMode(v->renderer, SDL_BLENDMODE_BLEND);
    background(v->renderer, pui_frect(area), radius, bg, border);
    uint32_t swatch;
    if (*n->color_input && pui_view_color(v, n->color_input, &swatch)) {
        color_swatch(v->renderer, area, swatch);
    } else if (s->image) {
        image_content(v->renderer, pui_frect(area), s->image, n->text,
                      fmaxf(n->image_padding, fminf(radius, fminf(area.w, area.h) / 2) * .3f),
                      enabled);
        if (*n->text) {
            pui_text(v->renderer, area.x + 31, area.y + (area.h - PUI_FONT_HEIGHT) * .5f, n->text,
                     fg);
        }
    } else {
        content(v->renderer, pui_frect(area), icon, n->text, fg);
    }
    SDL_SetRenderDrawBlendMode(v->renderer, blend);
}

void pui_widget_button_draw(PUIView *v, PUIContext *pui, int i, PUIRect area, PUIAction action,
                            void *context) {
    const PUINode *n = &v->layout->nodes[i];
    Widget *s = &v->state[i];
    bool activated = !s->group && pui_button(pui, n->id, area, !pui->blocked);
    PUIIcon button_icon = *n->image ? ICON_IMAGE : pui_icon_from_name(n->icon);
    if (v->window && !strcmp(n->action, "window.maximize") &&
        (SDL_GetWindowFlags(v->window) & SDL_WINDOW_MAXIMIZED)) {
        button_icon =
            !strncmp(n->icon, "tabler:", 7) ? pui_icon_from_name("tabler:copy") : ICON_RESTORE;
    }
    pui_widget_button_paint(v, pui, i, area, s->checked, button_icon);
    if (activated && action && *n->action) {
        action(context, n->action, n->name);
    }
}
