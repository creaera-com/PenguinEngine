/** @file @brief Modal, YAML-configured radial context menu with deferred action dispatch. */
#include "view/internal.h"
#include <math.h>
#include <string.h>

bool pui_view_open_radial(PUIView *view, const char *id, float x, float y) {
    if (!isfinite(x) || !isfinite(y)) {
        return false;
    }
    for (int i = 0; i < view->layout->count; i++) {
        const PUINode *node = &view->layout->nodes[i];
        if (node->type == PUI_NODE_RADIAL_MENU && node->enabled && !strcmp(node->name, id)) {
            float size = fminf(node->popup_width > 0 ? node->popup_width : 300,
                               fminf(view->bounds.w, view->bounds.h));
            if (size < 100) {
                return false;
            }
            view->radial_bounds = (PUIRect){
                fmaxf(view->bounds.x, fminf(x - size / 2, view->bounds.x + view->bounds.w - size)),
                fmaxf(view->bounds.y, fminf(y - size / 2, view->bounds.y + view->bounds.h - size)),
                size, size};
            view->radial_open = true;
            view->radial_index = i;
            view->radial_pressed = view->radial_pending = -1;
            view->radial_x = x;
            view->radial_y = y;
            return true;
        }
    }
    return false;
}

bool pui_view_set_radial_enabled(PUIView *view, const char *id, bool enabled) {
    for (int i = 0; i < view->layout->count; i++) {
        if (view->layout->nodes[i].type != PUI_NODE_RADIAL_MENU) {
            continue;
        }
        for (int k = view->layout->nodes[i].child; k >= 0; k = view->layout->nodes[k].next) {
            if (!strcmp(view->layout->nodes[k].name, id)) {
                view->state[k].radial_disabled = !enabled;
                return true;
            }
        }
    }
    return false;
}

void pui_view_close_radial(PUIView *view) {
    view->radial_open = false;
    view->radial_pending = -1;
}

static int item_at(PUIView *view, float x, float y) {
    int count = 0;
    int items[8];
    for (int k = view->layout->nodes[view->radial_index].child; k >= 0;
         k = view->layout->nodes[k].next) {
        if (count == 8) {
            return -1;
        }
        items[count++] = k;
    }
    int index = pui_radial_item(view->radial_bounds, count, x, y);
    return index < 0 || (!view->layout->nodes[items[index]].enabled ||
                         view->state[items[index]].radial_disabled)
               ? -1
               : items[index];
}

bool pui_widget_radial_event(PUIView *view, PUIContext *input, const SDL_Event *event) {
    if (!view->radial_open) {
        return false;
    }
    if (event->type == SDL_EVENT_WINDOW_FOCUS_LOST ||
        (event->type == SDL_EVENT_KEY_DOWN && event->key.key == SDLK_ESCAPE)) {
        pui_view_close_radial(view);
        pui_cancel(input);
        return event->type != SDL_EVENT_WINDOW_FOCUS_LOST;
    }
    if (event->type == SDL_EVENT_MOUSE_MOTION) {
        view->radial_x = event->motion.x;
        view->radial_y = event->motion.y;
        return true;
    }
    if (event->type == SDL_EVENT_MOUSE_BUTTON_DOWN || event->type == SDL_EVENT_MOUSE_BUTTON_UP) {
        bool down = event->type == SDL_EVENT_MOUSE_BUTTON_DOWN;
        if (event->button.button == SDL_BUTTON_LEFT) {
            int item = item_at(view, event->button.x, event->button.y);
            if (down) {
                view->radial_pressed = item;
                if (item < 0) {
                    pui_view_close_radial(view);
                }
            } else {
                view->radial_pending = item == view->radial_pressed ? item : -1;
                view->radial_open = false;
            }
        } else if (down) {
            pui_view_close_radial(view);
        }
        // Consume this entire click; it must never start a gesture behind the menu.
        pui_cancel(input);
        return true;
    }
    return event->type == SDL_EVENT_MOUSE_WHEEL || event->type == SDL_EVENT_KEY_DOWN ||
           event->type == SDL_EVENT_KEY_UP || event->type == SDL_EVENT_TEXT_INPUT;
}

void pui_widget_radial_draw(PUIView *view, PUIContext *input, PUIAction action, void *context) {
    if (view->radial_pending >= 0) {
        int item = view->radial_pending;
        view->radial_pending = -1;
        if (action && *view->layout->nodes[item].action) {
            action(context, view->layout->nodes[item].action, view->layout->nodes[item].name);
        }
    }
    if (!view->radial_open) {
        return;
    }
    pui_view_blur(view, input);
    pui_cancel(input);
    int count = 0;
    const PUINode *menu = &view->layout->nodes[view->radial_index];
    for (int k = menu->child; k >= 0; k = view->layout->nodes[k].next) {
        count++;
    }
    PUIRect b = view->radial_bounds;
    float cx = b.x + b.w / 2, cy = b.y + b.h / 2, radius = b.w / 2;
    int hovered = item_at(view, view->radial_x, view->radial_y);
    SDL_Rect previous;
    bool clipped = SDL_RenderClipEnabled(view->renderer);
    SDL_GetRenderClipRect(view->renderer, &previous);
    SDL_SetRenderClipRect(view->renderer, NULL);
    int index = 0;
    for (int k = menu->child; k >= 0; k = view->layout->nodes[k].next, index++) {
        const PUINode *node = &view->layout->nodes[k];
        bool enabled = node->enabled && !view->state[k].radial_disabled;
        float angle = -1.570796327f + index * 6.283185307f / count;
        float half = 3.141592654f / count;
        uint32_t packed = !enabled       ? pui_theme.panel
                          : hovered == k ? pui_theme.accent
                                         : pui_theme.surface;
        SDL_FColor color = {(packed >> 24) / 255.f, ((packed >> 16) & 255) / 255.f,
                            ((packed >> 8) & 255) / 255.f, 1};
        for (int segment = 0; segment < 20; segment++) {
            float a = angle - half + .018f + (2 * half - .036f) * segment / 20;
            float z = angle - half + .018f + (2 * half - .036f) * (segment + 1) / 20;
            SDL_Vertex vertices[4] = {
                {.position = {cx + cosf(a) * radius, cy + sinf(a) * radius}, .color = color},
                {.position = {cx + cosf(z) * radius, cy + sinf(z) * radius}, .color = color},
                {.position = {cx + cosf(z) * radius * .24f, cy + sinf(z) * radius * .24f},
                 .color = color},
                {.position = {cx + cosf(a) * radius * .24f, cy + sinf(a) * radius * .24f},
                 .color = color}};
            const int indices[6] = {0, 1, 2, 0, 2, 3};
            SDL_RenderGeometry(view->renderer, NULL, vertices, 4, indices, 6);
        }
        float x = cx + cosf(angle) * radius * .65f;
        float y = cy + sinf(angle) * radius * .65f;
        uint32_t foreground = !enabled       ? pui_theme.disabled
                              : hovered == k ? pui_theme.canvas
                                             : pui_theme.text;
        pui_icon(view->renderer, pui_icon_from_name(node->icon), x - 9, y - 20, 18, foreground);
        pui_text_scaled(view->renderer, x - pui_text_width(node->text) * .875f / 2, y + 4,
                        node->text, foreground, .875f);
    }
    SDL_FColor center_color = {(pui_theme.panel >> 24) / 255.f,
                               ((pui_theme.panel >> 16) & 255) / 255.f,
                               ((pui_theme.panel >> 8) & 255) / 255.f, 1};
    for (int i = 0; i < 40; i++) {
        float a = i * 6.283185307f / 40, b = (i + 1) * 6.283185307f / 40;
        SDL_Vertex triangle[3] = {
            {.position = {cx, cy}, .color = center_color},
            {.position = {cx + cosf(a) * radius * .24f, cy + sinf(a) * radius * .24f},
             .color = center_color},
            {.position = {cx + cosf(b) * radius * .24f, cy + sinf(b) * radius * .24f},
             .color = center_color}};
        SDL_RenderGeometry(view->renderer, NULL, triangle, 3, NULL, 0);
    }
    pui_text_scaled(view->renderer, cx - 10, cy - 6, "Esc", pui_theme.text, .875f);
    SDL_SetRenderClipRect(view->renderer, clipped ? &previous : NULL);
}
