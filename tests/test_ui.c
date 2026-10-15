#include "penguin/ui.h"
#include "unity.h"
#include <math.h>
#include <string.h>
static UiContext ui;
static UiRect button = {10, 10, 100, 30};
void setUp(void) {
    ui_init(&ui);
}
void tearDown(void) {}
static void test_button_release(void) {
    ui_pointer(&ui, 20, 20, true);
    TEST_ASSERT_FALSE(ui_button(&ui, 1, button, true));
    TEST_ASSERT_EQUAL_UINT(1, ui.active);
    ui_end_frame(&ui);
    ui_pointer(&ui, 20, 20, false);
    TEST_ASSERT_TRUE(ui_button(&ui, 1, button, true));
    TEST_ASSERT_EQUAL_UINT(0, ui.active);
}
static void test_fast_click(void) {
    ui_pointer(&ui, 20, 20, true);
    ui_pointer(&ui, 20, 20, false);
    TEST_ASSERT_TRUE(ui_button(&ui, 1, button, true));
    TEST_ASSERT_FALSE(ui_button(&ui, 1, button, true));
}
static void test_press_origin(void) {
    ui_pointer(&ui, 500, 500, true);
    ui_pointer(&ui, 20, 20, false);
    TEST_ASSERT_FALSE(ui_button(&ui, 1, button, true));
}
static void test_release_outside(void) {
    ui_pointer(&ui, 20, 20, true);
    ui_button(&ui, 1, button, true);
    ui_end_frame(&ui);
    ui_pointer(&ui, 500, 500, false);
    TEST_ASSERT_FALSE(ui_button(&ui, 1, button, true));
    TEST_ASSERT_EQUAL_UINT(0, ui.active);
}
static void test_disabled_and_blocked(void) {
    ui_pointer(&ui, 20, 20, true);
    TEST_ASSERT_FALSE(ui_button(&ui, 1, button, false));
    ui.blocked = true;
    TEST_ASSERT_FALSE(ui_button(&ui, 1, button, true));
    TEST_ASSERT_EQUAL_UINT(0, ui.active);
}
static void test_capture_is_exclusive(void) {
    ui_pointer(&ui, 20, 20, true);
    ui_button(&ui, 1, button, true);
    TEST_ASSERT_FALSE(ui_button(&ui, 2, button, true));
    TEST_ASSERT_EQUAL_UINT(1, ui.active);
}
static void test_focus_keyboard(void) {
    ui_focus(&ui, 2);
    ui_activate(&ui);
    TEST_ASSERT_FALSE(ui_button(&ui, 1, button, true));
    TEST_ASSERT_TRUE(ui_button(&ui, 2, button, true));
    TEST_ASSERT_FALSE(ui_button(&ui, 2, button, true));
}
static void test_cancel_capture(void) {
    ui_pointer(&ui, 20, 20, true);
    ui_button(&ui, 1, button, true);
    ui_cancel(&ui);
    ui_pointer(&ui, 20, 20, false);
    TEST_ASSERT_FALSE(ui_button(&ui, 1, button, true));
}
static void test_removed_control_releases_capture(void) {
    ui_pointer(&ui, 20, 20, true);
    ui_button(&ui, 1, button, true);
    ui_end_frame(&ui);
    ui_end_frame(&ui);
    TEST_ASSERT_EQUAL_UINT(0, ui.active);
}
static void test_clip_intersection(void) {
    TEST_ASSERT_TRUE(ui_clip_push(&ui, (UiRect){0, 0, 50, 50}));
    TEST_ASSERT_TRUE(ui_clip_push(&ui, (UiRect){25, 25, 50, 50}));
    TEST_ASSERT_EQUAL_FLOAT(25, ui.clips[1].w);
    ui_pointer(&ui, 15, 15, true);
    TEST_ASSERT_FALSE(ui_button(&ui, 1, button, true));
    ui_clip_pop(&ui);
    TEST_ASSERT_FALSE(ui_button(&ui, 1, button, true));
    ui_pointer(&ui, 15, 15, false);
    TEST_ASSERT_TRUE(ui_button(&ui, 1, button, true));
}
static void test_clip_stack_limit(void) {
    for (int i = 0; i < UI_MAX_CLIPS; i++)
        TEST_ASSERT_TRUE(ui_clip_push(&ui, button));
    TEST_ASSERT_FALSE(ui_clip_push(&ui, button));
    for (int i = 0; i < UI_MAX_CLIPS + 1; i++)
        ui_clip_pop(&ui);
    TEST_ASSERT_EQUAL_INT(0, ui.clip_depth);
}
static void test_disclosure(void) {
    bool open = true;
    ui_pointer(&ui, 20, 20, true);
    ui_pointer(&ui, 20, 20, false);
    TEST_ASSERT_TRUE(ui_disclosure(&ui, 1, button, &open, true));
    TEST_ASSERT_FALSE(open);
}
static void test_drag_outside_and_clamp(void) {
    float value = 200;
    ui_pointer(&ui, 20, 20, true);
    ui_drag_value(&ui, 1, button, &value, 100, 400, UI_AXIS_X, 1);
    ui_end_frame(&ui);
    ui_pointer(&ui, 520, 20, true);
    TEST_ASSERT_TRUE(ui_drag_value(&ui, 1, button, &value, 100, 400, UI_AXIS_X, 1));
    TEST_ASSERT_EQUAL_FLOAT(400, value);
    ui_end_frame(&ui);
    ui_pointer(&ui, -500, 20, false);
    ui_drag_value(&ui, 1, button, &value, 100, 400, UI_AXIS_X, 1);
    TEST_ASSERT_EQUAL_FLOAT(100, value);
    TEST_ASSERT_EQUAL_UINT(0, ui.active);
}
static void test_reverse_drag(void) {
    ui_focus(&ui, 9);
    float value = 200;
    ui_pointer(&ui, 20, 20, true);
    ui_drag_value(&ui, 1, button, &value, 100, 400, UI_AXIS_X, -1);
    ui_end_frame(&ui);
    ui_pointer(&ui, 60, 20, false);
    ui_drag_value(&ui, 1, button, &value, 100, 400, UI_AXIS_X, -1);
    TEST_ASSERT_EQUAL_FLOAT(160, value);
    TEST_ASSERT_EQUAL_UINT(9, ui.focus);
}
static void test_column_constraints(void) {
    float left = 900, right = 900;
    ui_columns_fit(1100, &left, &right, 192, 264, 320);
    TEST_ASSERT_TRUE(left >= 192 && right >= 264);
    TEST_ASSERT_TRUE(1100 - left - right >= 320);
}
static void test_tiny_column_layout(void) {
    float l = 240, r = 280;
    ui_columns_fit(400, &l, &r, 192, 264, 320);
    TEST_ASSERT_FLOAT_WITHIN(.01f, 80, l + r);
    ui_columns_fit(0, &l, &r, 192, 264, 320);
    TEST_ASSERT_EQUAL_FLOAT(0, l + r);
}
static void test_scroll_clamp(void) {
    UiScroll s = {999, 500, 200};
    ui_scroll_clamp(&s);
    TEST_ASSERT_EQUAL_FLOAT(300, s.offset);
    ui_scroll_by(&s, -999);
    TEST_ASSERT_EQUAL_FLOAT(0, s.offset);
    s.content = 100;
    ui_scroll_by(&s, 99);
    TEST_ASSERT_EQUAL_FLOAT(0, s.offset);
}
static void test_thumb_size(void) {
    UiScroll s = {150, 1000, 200};
    UiRect thumb = ui_scroll_thumb(&s, (UiRect){0, 0, 8, 200}, UI_AXIS_Y);
    TEST_ASSERT_EQUAL_FLOAT(40, thumb.h);
    TEST_ASSERT_EQUAL_FLOAT(30, thumb.y);
}
static void test_thumb_drag(void) {
    UiScroll s = {0, 1000, 200};
    UiRect track = {0, 0, 8, 200};
    ui_pointer(&ui, 4, 10, true);
    ui_scrollbar(&ui, 1, track, &s, UI_AXIS_Y);
    ui_end_frame(&ui);
    ui_pointer(&ui, 4, 90, false);
    ui_scrollbar(&ui, 1, track, &s, UI_AXIS_Y);
    TEST_ASSERT_FLOAT_WITHIN(.01f, 400, s.offset);
}
static void test_track_pages(void) {
    UiScroll s = {0, 1000, 200};
    ui_pointer(&ui, 4, 150, true);
    ui_pointer(&ui, 4, 150, false);
    ui_scrollbar(&ui, 1, (UiRect){0, 0, 8, 200}, &s, UI_AXIS_Y);
    TEST_ASSERT_EQUAL_FLOAT(200, s.offset);
}
static void test_horizontal_thumb(void) {
    UiScroll s = {50, 400, 100};
    UiRect thumb = ui_scroll_thumb(&s, (UiRect){10, 10, 200, 8}, UI_AXIS_X);
    TEST_ASSERT_EQUAL_FLOAT(50, thumb.w);
    TEST_ASSERT_EQUAL_FLOAT(35, thumb.x);
}
static void test_tab_selection(void) {
    UiTab items[] = {{1, true, 0}, {2, true, 0}};
    UiTabs tabs;
    TEST_ASSERT_TRUE(ui_tabs_init(&tabs, items, 2, 1));
    ui_pointer(&ui, 140, 12, true);
    ui_pointer(&ui, 140, 12, false);
    TEST_ASSERT_TRUE(ui_tabs_update(&ui, &tabs, (UiRect){0, 0, 300, 25}, 110));
    TEST_ASSERT_EQUAL_UINT(2, tabs.selected);
}
static void test_tab_reorder_preserves_identity(void) {
    UiTab items[] = {{1, true, 0}, {2, true, 0}, {3, true, 0}};
    UiTabs t;
    ui_tabs_init(&t, items, 3, 1);
    ui_pointer(&ui, 20, 12, true);
    ui_tabs_update(&ui, &t, (UiRect){0, 0, 400, 25}, 110);
    ui_end_frame(&ui);
    ui_pointer(&ui, 300, 12, true);
    ui_tabs_update(&ui, &t, (UiRect){0, 0, 400, 25}, 110);
    TEST_ASSERT_EQUAL_UINT(1, t.items[2].id);
    TEST_ASSERT_EQUAL_UINT(1, t.selected);
    ui_end_frame(&ui);
    ui_pointer(&ui, 300, 12, false);
    ui_tabs_update(&ui, &t, (UiRect){0, 0, 400, 25}, 110);
    TEST_ASSERT_EQUAL_UINT(0, ui.active);
}
static void test_disabled_tab(void) {
    UiTab items[] = {{1, true, 0}, {2, false, 0}};
    UiTabs t;
    ui_tabs_init(&t, items, 2, 1);
    ui_pointer(&ui, 140, 12, true);
    ui_pointer(&ui, 140, 12, false);
    TEST_ASSERT_FALSE(ui_tabs_update(&ui, &t, (UiRect){0, 0, 300, 25}, 110));
    TEST_ASSERT_EQUAL_UINT(1, t.selected);
}
static void test_tab_scroll_and_duplicates(void) {
    UiTab items[] = {{1, true, 0}, {2, true, 0}};
    UiTabs t;
    ui_tabs_init(&t, items, 2, 1);
    ui_tabs_scroll(&t, 100, 110, 999);
    TEST_ASSERT_EQUAL_FLOAT(116, t.scroll);
    ui_tabs_scroll(&t, 100, 110, -999);
    TEST_ASSERT_EQUAL_FLOAT(0, t.scroll);
    items[1].id = 1;
    TEST_ASSERT_FALSE(ui_tabs_init(&t, items, 2, 1));
    TEST_ASSERT_FALSE(ui_tabs_init(&t, items, 0, 1));
}
static void test_variable_tab_geometry_and_scroll(void) {
    UiTab items[] = {{1, true, 64}, {2, true, 144}, {3, true, 80}};
    UiTabs t;
    TEST_ASSERT_TRUE(ui_tabs_init(&t, items, 3, 1));
    UiRect bar = {10, 20, 200, 26};
    UiRect r = ui_tab_rect(&t, bar, 110, 1);
    TEST_ASSERT_EQUAL_FLOAT(74, r.x);
    TEST_ASSERT_EQUAL_FLOAT(140, r.w);
    ui_tabs_scroll(&t, 200, 110, 999);
    TEST_ASSERT_EQUAL_FLOAT(84, t.scroll);
    TEST_ASSERT_TRUE(ui_tabs_set_width(&t, 2, 100));
    ui_tabs_scroll(&t, 200, 110, 0);
    TEST_ASSERT_EQUAL_FLOAT(40, t.scroll);
    TEST_ASSERT_FALSE(ui_tabs_set_width(&t, 2, NAN));
    TEST_ASSERT_FALSE(ui_tabs_set_width(&t, 2, 4));
    TEST_ASSERT_FALSE(ui_tabs_set_width(&t, 99, 100));
    items[0].width = -1;
    TEST_ASSERT_FALSE(ui_tabs_init(&t, items, 3, 1));
}
static void test_variable_tab_hit_and_reorder(void) {
    UiTab items[] = {{1, true, 64}, {2, true, 144}, {3, true, 80}};
    UiTabs t;
    UiRect bar = {0, 0, 400, 26};
    ui_tabs_init(&t, items, 3, 1);
    ui_pointer(&ui, 150, 12, true);
    ui_tabs_update(&ui, &t, bar, 110);
    ui_end_frame(&ui);
    ui_pointer(&ui, 150, 12, false);
    TEST_ASSERT_TRUE(ui_tabs_update(&ui, &t, bar, 110));
    TEST_ASSERT_EQUAL_UINT(2, t.selected);
    ui_end_frame(&ui);
    ui_pointer(&ui, 20, 12, true);
    ui_tabs_update(&ui, &t, bar, 110);
    ui_end_frame(&ui);
    ui_pointer(&ui, 150, 12, true);
    ui_tabs_update(&ui, &t, bar, 110);
    TEST_ASSERT_EQUAL_UINT(1, t.items[1].id);
    TEST_ASSERT_EQUAL_FLOAT(64, t.items[1].width);
    /* Repeated frames with the pointer still held must not reverse the reorder. */
    ui_end_frame(&ui);
    ui_tabs_update(&ui, &t, bar, 110);
    TEST_ASSERT_EQUAL_UINT(1, t.items[1].id);
    ui_pointer(&ui, 150, 12, false);
    ui_tabs_update(&ui, &t, bar, 110);
    TEST_ASSERT_EQUAL_UINT(1, t.selected);
}
static void test_hidden_focused_button(void) {
    ui_focus(&ui, 1);
    ui_activate(&ui);
    ui_clip_push(&ui, (UiRect){200, 200, 50, 50});
    TEST_ASSERT_FALSE(ui_button(&ui, 1, button, true));
}
int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_button_release);
    RUN_TEST(test_fast_click);
    RUN_TEST(test_press_origin);
    RUN_TEST(test_release_outside);
    RUN_TEST(test_disabled_and_blocked);
    RUN_TEST(test_capture_is_exclusive);
    RUN_TEST(test_focus_keyboard);
    RUN_TEST(test_cancel_capture);
    RUN_TEST(test_removed_control_releases_capture);
    RUN_TEST(test_clip_intersection);
    RUN_TEST(test_clip_stack_limit);
    RUN_TEST(test_disclosure);
    RUN_TEST(test_drag_outside_and_clamp);
    RUN_TEST(test_reverse_drag);
    RUN_TEST(test_column_constraints);
    RUN_TEST(test_tiny_column_layout);
    RUN_TEST(test_scroll_clamp);
    RUN_TEST(test_thumb_size);
    RUN_TEST(test_thumb_drag);
    RUN_TEST(test_track_pages);
    RUN_TEST(test_horizontal_thumb);
    RUN_TEST(test_tab_selection);
    RUN_TEST(test_tab_reorder_preserves_identity);
    RUN_TEST(test_disabled_tab);
    RUN_TEST(test_tab_scroll_and_duplicates);
    RUN_TEST(test_hidden_focused_button);
    RUN_TEST(test_variable_tab_geometry_and_scroll);
    RUN_TEST(test_variable_tab_hit_and_reorder);
    return UNITY_END();
}
