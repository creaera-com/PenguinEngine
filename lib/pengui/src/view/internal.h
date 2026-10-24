/** @file @brief Private state shared only by the SDL view implementation. */
#ifndef PUI_VIEW_INTERNAL_H
#define PUI_VIEW_INTERNAL_H
#include "pengui/pengui.h"
#include "rendering/internal.h"
#include "widgets/color_picker.h"

typedef struct {
    PUITreeItem *items;
    bool *open;
    int count;
} TreeData;

typedef struct {
    PUIColorPickerState color;
    TreeData *tree;
    bool open, radial_disabled;
    bool checked;
    bool commit, keyboard_navigation, reveal;
    int highlighted;
    int group;      ///< Parent button-group index plus one; zero for independent controls.
    PUIId selected; ///< Persistent selection for each tree, independent of keyboard focus.
    float preferred[3];
    char text[256];
    PUITextEdit edit;
    double input_base; ///< Last committed/set numeric value for relative arithmetic.
    PUIColorTransition button_background, button_foreground, button_focus;
    bool input_invalid;
    PUITabs tabs;
    PUIScroll scroll;
    SDL_Texture *image;
    SDL_Texture *modal_layer; ///< Temporary dialog snapshot, released after its fade-out.
    PUIColorTransition modal_opacity;
    double modal_drawn_ms; ///< Avoid compositing an X-closed modal twice in one frame.
} Widget;

struct PUIView {
    const PUILayout *layout;
    PUILayout *owned_layout; ///< Composed copy after slot replacement.
    SDL_Renderer *renderer;
    SDL_Window *window;
    PUICanvasRenderer canvas_renderer;
    void *canvas_context;
    const PUIContext *canvas_input; ///< Borrowed only during the canvas draw callback.
    Widget state[PUI_LAYOUT_NODES];
    PUIRect rect[PUI_LAYOUT_NODES], hit[PUI_LAYOUT_NODES];
    bool visible[PUI_LAYOUT_NODES], enabled[PUI_LAYOUT_NODES];
    int focused, submit;
    int selection_clicks;
    size_t selection_start, selection_end;

    struct {
        int index;
        bool pending, active, committing;
        Uint64 started;
        float x, y;
        double value;
        char original[256];
    } numeric_drag; ///< Transient long-press state; never retained across layout replacement.

    bool selecting; ///< Mouse selection belongs to the focused text field until release.
    int popups[PUI_LAYOUT_DEPTH], popup_count;
    PUIRect popup_rects[PUI_LAYOUT_DEPTH], bounds;
    bool radial_open;
    PUIColorTransition radial_opacity; ///< Visual fade continues after modal input closes.
    int radial_index, radial_pressed, radial_pending;
    Uint8 radial_release_button;
    int radial_parents[PUI_LAYOUT_DEPTH], radial_depth;
    PUIRect radial_bounds;
    float radial_x, radial_y;
    SDL_SystemCursor cursor;
    int cursor_priority;
    bool rotation_cursor_requested, rotation_cursor_attempted;
    SDL_Cursor *rotation_cursor; ///< Lazily created curved arrow, owned by this view.
    bool pointer_inside;
    double frame_ms;
    SDL_Cursor *cursors[SDL_SYSTEM_CURSOR_COUNT];
    bool cursor_attempted[SDL_SYSTEM_CURSOR_COUNT];
};

bool pui_widget_radial_event(PUIView *view, PUIContext *input, const SDL_Event *event);
void pui_widget_radial_draw(PUIView *view, PUIContext *input, PUIAction action, void *context);
void pui_widget_color_picker_finish_frame(PUIView *view, PUIContext *input, PUIAction action,
                                          void *context);
void pui_widget_color_picker_init(PUIView *view, int index);
void pui_widget_color_picker_draw(PUIView *view, PUIContext *input, int index, PUIRect area,
                                  PUIAction action, void *context);
bool pui_widget_color_picker_event(PUIView *view, PUIContext *input, const SDL_Event *event);
bool pui_widget_color_picker_owns(const PUIView *view, int input);
void pui_widget_color_picker_cancel(PUIView *view, int index);
void pui_widget_tree_clear(Widget *state);
void pui_widget_dropdown_init(PUIView *view, int index);
void pui_widget_dropdown_draw(PUIView *v, PUIContext *pui, int i, PUIRect area, PUIAction action,
                              void *context);
void pui_widget_dropdown_list(PUIView *v, PUIContext *pui, int i, PUIRect area, PUIAction action,
                              void *context);
bool pui_widget_dropdown_key(PUIView *view, const SDL_KeyboardEvent *key);
void pui_widget_popup_button_draw(PUIView *v, PUIContext *pui, int i, PUIRect area,
                                  PUIAction action, void *context);
void pui_view_popup_open(PUIView *view, PUIContext *pui, int index);
void pui_view_popup_close(PUIView *view, PUIContext *pui);
bool pui_view_popup_prepare(PUIView *view, PUIContext *pui);
void pui_view_popup_draw(PUIView *view, PUIContext *pui, PUIAction action, void *context);
/** @brief Paint cached closing modals without reactivating their widgets. */
void pui_modal_draw_closing(PUIView *view);
/** @brief Center a modal in its configured widget, clamped to the visible view. */
PUIRect pui_modal_rect(const PUIView *view, int index);
/** @brief Draw modal chrome/content; false when its close button dismissed it. */
bool pui_modal_draw(PUIView *view, PUIContext *input, int index, PUIRect rect, PUIAction action,
                    void *context);
bool pui_view_popup_event(PUIView *view, PUIContext *pui, const SDL_Event *event);
PUIContext pui_view_inert_context(const PUIContext *pui);
void pui_view_apply_cursor(PUIView *view, const PUIContext *pui);
void pui_view_free_cursors(PUIView *view);
void pui_view_blur(PUIView *view, PUIContext *pui);
void pui_view_draw_node(PUIView *v, PUIContext *pui, int i, PUIRect area, PUIAction action,
                        void *context);
void pui_view_draw_children(PUIView *v, PUIContext *pui, int i, PUIRect area, bool row,
                            PUIAction action, void *context);
void pui_view_draw_container(PUIView *v, PUIContext *pui, int i, PUIRect area, PUIAction action,
                             void *context);
void pui_widget_toggle_draw(PUIView *v, PUIContext *pui, int i, PUIRect area, PUIAction action,
                            void *context);
void pui_widget_checkbox_draw(PUIView *v, PUIContext *pui, int i, PUIRect area, PUIAction action,
                              void *context);
void pui_widget_button_group_init(PUIView *view, int index);
void pui_widget_button_group_draw(PUIView *v, PUIContext *pui, int i, PUIRect area,
                                  PUIAction action, void *context);
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
bool pui_widget_text_input_drag(PUIView *view, PUIContext *pui, const SDL_Event *event);
void pui_numeric_drag_cancel(PUIView *view);
void pui_numeric_drag_begin(PUIView *view, const SDL_Event *event);
void pui_numeric_drag_update(PUIView *view, PUIContext *input, int index);
bool pui_numeric_drag_event(PUIView *view, PUIContext *input, const SDL_Event *event);
bool pui_widget_text_input_event(PUIView *view, PUIContext *pui, const SDL_Event *event);
bool pui_widget_tree_scroll(PUIView *view, int index, const SDL_Event *event);
void pui_widget_tabs_draw_drag(PUIView *view, PUIContext *pui);
bool pui_widget_tabs_scroll(PUIView *view, int index, const SDL_Event *event);
void pui_widget_tabs_init(PUIView *view, int index);
void pui_widget_section_draw(PUIView *v, PUIContext *pui, int i, PUIRect area, PUIAction action,
                             void *context);

/** @brief Shared node styling for ordinary, grouped, image, toggle and popup buttons. */
void pui_widget_button_paint(PUIView *view, PUIContext *input, int index, PUIRect area, bool active,
                             PUIIcon icon);

#endif
