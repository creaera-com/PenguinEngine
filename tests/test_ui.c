#include "pengui/pengui.h"
#include "unity.h"
#include <math.h>
#include <string.h>
static PUIContext pui;
static PUIRect button = {10, 10, 100, 30};

void setUp(void) {
    pui_init(&pui);
}

void tearDown(void) {}

static void test_button_release(void) {
    pui_pointer(&pui, 20, 20, true);
    TEST_ASSERT_FALSE(pui_button(&pui, 1, button, true));
    TEST_ASSERT_EQUAL_UINT(1, pui.active);
    pui_end_frame(&pui);
    pui_pointer(&pui, 20, 20, false);
    TEST_ASSERT_TRUE(pui_button(&pui, 1, button, true));
    TEST_ASSERT_EQUAL_UINT(0, pui.active);
}

static void test_fast_click(void) {
    pui_pointer(&pui, 20, 20, true);
    pui_pointer(&pui, 20, 20, false);
    TEST_ASSERT_TRUE(pui_button(&pui, 1, button, true));
    TEST_ASSERT_FALSE(pui_button(&pui, 1, button, true));
}

static void test_press_origin(void) {
    pui_pointer(&pui, 500, 500, true);
    pui_pointer(&pui, 20, 20, false);
    TEST_ASSERT_FALSE(pui_button(&pui, 1, button, true));
}

static void test_release_outside(void) {
    pui_pointer(&pui, 20, 20, true);
    pui_button(&pui, 1, button, true);
    pui_end_frame(&pui);
    pui_pointer(&pui, 500, 500, false);
    TEST_ASSERT_FALSE(pui_button(&pui, 1, button, true));
    TEST_ASSERT_EQUAL_UINT(0, pui.active);
}

static void test_disabled_and_blocked(void) {
    pui_pointer(&pui, 20, 20, true);
    TEST_ASSERT_FALSE(pui_button(&pui, 1, button, false));
    pui.blocked = true;
    TEST_ASSERT_FALSE(pui_button(&pui, 1, button, true));
    TEST_ASSERT_EQUAL_UINT(0, pui.active);
}

static void test_capture_is_exclusive(void) {
    pui_pointer(&pui, 20, 20, true);
    pui_button(&pui, 1, button, true);
    TEST_ASSERT_FALSE(pui_button(&pui, 2, button, true));
    TEST_ASSERT_EQUAL_UINT(1, pui.active);
}

static void test_focus_keyboard(void) {
    pui_focus(&pui, 2);
    pui_activate(&pui);
    TEST_ASSERT_FALSE(pui_button(&pui, 1, button, true));
    TEST_ASSERT_TRUE(pui_button(&pui, 2, button, true));
    TEST_ASSERT_FALSE(pui_button(&pui, 2, button, true));
}

static void test_cancel_capture(void) {
    pui_pointer(&pui, 20, 20, true);
    pui_button(&pui, 1, button, true);
    pui_cancel(&pui);
    pui_pointer(&pui, 20, 20, false);
    TEST_ASSERT_FALSE(pui_button(&pui, 1, button, true));
}

static void test_removed_control_releases_capture(void) {
    pui_pointer(&pui, 20, 20, true);
    pui_button(&pui, 1, button, true);
    pui_end_frame(&pui);
    pui_end_frame(&pui);
    TEST_ASSERT_EQUAL_UINT(0, pui.active);
}

static void test_clip_intersection(void) {
    TEST_ASSERT_TRUE(pui_clip_push(&pui, (PUIRect){0, 0, 50, 50}));
    TEST_ASSERT_TRUE(pui_clip_push(&pui, (PUIRect){25, 25, 50, 50}));
    TEST_ASSERT_EQUAL_FLOAT(25, pui.clips[1].w);
    pui_pointer(&pui, 15, 15, true);
    TEST_ASSERT_FALSE(pui_button(&pui, 1, button, true));
    pui_clip_pop(&pui);
    TEST_ASSERT_FALSE(pui_button(&pui, 1, button, true));
    pui_pointer(&pui, 15, 15, false);
    TEST_ASSERT_TRUE(pui_button(&pui, 1, button, true));
}

static void test_clip_stack_limit(void) {
    for (int i = 0; i < PUI_MAX_CLIPS; i++) {
        TEST_ASSERT_TRUE(pui_clip_push(&pui, button));
    }
    TEST_ASSERT_FALSE(pui_clip_push(&pui, button));
    for (int i = 0; i < PUI_MAX_CLIPS + 1; i++) {
        pui_clip_pop(&pui);
    }
    TEST_ASSERT_EQUAL_INT(0, pui.clip_depth);
}

static void test_disclosure(void) {
    bool open = true;
    pui_pointer(&pui, 20, 20, true);
    pui_pointer(&pui, 20, 20, false);
    TEST_ASSERT_TRUE(pui_disclosure(&pui, 1, button, &open, true));
    TEST_ASSERT_FALSE(open);
}

static void test_drag_outside_and_clamp(void) {
    float value = 200;
    pui_pointer(&pui, 20, 20, true);
    pui_drag_value(&pui, 1, button, &value, 100, 400, PUI_AXIS_X, 1);
    pui_end_frame(&pui);
    pui_pointer(&pui, 520, 20, true);
    TEST_ASSERT_TRUE(pui_drag_value(&pui, 1, button, &value, 100, 400, PUI_AXIS_X, 1));
    TEST_ASSERT_EQUAL_FLOAT(400, value);
    pui_end_frame(&pui);
    pui_pointer(&pui, -500, 20, false);
    pui_drag_value(&pui, 1, button, &value, 100, 400, PUI_AXIS_X, 1);
    TEST_ASSERT_EQUAL_FLOAT(100, value);
    TEST_ASSERT_EQUAL_UINT(0, pui.active);
}

static void test_reverse_drag(void) {
    pui_focus(&pui, 9);
    float value = 200;
    pui_pointer(&pui, 20, 20, true);
    pui_drag_value(&pui, 1, button, &value, 100, 400, PUI_AXIS_X, -1);
    pui_end_frame(&pui);
    pui_pointer(&pui, 60, 20, false);
    pui_drag_value(&pui, 1, button, &value, 100, 400, PUI_AXIS_X, -1);
    TEST_ASSERT_EQUAL_FLOAT(160, value);
    TEST_ASSERT_EQUAL_UINT(9, pui.focus);
}

static void test_column_constraints(void) {
    float left = 900, right = 900;
    pui_columns_fit(1100, &left, &right, 192, 264, 320);
    TEST_ASSERT_TRUE(left >= 192 && right >= 264);
    TEST_ASSERT_TRUE(1100 - left - right >= 320);
}

static void test_tiny_column_layout(void) {
    float l = 240, r = 280;
    pui_columns_fit(400, &l, &r, 192, 264, 320);
    TEST_ASSERT_FLOAT_WITHIN(.01f, 80, l + r);
    pui_columns_fit(0, &l, &r, 192, 264, 320);
    TEST_ASSERT_EQUAL_FLOAT(0, l + r);
}

static void test_scroll_clamp(void) {
    PUIScroll s = {999, 500, 200};
    pui_scroll_clamp(&s);
    TEST_ASSERT_EQUAL_FLOAT(300, s.offset);
    pui_scroll_by(&s, -999);
    TEST_ASSERT_EQUAL_FLOAT(0, s.offset);
    s.content = 100;
    pui_scroll_by(&s, 99);
    TEST_ASSERT_EQUAL_FLOAT(0, s.offset);
}

static void test_thumb_size(void) {
    PUIScroll s = {150, 1000, 200};
    PUIRect thumb = pui_scroll_thumb(&s, (PUIRect){0, 0, 8, 200}, PUI_AXIS_Y);
    TEST_ASSERT_EQUAL_FLOAT(40, thumb.h);
    TEST_ASSERT_EQUAL_FLOAT(30, thumb.y);
}

static void test_thumb_drag(void) {
    PUIScroll s = {0, 1000, 200};
    PUIRect track = {0, 0, 8, 200};
    pui_pointer(&pui, 4, 10, true);
    pui_scrollbar(&pui, 1, track, &s, PUI_AXIS_Y);
    pui_end_frame(&pui);
    pui_pointer(&pui, 4, 90, false);
    pui_scrollbar(&pui, 1, track, &s, PUI_AXIS_Y);
    TEST_ASSERT_FLOAT_WITHIN(.01f, 400, s.offset);
}

static void test_track_pages(void) {
    PUIScroll s = {0, 1000, 200};
    pui_pointer(&pui, 4, 150, true);
    pui_pointer(&pui, 4, 150, false);
    pui_scrollbar(&pui, 1, (PUIRect){0, 0, 8, 200}, &s, PUI_AXIS_Y);
    TEST_ASSERT_EQUAL_FLOAT(200, s.offset);
}

static void test_horizontal_thumb(void) {
    PUIScroll s = {50, 400, 100};
    PUIRect thumb = pui_scroll_thumb(&s, (PUIRect){10, 10, 200, 8}, PUI_AXIS_X);
    TEST_ASSERT_EQUAL_FLOAT(50, thumb.w);
    TEST_ASSERT_EQUAL_FLOAT(35, thumb.x);
}

static void test_tab_selection(void) {
    PUITab items[] = {{1, true, 0}, {2, true, 0}};
    PUITabs tabs;
    TEST_ASSERT_TRUE(pui_tabs_init(&tabs, items, 2, 1));
    pui_pointer(&pui, 140, 12, true);
    pui_pointer(&pui, 140, 12, false);
    TEST_ASSERT_TRUE(pui_tabs_update(&pui, &tabs, (PUIRect){0, 0, 300, 25}, 110));
    TEST_ASSERT_EQUAL_UINT(2, tabs.selected);
}

static void test_tab_reorder_preserves_identity(void) {
    PUITab items[] = {{1, true, 0}, {2, true, 0}, {3, true, 0}};
    PUITabs t;
    pui_tabs_init(&t, items, 3, 1);
    pui_pointer(&pui, 20, 12, true);
    pui_tabs_update(&pui, &t, (PUIRect){0, 0, 400, 25}, 110);
    pui_end_frame(&pui);
    pui_pointer(&pui, 300, 12, true);
    pui_tabs_update(&pui, &t, (PUIRect){0, 0, 400, 25}, 110);
    TEST_ASSERT_EQUAL_UINT(1, t.items[2].id);
    TEST_ASSERT_EQUAL_UINT(1, t.selected);
    pui_end_frame(&pui);
    pui_pointer(&pui, 300, 12, false);
    pui_tabs_update(&pui, &t, (PUIRect){0, 0, 400, 25}, 110);
    TEST_ASSERT_EQUAL_UINT(0, pui.active);
}

static void test_disabled_tab(void) {
    PUITab items[] = {{1, true, 0}, {2, false, 0}};
    PUITabs t;
    pui_tabs_init(&t, items, 2, 1);
    pui_pointer(&pui, 140, 12, true);
    pui_pointer(&pui, 140, 12, false);
    TEST_ASSERT_FALSE(pui_tabs_update(&pui, &t, (PUIRect){0, 0, 300, 25}, 110));
    TEST_ASSERT_EQUAL_UINT(1, t.selected);
}

static void test_tab_scroll_and_duplicates(void) {
    PUITab items[] = {{1, true, 0}, {2, true, 0}};
    PUITabs t;
    pui_tabs_init(&t, items, 2, 1);
    pui_tabs_scroll(&t, 100, 110, 999);
    TEST_ASSERT_EQUAL_FLOAT(118, t.scroll);
    pui_tabs_scroll(&t, 100, 110, -999);
    TEST_ASSERT_EQUAL_FLOAT(0, t.scroll);
    items[1].id = 1;
    TEST_ASSERT_FALSE(pui_tabs_init(&t, items, 2, 1));
    TEST_ASSERT_FALSE(pui_tabs_init(&t, items, 0, 1));
}

static void test_variable_tab_geometry_and_scroll(void) {
    PUITab items[] = {{1, true, 64}, {2, true, 144}, {3, true, 80}};
    PUITabs t;
    TEST_ASSERT_TRUE(pui_tabs_init(&t, items, 3, 1));
    PUIRect bar = {10, 20, 200, 26};
    PUIRect r = pui_tab_rect(&t, bar, 110, 1);
    TEST_ASSERT_EQUAL_FLOAT(74, r.x);
    TEST_ASSERT_EQUAL_FLOAT(142, r.w);
    pui_tabs_scroll(&t, 200, 110, 999);
    TEST_ASSERT_EQUAL_FLOAT(86, t.scroll);
    TEST_ASSERT_TRUE(pui_tabs_set_width(&t, 2, 100));
    pui_tabs_scroll(&t, 200, 110, 0);
    TEST_ASSERT_EQUAL_FLOAT(42, t.scroll);
    TEST_ASSERT_FALSE(pui_tabs_set_width(&t, 2, NAN));
    TEST_ASSERT_FALSE(pui_tabs_set_width(&t, 2, PUI_TAB_GAP));
    TEST_ASSERT_FALSE(pui_tabs_set_width(&t, 99, 100));
    items[0].width = -1;
    TEST_ASSERT_FALSE(pui_tabs_init(&t, items, 3, 1));
}

static void test_variable_tab_hit_and_reorder(void) {
    PUITab items[] = {{1, true, 64}, {2, true, 144}, {3, true, 80}};
    PUITabs t;
    PUIRect bar = {0, 0, 400, 26};
    pui_tabs_init(&t, items, 3, 1);
    pui_pointer(&pui, 150, 12, true);
    pui_tabs_update(&pui, &t, bar, 110);
    pui_end_frame(&pui);
    pui_pointer(&pui, 150, 12, false);
    TEST_ASSERT_TRUE(pui_tabs_update(&pui, &t, bar, 110));
    TEST_ASSERT_EQUAL_UINT(2, t.selected);
    pui_end_frame(&pui);
    pui_pointer(&pui, 20, 12, true);
    pui_tabs_update(&pui, &t, bar, 110);
    pui_end_frame(&pui);
    pui_pointer(&pui, 150, 12, true);
    pui_tabs_update(&pui, &t, bar, 110);
    TEST_ASSERT_EQUAL_UINT(1, t.items[1].id);
    TEST_ASSERT_EQUAL_FLOAT(64, t.items[1].width);
    /* Repeated frames with the pointer still held must not reverse the reorder. */
    pui_end_frame(&pui);
    pui_tabs_update(&pui, &t, bar, 110);
    TEST_ASSERT_EQUAL_UINT(1, t.items[1].id);
    pui_pointer(&pui, 150, 12, false);
    pui_tabs_update(&pui, &t, bar, 110);
    TEST_ASSERT_EQUAL_UINT(1, t.selected);
}

static void test_hidden_focused_button(void) {
    pui_focus(&pui, 1);
    pui_activate(&pui);
    pui_clip_push(&pui, (PUIRect){200, 200, 50, 50});
    TEST_ASSERT_FALSE(pui_button(&pui, 1, button, true));
}

static void test_vertical_split_constraints(void) {
    PUISplit split = pui_split_rects((PUIRect){10, 20, 400, 600}, PUI_AXIS_Y, 180, 120, 80, 8);
    TEST_ASSERT_FLOAT_WITHIN(.01f, 412, split.first.h);
    TEST_ASSERT_FLOAT_WITHIN(.01f, 432, split.grip.y);
    TEST_ASSERT_FLOAT_WITHIN(.01f, 440, split.second.y);
    TEST_ASSERT_FLOAT_WITHIN(.01f, 180, split.second.h);
    split = pui_split_rects((PUIRect){0, 0, 400, 600}, PUI_AXIS_Y, 999, 120, 80, 8);
    TEST_ASSERT_FLOAT_WITHIN(.01f, 120, split.first.h);
    TEST_ASSERT_FLOAT_WITHIN(.01f, 472, split.second.h);
}

static void test_split_tiny_and_horizontal(void) {
    PUISplit split = pui_split_rects((PUIRect){0, 0, 20, 30}, PUI_AXIS_Y, 180, 120, 80, 8);
    TEST_ASSERT_TRUE(split.first.h >= 0 && split.second.h >= 0);
    TEST_ASSERT_FLOAT_WITHIN(.01f, 30, split.first.h + split.grip.h + split.second.h);
    split = pui_split_rects((PUIRect){5, 10, 500, 100}, PUI_AXIS_X, 150, 100, 80, 8);
    TEST_ASSERT_FLOAT_WITHIN(.01f, 342, split.first.w);
    TEST_ASSERT_FLOAT_WITHIN(.01f, 355, split.second.x);
}

static void test_split_drag_reduces_lower_panel(void) {
    float size = 180;
    PUISplit split = pui_split_rects((PUIRect){0, 0, 400, 600}, PUI_AXIS_Y, size, 120, 80, 8);
    pui_pointer(&pui, 200, split.grip.y + 2, true);
    pui_drag_value(&pui, 99, split.grip, &size, 80, 472, PUI_AXIS_Y, -1);
    pui_end_frame(&pui);
    pui_pointer(&pui, 200, split.grip.y + 42, true);
    TEST_ASSERT_TRUE(pui_drag_value(&pui, 99, split.grip, &size, 80, 472, PUI_AXIS_Y, -1));
    TEST_ASSERT_FLOAT_WITHIN(.01f, 140, size);
}

static void test_property_columns_align_and_resize(void) {
    PUIPropertyRow row = pui_property_row((PUIRect){10, 20, 240, 25}, .45f, 8);
    PUIPropertyRow next = pui_property_row((PUIRect){10, 49, 240, 25}, .45f, 8);
    TEST_ASSERT_FLOAT_WITHIN(.01f, row.value.x, next.value.x);
    TEST_ASSERT_FLOAT_WITHIN(.01f, 8, row.value.x - row.label.x - row.label.w);
    TEST_ASSERT_FLOAT_WITHIN(.01f, 250, row.value.x + row.value.w);
    PUIPropertyRow wider = pui_property_row((PUIRect){10, 20, 400, 25}, .45f, 8);
    TEST_ASSERT_TRUE(wider.label.w > row.label.w && wider.value.w > row.value.w);
    TEST_ASSERT_FLOAT_WITHIN(.01f, 410, wider.value.x + wider.value.w);
}

static void test_property_columns_tiny_and_clamped(void) {
    PUIPropertyRow row = pui_property_row((PUIRect){10, 20, 4, 25}, .45f, 8);
    TEST_ASSERT_FLOAT_WITHIN(.01f, 0, row.label.w);
    TEST_ASSERT_FLOAT_WITHIN(.01f, 0, row.value.w);
    TEST_ASSERT_FLOAT_WITHIN(.01f, 14, row.value.x);
    row = pui_property_row((PUIRect){10, 20, -4, -2}, .45f, -8);
    TEST_ASSERT_FLOAT_WITHIN(.01f, 0, row.label.w + row.value.w + row.value.h);
    row = pui_property_row((PUIRect){10, 20, 100, 25}, 2, -8);
    TEST_ASSERT_FLOAT_WITHIN(.01f, 100, row.label.w);
    TEST_ASSERT_FLOAT_WITHIN(.01f, 0, row.value.w);
}

static void test_text_input_insert_at_cursor(void) {
    char text[32] = "12.5";
    PUITextEdit edit;
    pui_text_edit_init(&edit, text, false);
    pui_text_edit_key(&edit, text, PUI_TEXT_HOME, false);
    TEST_ASSERT_TRUE(pui_text_edit_insert(&edit, text, sizeof text, "-"));
    TEST_ASSERT_EQUAL_STRING("-12.5", text);
    pui_text_edit_key(&edit, text, PUI_TEXT_END, false);
    pui_text_edit_key(&edit, text, PUI_TEXT_LEFT, false);
    TEST_ASSERT_TRUE(pui_text_edit_key(&edit, text, PUI_TEXT_DELETE, false));
    TEST_ASSERT_EQUAL_STRING("-12.", text);
    TEST_ASSERT_TRUE(pui_text_edit_insert(&edit, text, sizeof text, "25"));
    TEST_ASSERT_EQUAL_STRING("-12.25", text);
}

static void test_text_input_utf8_selection_and_delete(void) {
    char text[32] = "a\xc3\xa4"
                    "b";
    PUITextEdit edit;
    pui_text_edit_init(&edit, text, false);
    pui_text_edit_key(&edit, text, PUI_TEXT_LEFT, false);
    pui_text_edit_key(&edit, text, PUI_TEXT_LEFT, true);
    TEST_ASSERT_EQUAL_UINT(1, edit.cursor);
    TEST_ASSERT_EQUAL_UINT(3, edit.anchor);
    TEST_ASSERT_TRUE(pui_text_edit_key(&edit, text, PUI_TEXT_BACKSPACE, false));
    TEST_ASSERT_EQUAL_STRING("ab", text);
    TEST_ASSERT_TRUE(pui_text_edit_insert(&edit, text, sizeof text, "\xc3\xb6"));
    TEST_ASSERT_TRUE(pui_text_edit_key(&edit, text, PUI_TEXT_BACKSPACE, false));
    TEST_ASSERT_EQUAL_STRING("ab", text);
}

static void test_text_input_replacement_capacity_is_atomic(void) {
    char text[6] = "12345";
    PUITextEdit edit;
    pui_text_edit_init(&edit, text, true);
    TEST_ASSERT_FALSE(pui_text_edit_insert(&edit, text, sizeof text, "123456"));
    TEST_ASSERT_EQUAL_STRING("12345", text);
    TEST_ASSERT_TRUE(pui_text_edit_insert(&edit, text, sizeof text, "\xc3\xa4\xc3\xb6"));
    TEST_ASSERT_EQUAL_STRING("\xc3\xa4\xc3\xb6", text);
    TEST_ASSERT_FALSE(pui_text_edit_insert(&edit, text, sizeof text, "\xc3\xbc"));
    TEST_ASSERT_EQUAL_STRING("\xc3\xa4\xc3\xb6", text);
}

static void test_text_input_rejects_multiline_and_invalid_utf8(void) {
    char text[32] = "64";
    PUITextEdit edit;
    pui_text_edit_init(&edit, text, true);
    TEST_ASSERT_FALSE(pui_text_edit_insert(&edit, text, sizeof text, "1\n2"));
    TEST_ASSERT_FALSE(pui_text_edit_insert(&edit, text, sizeof text, "\t"));
    TEST_ASSERT_FALSE(pui_text_edit_insert(&edit, text, sizeof text, "\xc0\xaf"));
    TEST_ASSERT_FALSE(pui_text_edit_insert(&edit, text, sizeof text, "\xed\xa0\x80"));
    TEST_ASSERT_FALSE(pui_text_edit_insert(&edit, text, sizeof text, "\xf0\x9f"));
    TEST_ASSERT_EQUAL_STRING("64", text);
}

static void test_text_input_click_and_stale_offsets(void) {
    char text[32] = "a\xc3\xa4"
                    "b";
    PUITextEdit edit = {100, 100, 0};
    pui_text_edit_key(&edit, text, PUI_TEXT_LEFT, false);
    TEST_ASSERT_EQUAL_UINT(3, edit.cursor);
    pui_text_edit_click(&edit, text, 2, false);
    TEST_ASSERT_EQUAL_UINT(3, edit.cursor);
    pui_text_edit_click(&edit, text, 1, true);
    TEST_ASSERT_EQUAL_UINT(1, edit.cursor);
    TEST_ASSERT_EQUAL_UINT(3, edit.anchor);
    TEST_ASSERT_EQUAL_UINT(2, pui_text_edit_column(text, 3));
    TEST_ASSERT_EQUAL_UINT(3, pui_text_edit_column(text, 999));
    pui_text_edit_key(&edit, text, PUI_TEXT_RIGHT, false);
    TEST_ASSERT_EQUAL_UINT(3, edit.cursor);
    TEST_ASSERT_EQUAL_UINT(edit.cursor, edit.anchor);
}

static void test_tree_guides_follow_siblings_and_last_children(void) {
    const int depths[] = {0, 1, 2, 2, 1, 0, 1};
    TEST_ASSERT_TRUE(pui_tree_continues(depths, 7, 2, 2));
    TEST_ASSERT_FALSE(pui_tree_continues(depths, 7, 3, 2));
    TEST_ASSERT_TRUE(pui_tree_continues(depths, 7, 2, 1));
    TEST_ASSERT_FALSE(pui_tree_continues(depths, 7, 4, 1));
    TEST_ASSERT_FALSE(pui_tree_continues(depths, 7, 6, 1));
    const int collapsed[] = {0, 1, 1, 0, 1};
    TEST_ASSERT_TRUE(pui_tree_continues(collapsed, 5, 1, 1));
    TEST_ASSERT_FALSE(pui_tree_continues(collapsed, 5, 2, 1));
}

static void test_tree_guides_invalid_and_clipped_lists(void) {
    const int depths[] = {0, 1, 2, 1, 0};
    TEST_ASSERT_TRUE(pui_tree_continues(depths, 5, 2, 1));
    TEST_ASSERT_FALSE(pui_tree_continues(NULL, 0, 0, 0));
    TEST_ASSERT_FALSE(pui_tree_continues(depths, 5, -1, 0));
    TEST_ASSERT_FALSE(pui_tree_continues(depths, 5, 5, 0));
    TEST_ASSERT_FALSE(pui_tree_continues(depths, 5, 1, -1));
    TEST_ASSERT_FALSE(pui_tree_continues(depths, 5, 1, 2));
}

static void test_image_fit_preserves_aspect_and_state_margin(void) {
    PUIRect fit = pui_image_fit((PUIRect){10, 20, 32, 24}, 64, 32, 4);
    TEST_ASSERT_EQUAL_FLOAT(24, fit.w);
    TEST_ASSERT_EQUAL_FLOAT(12, fit.h);
    TEST_ASSERT_EQUAL_FLOAT(14, fit.x);
    TEST_ASSERT_EQUAL_FLOAT(26, fit.y);
    fit = pui_image_fit((PUIRect){0, 0, 32, 24}, 16, 64, 4);
    TEST_ASSERT_EQUAL_FLOAT(4, fit.w);
    TEST_ASSERT_EQUAL_FLOAT(16, fit.h);
    TEST_ASSERT_EQUAL_FLOAT(4, fit.y);
    fit = pui_image_fit((PUIRect){0, 0, 10, 10}, 1, 1, 0);
    TEST_ASSERT_EQUAL_FLOAT(2, fit.x);
    TEST_ASSERT_EQUAL_FLOAT(6, fit.w);
    TEST_ASSERT_EQUAL_FLOAT(0, pui_image_fit((PUIRect){0, 0, 4, 4}, 1, 1, 4).w);
    TEST_ASSERT_EQUAL_FLOAT(0, pui_image_fit((PUIRect){0, 0, 32, 24}, NAN, 1, 4).w);
}

static void test_toggle_and_radio_change_only_on_valid_activation(void) {
    bool checked = false;
    pui_pointer(&pui, 20, 20, true);
    pui_pointer(&pui, 20, 20, false);
    TEST_ASSERT_TRUE(pui_toggle(&pui, 1, button, &checked, true));
    TEST_ASSERT_TRUE(checked);
    TEST_ASSERT_FALSE(pui_toggle(&pui, 1, button, &checked, true));
    pui_end_frame(&pui);
    pui_focus(&pui, 1);
    pui_activate(&pui);
    TEST_ASSERT_TRUE(pui_toggle(&pui, 1, button, &checked, true));
    TEST_ASSERT_FALSE(checked);
    pui_end_frame(&pui);
    pui_activate(&pui);
    TEST_ASSERT_FALSE(pui_toggle(&pui, 1, button, &checked, false));
    TEST_ASSERT_FALSE(checked);
    pui_end_frame(&pui);
    PUIId selected = 2;
    pui_activate(&pui);
    TEST_ASSERT_TRUE(pui_radio(&pui, 1, button, &selected, true));
    TEST_ASSERT_EQUAL_UINT(1, selected);
    pui_end_frame(&pui);
    pui_activate(&pui);
    TEST_ASSERT_FALSE(pui_radio(&pui, 1, button, &selected, true));
    TEST_ASSERT_EQUAL_UINT(1, selected);
    TEST_ASSERT_FALSE(pui_toggle(&pui, 1, button, NULL, true));
    TEST_ASSERT_FALSE(pui_radio(&pui, 1, button, NULL, true));
}

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_toggle_and_radio_change_only_on_valid_activation);
    RUN_TEST(test_image_fit_preserves_aspect_and_state_margin);
    RUN_TEST(test_tree_guides_follow_siblings_and_last_children);
    RUN_TEST(test_tree_guides_invalid_and_clipped_lists);
    RUN_TEST(test_text_input_insert_at_cursor);
    RUN_TEST(test_text_input_utf8_selection_and_delete);
    RUN_TEST(test_text_input_replacement_capacity_is_atomic);
    RUN_TEST(test_text_input_rejects_multiline_and_invalid_utf8);
    RUN_TEST(test_text_input_click_and_stale_offsets);
    RUN_TEST(test_property_columns_align_and_resize);
    RUN_TEST(test_property_columns_tiny_and_clamped);
    RUN_TEST(test_vertical_split_constraints);
    RUN_TEST(test_split_tiny_and_horizontal);
    RUN_TEST(test_split_drag_reduces_lower_panel);
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
