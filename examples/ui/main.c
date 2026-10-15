#include "penguin/ui_sdl.h"
#include <SDL3/SDL_main.h>
#include <stdio.h>
#include <string.h>
static void text(SDL_Renderer *r, float x, float y, const char *s, Uint32 c) {
    SDL_SetRenderDrawColor(r, (Uint8)(c >> 24), (Uint8)(c >> 16), (Uint8)(c >> 8), (Uint8)c);
    ui_text(r, x, y, s, c);
}
int main(int argc, char **argv) {
    bool smoke = argc == 2 && !strcmp(argv[1], "--smoke");
    if (argc > 1 && !smoke)
        return 1;
    if (!SDL_Init(SDL_INIT_VIDEO))
        return 1;
    SDL_Window *window =
        SDL_CreateWindow("Penguin UI - reusable C widgets", 960, 640, SDL_WINDOW_RESIZABLE);
    SDL_Renderer *r = window ? SDL_CreateRenderer(window, NULL) : NULL;
    if (!r) {
        SDL_DestroyWindow(window);
        SDL_Quit();
        return 1;
    }
    SDL_SetWindowMinimumSize(window, 720, 480);
    SDL_SetRenderVSync(r, 1);
    UiContext context;
    ui_init(&context);
    UiTabs tabs;
    UiTab items[] = {{101, true}, {102, true}, {103, true}};
    ui_tabs_init(&tabs, items, 3, 101);
    UiScroll scroll = {0, 50 * 32, 500};
    float left = 220, right = 200;
    bool running = true, open = true, checked = false;
    int clicks = 0, frames = 0;
    while (running) {
        SDL_Event e;
        while (SDL_PollEvent(&e)) {
            if (e.type == SDL_EVENT_QUIT)
                running = false;
            else if (e.type == SDL_EVENT_WINDOW_FOCUS_LOST)
                ui_cancel(&context);
            else if (e.type == SDL_EVENT_MOUSE_MOTION)
                ui_pointer(&context, e.motion.x, e.motion.y, context.down);
            else if ((e.type == SDL_EVENT_MOUSE_BUTTON_DOWN ||
                      e.type == SDL_EVENT_MOUSE_BUTTON_UP) &&
                     e.button.button == SDL_BUTTON_LEFT)
                ui_pointer(&context, e.button.x, e.button.y, e.type == SDL_EVENT_MOUSE_BUTTON_DOWN);
            else if (e.type == SDL_EVENT_KEY_DOWN && !e.key.repeat) {
                if (e.key.key == SDLK_ESCAPE)
                    running = false;
                else if (e.key.key == SDLK_RETURN || e.key.key == SDLK_SPACE)
                    ui_activate(&context);
            } else if (e.type == SDL_EVENT_MOUSE_WHEEL && e.wheel.mouse_x < left)
                ui_scroll_by(&scroll, -e.wheel.y * 64);
        }
        int width, height;
        SDL_GetWindowSize(window, &width, &height);
        ui_columns_fit((float)width, &left, &right, 160, 160, 320);
        ui_drag_value(&context, 1, (UiRect){left - 3, 0, 6, (float)height}, &left, 160,
                      width - right - 320, UI_AXIS_X, 1);
        ui_drag_value(&context, 2, (UiRect){width - right - 3, 0, 6, (float)height}, &right, 160,
                      width - left - 320, UI_AXIS_X, -1);
        ui_panel(r, (SDL_FRect){0, 0, (float)width, (float)height}, UI_CANVAS);
        ui_panel(r, (SDL_FRect){0, 0, left, (float)height}, UI_PANEL);
        ui_panel(r, (SDL_FRect){width - right, 0, right, (float)height}, UI_PANEL);
        text(r, 14, 18, "SCROLLABLE LIST", UI_MUTED);
        scroll.viewport = height - 60;
        ui_scroll_clamp(&scroll);
        UiRect track = {left - 12, 48, 8, scroll.viewport};
        ui_scrollbar(&context, 3, track, &scroll, UI_AXIS_Y);
        ui_clip_push(&context, (UiRect){8, 48, left - 28, scroll.viewport});
        ui_apply_clip(r, &context);
        for (int i = 0; i < 50; i++) {
            float y = 50 + i * 32 - scroll.offset;
            UiRect b = {12, y, left - 36, 26};
            char label[32];
            snprintf(label, sizeof label, "Item %02d", i + 1);
            ui_draw_button(r, (SDL_FRect){b.x, b.y, b.w, b.h}, ICON_NONE, label, false, true,
                           ui_hovered(&context, b));
            if (ui_button(&context, 1000u + (UiId)i, b, true))
                clicks++;
        }
        ui_clip_pop(&context);
        ui_apply_clip(r, &context);
        ui_draw_scrollbar(r, &context, 3, track, &scroll, UI_AXIS_Y);
        UiRect bar = {left + 14, 12, width - left - right - 28, 28};
        ui_tabs_update(&context, &tabs, bar, 110);
        ui_clip_push(&context, bar);
        ui_apply_clip(r, &context);
        for (int i = 0; i < tabs.count; i++) {
            UiRect b = ui_tab_rect(&tabs, bar, 110, i);
            const char *label = tabs.items[i].id == 101   ? "Menu"
                                : tabs.items[i].id == 102 ? "Widgets"
                                                          : "Layout";
            ui_draw_tab(r, (SDL_FRect){b.x, b.y, b.w, b.h}, ICON_NONE, label,
                        tabs.selected == tabs.items[i].id, true, ui_hovered(&context, b));
        }
        ui_clip_pop(&context);
        ui_apply_clip(r, &context);
        float x = left + 20;
        UiRect button = {x, 76, 140, 32};
        ui_draw_button(r, (SDL_FRect){button.x, button.y, button.w, button.h}, ICON_PLAY, "Play",
                       checked, true, ui_hovered(&context, button));
        if (ui_button(&context, 10, button, true)) {
            checked = !checked;
            clicks++;
        }
        UiRect header = {x, 134, width - left - right - 40, 28};
        ui_disclosure(&context, 11, header, &open, true);
        ui_draw_button(r, (SDL_FRect){header.x, header.y, header.w, header.h},
                       open ? ICON_DOWN : ICON_RIGHT, "Details", false, true,
                       ui_hovered(&context, header));
        if (open) {
            text(r, x + 8, 182, "Flat surfaces, square corners.", UI_TEXT);
            text(r, x + 8, 206, "Drag tabs or panel dividers.", UI_MUTED);
            text(r, x + 8, 230, "Scroll wheel and draggable thumbs.", UI_MUTED);
            text(r, x + 8, 254, "Focused buttons: Enter / Space.", UI_MUTED);
        }
        char info[64];
        snprintf(info, sizeof info, "Clicks: %d", clicks);
        text(r, width - right + 14, 20, "SHARED C LIBRARY", UI_ACCENT);
        text(r, width - right + 14, 54, info, UI_TEXT);
        text(r, width - right + 14, 86, "No engine / ECS", UI_MUTED);
        text(r, width - right + 14, 106, "or scripts needed.", UI_MUTED);
        ui_panel(r, (SDL_FRect){left - 1, 0, 2, (float)height},
                 context.active == 1 ? UI_ACCENT : UI_BORDER);
        ui_panel(r, (SDL_FRect){width - right - 1, 0, 2, (float)height},
                 context.active == 2 ? UI_ACCENT : UI_BORDER);
        ui_end_frame(&context);
        SDL_RenderPresent(r);
        SDL_Delay(1);
        if (smoke && ++frames >= 3)
            running = false;
    }
    SDL_DestroyRenderer(r);
    SDL_DestroyWindow(window);
    SDL_Quit();
    return 0;
}
