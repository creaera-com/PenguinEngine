/** @file @brief Private state shared only by the SDL view implementation. */
#ifndef PENGUI_VIEW_INTERNAL_H
#define PENGUI_VIEW_INTERNAL_H
#include "pengui/view.h"

typedef struct {
    bool open;
    float extent;
    char text[256];
    UiTextEdit edit;
    UiTabs tabs;
    UiScroll scroll;
    SDL_Texture *image;
} Widget;

struct UiView {
    const UiLayout *layout;
    SDL_Renderer *renderer;
    SDL_Window *window;
    Widget state[UI_LAYOUT_NODES];
    UiRect rect[UI_LAYOUT_NODES], hit[UI_LAYOUT_NODES];
    bool visible[UI_LAYOUT_NODES], enabled[UI_LAYOUT_NODES];
    int focused, submit;
};

UiIcon ui_view_icon(const char *name);
void ui_view_blur(UiView *view, UiContext *ui);
#endif
