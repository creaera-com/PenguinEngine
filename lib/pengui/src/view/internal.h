/** @file @brief Private state shared only by the SDL view implementation. */
#ifndef PENGUI_VIEW_INTERNAL_H
#define PENGUI_VIEW_INTERNAL_H
#include "pengui/pengui.h"
#include "rendering/internal.h"

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

void ui_view_blur(UiView *view, UiContext *ui);
void ui_view_draw_node(UiView *v, UiContext *ui, int i, UiRect area, UiAction action,
                       void *context);
void ui_view_draw_children(UiView *v, UiContext *ui, int i, UiRect area, bool row, UiAction action,
                           void *context);
void ui_view_draw_container(UiView *v, UiContext *ui, int i, UiRect area, UiAction action,
                            void *context);
void ui_widget_button_draw(UiView *v, UiContext *ui, int i, UiRect area, UiAction action,
                           void *context);
void ui_widget_splitter_draw(UiView *v, UiContext *ui, int i, UiRect area, UiAction action,
                             void *context);
void ui_widget_tabs_draw(UiView *v, UiContext *ui, int i, UiRect area, UiAction action,
                         void *context);
void ui_widget_text_input_draw(UiView *v, UiContext *ui, int i, UiRect area, UiAction action,
                               void *context);
void ui_widget_tree_draw(UiView *v, UiContext *ui, int i, UiRect area, UiAction action,
                         void *context);
void ui_widget_basic_draw(UiView *v, UiContext *ui, int i, UiRect area, UiAction action,
                          void *context);
float ui_view_node_height(const UiView *view, int index);
void ui_widget_text_input_pointer(UiView *view, UiContext *ui, SDL_Window *window,
                                  const SDL_Event *event);
bool ui_widget_text_input_event(UiView *view, UiContext *ui, const SDL_Event *event);
bool ui_widget_tree_scroll(UiView *view, int index, const SDL_Event *event);
bool ui_widget_tabs_scroll(UiView *view, int index, const SDL_Event *event);
void ui_widget_tabs_init(UiView *view, int index);
void ui_widget_section_draw(UiView *v, UiContext *ui, int i, UiRect area, UiAction action,
                            void *context);
#endif
