/** @file @brief Modal, YAML-configured radial context menu with deferred action dispatch. */
#include "view/internal.h"
#include <math.h>
#include <string.h>

/* Reserve all declared rings up front so opening a branch never moves the center. */
static int menu_depth(const PUIView *view, int menu) {
    int depth = 1;
    for (int k = view->layout->nodes[menu].child; k >= 0; k = view->layout->nodes[k].next) {
        if (view->layout->nodes[k].type == PUI_NODE_RADIAL_MENU) {
            int child = 1 + menu_depth(view, k);
            if (child > depth) {
                depth = child;
            }
        }
    }
    return depth;
}

static int ring_menu(const PUIView *view, int level) {
    return level < view->radial_depth ? view->radial_parents[level] : view->radial_index;
}

static void ring_radii(const PUIView *view, int level, float *inner, float *outer) {
    float scale =
        view->radial_bounds.w / (2 * (108 + 64 * (menu_depth(view, ring_menu(view, 0)) - 1)));
    *inner = (level ? 112 + 64 * (level - 1) : 42) * scale;
    *outer = (108 + 64 * level) * scale;
}

bool pui_view_open_radial(PUIView *view, const char *id, float x, float y) {
    if (!isfinite(x) || !isfinite(y)) {
        return false;
    }
    for (int i = 0; i < view->layout->count; i++) {
        const PUINode *node = &view->layout->nodes[i];
        if (node->type == PUI_NODE_RADIAL_MENU && node->enabled && !strcmp(node->name, id)) {
            float preferred = node->popup_width > 0 ? node->popup_width / 320.f : 1;
            float size = fminf(2 * (108 + 64 * (menu_depth(view, i) - 1)) * preferred,
                               fminf(view->bounds.w, view->bounds.h - 36));
            if (size < 100) {
                return false;
            }
            view->radial_bounds = (PUIRect){
                fmaxf(view->bounds.x, fminf(x - size / 2, view->bounds.x + view->bounds.w - size)),
                fmaxf(view->bounds.y,
                      fminf(y - size / 2, view->bounds.y + view->bounds.h - size - 36)),
                size, size};
            view->radial_release_button = 0;
            view->radial_open = true;
            view->radial_index = i;
            view->radial_depth = 0;
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
    view->radial_depth = 0;
}

static int ring_item(const PUIView *view, int level, float x, float y) {
    PUIRect b = view->radial_bounds;
    float inner, outer;
    ring_radii(view, level, &inner, &outer);
    float distance = hypotf(x - b.x - b.w / 2, y - b.y - b.h / 2);
    if (distance < inner || distance > outer) {
        return -1;
    }
    int count = 0, items[8];
    for (int k = view->layout->nodes[ring_menu(view, level)].child; k >= 0;
         k = view->layout->nodes[k].next) {
        if (count == 8) {
            return -1;
        }
        items[count++] = k;
    }
    PUIRect bounds = {b.x + b.w / 2 - outer, b.y + b.h / 2 - outer, outer * 2, outer * 2};
    int index = pui_radial_item(bounds, count, x, y);
    return index < 0 ? -1 : items[index];
}

static int item_at(PUIView *view, float x, float y) {
    for (int level = 0; level <= view->radial_depth; level++) {
        int item = ring_item(view, level, x, y);
        if (item >= 0 && view->layout->nodes[item].enabled && !view->state[item].radial_disabled) {
            return item;
        }
    }
    return -1;
}

bool pui_view_radial_item_center(const PUIView *view, const char *id, float *x, float *y) {
    if (!view || !view->radial_open || !id || !x || !y) {
        return false;
    }
    for (int level = 0; level <= view->radial_depth; level++) {
        int count = 0;
        int menu = ring_menu(view, level);
        for (int k = view->layout->nodes[menu].child; k >= 0; k = view->layout->nodes[k].next) {
            count++;
        }
        int index = 0;
        for (int k = view->layout->nodes[menu].child; k >= 0;
             k = view->layout->nodes[k].next, index++) {
            if (!strcmp(view->layout->nodes[k].name, id)) {
                float inner, outer;
                ring_radii(view, level, &inner, &outer);
                float angle = -1.570796327f + index * 6.283185307f / count;
                *x = view->radial_bounds.x + view->radial_bounds.w / 2 +
                     cosf(angle) * (inner + outer) / 2;
                *y = view->radial_bounds.y + view->radial_bounds.h / 2 +
                     sinf(angle) * (inner + outer) / 2;
                return true;
            }
        }
    }
    return false;
}

static bool in_center(const PUIView *view, float x, float y) {
    float inner, outer;
    ring_radii(view, 0, &inner, &outer);
    PUIRect b = view->radial_bounds;
    return hypotf(x - b.x - b.w / 2, y - b.y - b.h / 2) < inner;
}

static void back(PUIView *view) {
    if (view->radial_depth > 0) {
        view->radial_index = view->radial_parents[--view->radial_depth];
        view->radial_open = true;
        view->radial_pressed = view->radial_pending = -1;
    } else {
        pui_view_close_radial(view);
    }
}

bool pui_widget_radial_event(PUIView *view, PUIContext *input, const SDL_Event *event) {
    if (!view->radial_open) {
        /* A dismissing press also owns its release, even after an intervening frame. */
        if (event->type == SDL_EVENT_MOUSE_BUTTON_UP &&
            event->button.button == view->radial_release_button) {
            view->radial_release_button = 0;
            pui_cancel(input);
            return true;
        }
        if (event->type == SDL_EVENT_MOUSE_BUTTON_DOWN ||
            event->type == SDL_EVENT_WINDOW_FOCUS_LOST) {
            view->radial_release_button = 0;
        }
        return false;
    }
    if (event->type == SDL_EVENT_WINDOW_FOCUS_LOST ||
        (event->type == SDL_EVENT_KEY_DOWN && event->key.key == SDLK_ESCAPE)) {
        if (event->type == SDL_EVENT_WINDOW_FOCUS_LOST) {
            pui_view_close_radial(view);
        } else {
            back(view);
        }
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
            if (view->radial_depth > 0 && in_center(view, event->button.x, event->button.y)) {
                item = -2; // Center navigates back; never dispatch an object action.
            }
            if (down) {
                view->radial_pressed = item;
                if (item == -1) {
                    pui_view_close_radial(view);
                    view->radial_release_button = event->button.button;
                }
            } else if (item == -2 && view->radial_pressed == -2) {
                back(view);
            } else {
                view->radial_pending = item >= 0 && item == view->radial_pressed ? item : -1;
                view->radial_open = false;
            }
        } else if (down) {
            pui_view_close_radial(view);
            view->radial_release_button = event->button.button;
        }
        // Consume this entire click; it must never start a gesture behind the menu.
        pui_cancel(input);
        return true;
    }
    return event->type == SDL_EVENT_MOUSE_WHEEL || event->type == SDL_EVENT_KEY_DOWN ||
           event->type == SDL_EVENT_KEY_UP || event->type == SDL_EVENT_TEXT_INPUT;
}

/* Slightly translucent surfaces; icons, labels and active outlines retain full contrast. */
static uint32_t translucent(uint32_t color) {
    return (color & 0xffffff00u) | ((color & 255u) * 235u / 255u);
}

/* Thin annular sectors share geometry for the background and the accent edge. */
static void draw_sector(PUIView *view, float cx, float cy, float inner, float outer, float angle,
                        float half, uint32_t packed) {
    SDL_FColor color = {(packed >> 24) / 255.f, ((packed >> 16) & 255) / 255.f,
                        ((packed >> 8) & 255) / 255.f, (packed & 255u) / 255.f};
    float gap = inner > 0 ? .018f : 0;
    for (int segment = 0; segment < 24; segment++) {
        float a = angle - half + gap + (2 * half - 2 * gap) * segment / 24;
        float z = angle - half + gap + (2 * half - 2 * gap) * (segment + 1) / 24;
        SDL_Vertex vertices[4] = {
            {.position = {cx + cosf(a) * outer, cy + sinf(a) * outer}, .color = color},
            {.position = {cx + cosf(z) * outer, cy + sinf(z) * outer}, .color = color},
            {.position = {cx + cosf(z) * inner, cy + sinf(z) * inner}, .color = color},
            {.position = {cx + cosf(a) * inner, cy + sinf(a) * inner}, .color = color}};
        const int indices[6] = {0, 1, 2, 0, 2, 3};
        SDL_RenderGeometry(view->renderer, NULL, vertices, 4, indices, 6);
    }
}

/* A radial chevron identifies navigation independently of hover or the active branch. */
static void draw_submenu_hint(PUIView *view, float cx, float cy, float radius, float angle,
                              uint32_t color) {
    float dx = cosf(angle), dy = sinf(angle);
    float x = cx + dx * radius, y = cy + dy * radius;
    SDL_SetRenderDrawColor(view->renderer, color >> 24, (color >> 16) & 255, (color >> 8) & 255,
                           color & 255);
    SDL_RenderLine(view->renderer, x - dx * 3 - dy * 3, y - dy * 3 + dx * 3, x + dx, y + dy);
    SDL_RenderLine(view->renderer, x + dx, y + dy, x - dx * 3 + dy * 3, y - dy * 3 - dx * 3);
}

void pui_widget_radial_draw(PUIView *view, PUIContext *input, PUIAction action, void *context) {
    if (view->radial_pending >= 0) {
        int item = view->radial_pending;
        view->radial_pending = -1;
        const PUINode *node = &view->layout->nodes[item];
        if (node->type == PUI_NODE_RADIAL_MENU && view->radial_depth < PUI_LAYOUT_DEPTH) {
            /* Retain ancestors, replacing only the branch outside the clicked ring. */
            for (int level = 0; level <= view->radial_depth; level++) {
                int menu = ring_menu(view, level);
                bool found = false;
                for (int k = view->layout->nodes[menu].child; k >= 0;
                     k = view->layout->nodes[k].next) {
                    if (k == item) {
                        found = true;
                        break;
                    }
                }
                if (found) {
                    view->radial_depth = level;
                    view->radial_index = menu;
                    break;
                }
            }
            view->radial_parents[view->radial_depth++] = view->radial_index;
            view->radial_index = item;
            view->radial_pressed = -1;
            view->radial_open = true;
        } else if (action && *node->action) {
            action(context, node->action, node->name);
        }
    }
    if (!view->radial_open) {
        return;
    }
    pui_view_blur(view, input);
    pui_cancel(input);
    PUIRect b = view->radial_bounds;
    float cx = b.x + b.w / 2, cy = b.y + b.h / 2;
    int hovered = item_at(view, view->radial_x, view->radial_y);
    SDL_Rect previous;
    bool clipped = SDL_RenderClipEnabled(view->renderer);
    SDL_GetRenderClipRect(view->renderer, &previous);
    SDL_SetRenderClipRect(view->renderer, NULL);
    SDL_BlendMode previous_blend;
    SDL_GetRenderDrawBlendMode(view->renderer, &previous_blend);
    SDL_SetRenderDrawBlendMode(view->renderer, SDL_BLENDMODE_BLEND);
    for (int level = 0; level <= view->radial_depth; level++) {
        float inner, outer;
        ring_radii(view, level, &inner, &outer);
        const PUINode *menu = &view->layout->nodes[ring_menu(view, level)];
        int count = 0;
        for (int k = menu->child; k >= 0; k = view->layout->nodes[k].next) {
            count++;
        }
        int index = 0;
        for (int k = menu->child; k >= 0; k = view->layout->nodes[k].next, index++) {
            const PUINode *node = &view->layout->nodes[k];
            bool enabled = node->enabled && !view->state[k].radial_disabled;
            bool active = level < view->radial_depth && k == ring_menu(view, level + 1);
            float angle = -1.570796327f + index * 6.283185307f / count;
            float half = 3.141592654f / count;
            uint32_t fill = hovered == k ? pui_theme.hover : pui_theme.surface;
            draw_sector(view, cx, cy, inner, outer, angle, half, translucent(fill));
            if (active || hovered == k) {
                draw_sector(view, cx, cy, outer - 2, outer, angle, half, pui_theme.accent);
            }
            float x = cx + cosf(angle) * (inner + outer) / 2;
            float y = cy + sinf(angle) * (inner + outer) / 2;
            uint32_t foreground = !enabled ? pui_theme.disabled
                                  : active ? pui_theme.accent
                                           : pui_theme.text;
            if (node->type == PUI_NODE_RADIAL_MENU) {
                draw_submenu_hint(view, cx, cy, outer - 8, angle,
                                  enabled ? pui_theme.accent : pui_theme.disabled);
            }
            bool has_icon = *node->icon != 0;
            if (has_icon) {
                pui_icon(view->renderer, pui_icon_from_name(node->icon), x - 9, y - 9, 18,
                         foreground);
            } else {
                pui_text_scaled(view->renderer, x - pui_text_width(node->text) * .7f / 2, y - 5,
                                node->text, foreground, .7f);
            }
        }
    }
    float center_inner, center_outer;
    ring_radii(view, 0, &center_inner, &center_outer);
    draw_sector(view, cx, cy, 0, center_inner - 2, 0, 3.141592654f, translucent(pui_theme.panel));
    /* Full labels stay legible without inflating every sector. */
    const char *caption = hovered >= 0 ? view->layout->nodes[hovered].text : "";
    if (*caption) {
        float inner, outer;
        ring_radii(view, view->radial_depth, &inner, &outer);
        float width = pui_text_width(caption) * .875f;
        pui_panel(view->renderer, (SDL_FRect){cx - width / 2 - 8, cy + outer + 6, width + 16, 24},
                  translucent(pui_theme.panel));
        pui_text_scaled(view->renderer, cx - width / 2, cy + outer + 12, caption, pui_theme.text,
                        .875f);
    }
    const char *center = view->radial_depth ? "Zurueck" : "Esc";
    pui_text_scaled(view->renderer, cx - pui_text_width(center) * .75f / 2, cy - 5, center,
                    pui_theme.text, .75f);
    SDL_SetRenderDrawBlendMode(view->renderer, previous_blend);
    SDL_SetRenderClipRect(view->renderer, clipped ? &previous : NULL);
}
