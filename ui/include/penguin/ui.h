#ifndef PENGUIN_UI_H
#define PENGUIN_UI_H
#include <stdbool.h>
#include <stdint.h>
#define UI_MAX_CLIPS 16
#define UI_MAX_TABS 16
#define UI_TAB_GAP 2
typedef uint32_t UiId;
typedef struct {
    float x, y, w, h;
} UiRect;
typedef enum { UI_AXIS_X, UI_AXIS_Y } UiAxis;
typedef struct {
    float x, y, press_x, press_y, release_x, release_y;
    bool down, pressed, released, activate, blocked, active_seen;
    UiId active, focus;
    float drag_origin, drag_value;
    bool dragging;
    UiRect clips[UI_MAX_CLIPS];
    int clip_depth;
} UiContext;
typedef struct {
    float offset, content, viewport;
} UiScroll;
typedef struct {
    UiId id;
    bool enabled;
    float width; /* Slot width including the 2 px gap; zero uses the fallback width. */
} UiTab;
typedef struct {
    UiTab items[UI_MAX_TABS];
    int count;
    UiId selected;
    float scroll;
    bool dragging;
} UiTabs;
UiId ui_id(const char *label);
void ui_init(UiContext *context);
/* Feed transitions in event order. A press and release may occur before the next draw. */
void ui_pointer(UiContext *context, float x, float y, bool down);
void ui_cancel(UiContext *context);
void ui_focus(UiContext *context, UiId id);
void ui_activate(UiContext *context);
void ui_end_frame(UiContext *context);
bool ui_contains(UiRect rect, float x, float y);
bool ui_hovered(const UiContext *context, UiRect rect);
bool ui_clip_push(UiContext *context, UiRect clip);
void ui_clip_pop(UiContext *context);
/* Stable, nonzero IDs must be unique within a context. Activation is on release. */
bool ui_button(UiContext *context, UiId id, UiRect rect, bool enabled);
bool ui_disclosure(UiContext *context, UiId id, UiRect rect, bool *open, bool enabled);
bool ui_drag_value(UiContext *context, UiId id, UiRect rect, float *value, float minimum,
                   float maximum, UiAxis axis, float scale);
void ui_columns_fit(float total, float *left, float *right, float min_left, float min_right,
                    float min_center);
float ui_scroll_max(const UiScroll *scroll);
void ui_scroll_clamp(UiScroll *scroll);
void ui_scroll_by(UiScroll *scroll, float delta);
UiRect ui_scroll_thumb(const UiScroll *scroll, UiRect track, UiAxis axis);
bool ui_scrollbar(UiContext *context, UiId id, UiRect track, UiScroll *scroll, UiAxis axis);
bool ui_tabs_init(UiTabs *tabs, const UiTab *items, int count, UiId selected);
bool ui_tabs_set_width(UiTabs *tabs, UiId id, float width);
UiRect ui_tab_rect(const UiTabs *tabs, UiRect bar, float width, int index);
void ui_tabs_scroll(UiTabs *tabs, float viewport, float tab_width, float delta);
/* Reorders IDs while dragging; selected ID and the associated content remain stable. */
bool ui_tabs_update(UiContext *context, UiTabs *tabs, UiRect bar, float tab_width);
#endif
