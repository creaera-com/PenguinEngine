/** @file @brief Private state shared only by the SDL view implementation. */
#ifndef PUI_VIEW_INTERNAL_H
#define PUI_VIEW_INTERNAL_H
#include "pengui/pengui.h"
#include "rendering/internal.h"

typedef struct {
    bool open;
    PUIId selected; ///< Persistent selection for each tree, independent of keyboard focus.
    float preferred[3];
    char text[256];
    PUITextEdit edit;
    PUITabs tabs;
    PUIScroll scroll;
    SDL_Texture *image;
} Widget;

struct PUIView {
    const PUILayout *layout;
    SDL_Renderer *renderer;
    SDL_Window *window;
    Widget state[PUI_LAYOUT_NODES];
    PUIRect rect[PUI_LAYOUT_NODES], hit[PUI_LAYOUT_NODES];
    bool visible[PUI_LAYOUT_NODES], enabled[PUI_LAYOUT_NODES];
    int focused, submit;
};

void pui_view_blur(PUIView *view, PUIContext *pui);
void pui_view_draw_node(PUIView *v, PUIContext *pui, int i, PUIRect area, PUIAction action,
                        void *context);
void pui_view_draw_children(PUIView *v, PUIContext *pui, int i, PUIRect area, bool row,
                            PUIAction action, void *context);
void pui_view_draw_container(PUIView *v, PUIContext *pui, int i, PUIRect area, PUIAction action,
                             void *context);
void pui_widget_button_draw(PUIView *v, PUIContext *pui, int i, PUIRect area, PUIAction action,
                            void *context);
void pui_widget_splitter_draw(PUIView *v, PUIContext *pui, int i, PUIRect area, PUIAction action,
                              void *context);
void pui_widget_tabs_draw(PUIView *v, PUIContext *pui, int i, PUIRect area, PUIAction action,
                          void *context);
void pui_widget_text_input_draw(PUIView *v, PUIContext *pui, int i, PUIRect area, PUIAction action,
                                void *context);
void pui_widget_tree_draw(PUIView *v, PUIContext *pui, int i, PUIRect area, PUIAction action,
                          void *context);
void pui_widget_basic_draw(PUIView *v, PUIContext *pui, int i, PUIRect area, PUIAction action,
                           void *context);
float pui_view_node_height(const PUIView *view, int index);
void pui_widget_text_input_pointer(PUIView *view, PUIContext *pui, SDL_Window *window,
                                   const SDL_Event *event);
bool pui_widget_text_input_event(PUIView *view, PUIContext *pui, const SDL_Event *event);
bool pui_widget_tree_scroll(PUIView *view, int index, const SDL_Event *event);
bool pui_widget_tabs_scroll(PUIView *view, int index, const SDL_Event *event);
void pui_widget_tabs_init(PUIView *view, int index);
void pui_widget_section_draw(PUIView *v, PUIContext *pui, int i, PUIRect area, PUIAction action,
                             void *context);
#endif
