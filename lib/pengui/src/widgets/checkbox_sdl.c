/** @file @brief Checkbox rendering; the complete row is clickable. */
#include "view/internal.h"
#include <math.h>

void pui_draw_checkbox(SDL_Renderer *renderer, SDL_FRect rect, const char *label, bool checked,
                       bool enabled, bool hover, bool focused) {
    PUIRect area = {rect.x, rect.y, rect.w, rect.h};
    float size = fminf(18, fminf(area.w, area.h));
    SDL_FRect box = {area.x + 4, area.y + (area.h - size) / 2, size, size};
    pui_panel(renderer, box, enabled && checked ? PUI_ACCENT : PUI_CANVAS);
    pui_render_color(renderer, !enabled ? PUI_DISABLED : hover ? PUI_ACCENT : PUI_BORDER);
    SDL_RenderRect(renderer, &box);
    if (checked) {
        pui_icon(renderer, ICON_CHECK, box.x + 1, box.y + 1, fmaxf(0, size - 2),
                 enabled ? PUI_CANVAS : PUI_DISABLED);
    }
    pui_text(renderer, area.x + 30, area.y + (area.h - PUI_FONT_HEIGHT) / 2, label,
             enabled ? PUI_TEXT : PUI_DISABLED);
    if (enabled && focused) {
        pui_render_color(renderer, PUI_ACCENT);
        SDL_FRect outline = pui_frect(area);
        SDL_RenderRect(renderer, &outline);
    }
}

void pui_widget_checkbox_draw(PUIView *v, PUIContext *pui, int i, PUIRect area, PUIAction action,
                              void *context) {
    const PUINode *n = &v->layout->nodes[i];
    Widget *s = &v->state[i];
    bool enabled = !pui->blocked;
    bool changed = pui_toggle(pui, n->id, area, &s->checked, enabled);
    pui_draw_checkbox(v->renderer, pui_frect(area), n->text, s->checked, enabled,
                      pui_hovered(pui, area), pui->focus == n->id);
    if (changed && action && *n->action) {
        action(context, n->action, n->name);
    }
}
