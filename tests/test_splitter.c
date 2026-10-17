#include "pengui/pengui.h"
#include "unity.h"
#include <math.h>

void setUp(void) {}

void tearDown(void) {}

static void test_three_columns_keep_outer_widths_when_maximized(void) {
    float preferred[] = {240, 0, 280}, minimum[] = {160, 200, 160};
    PUIRegions small =
        pui_split_layout((PUIRect){0, 0, 1000, 600}, PUI_AXIS_X, 3, 1, preferred, minimum, 8);
    PUIRegions large =
        pui_split_layout((PUIRect){0, 0, 1800, 600}, PUI_AXIS_X, 3, 1, preferred, minimum, 8);
    TEST_ASSERT_EQUAL_FLOAT(240, small.panes[0].w);
    TEST_ASSERT_EQUAL_FLOAT(280, small.panes[2].w);
    TEST_ASSERT_EQUAL_FLOAT(small.panes[0].w, large.panes[0].w);
    TEST_ASSERT_EQUAL_FLOAT(small.panes[2].w, large.panes[2].w);
    TEST_ASSERT_EQUAL_FLOAT(small.panes[1].w + 800, large.panes[1].w);
    TEST_ASSERT_EQUAL_FLOAT(1800, large.panes[2].x + large.panes[2].w);
}

static void test_three_rows_keep_top_and_bottom_heights(void) {
    float preferred[] = {100, 0, 180}, minimum[] = {60, 100, 80};
    PUIRegions a =
        pui_split_layout((PUIRect){10, 20, 500, 700}, PUI_AXIS_Y, 3, 1, preferred, minimum, 9);
    PUIRegions b =
        pui_split_layout((PUIRect){10, 20, 500, 1000}, PUI_AXIS_Y, 3, 1, preferred, minimum, 9);
    TEST_ASSERT_EQUAL_FLOAT(100, b.panes[0].h);
    TEST_ASSERT_EQUAL_FLOAT(180, b.panes[2].h);
    TEST_ASSERT_EQUAL_FLOAT(a.panes[1].h + 300, b.panes[1].h);
    TEST_ASSERT_EQUAL_FLOAT(1020, b.panes[2].y + b.panes[2].h);
}

static void test_two_panes_can_keep_either_side_fixed(void) {
    float preferred[] = {240, 180, 0}, minimum[] = {80, 80, 0};
    for (int axis = 0; axis < 2; axis++) {
        PUIRegions first =
            pui_split_layout((PUIRect){0, 0, 900, 900}, (PUIAxis)axis, 2, 1, preferred, minimum, 8);
        PUIRegions last =
            pui_split_layout((PUIRect){0, 0, 900, 900}, (PUIAxis)axis, 2, 0, preferred, minimum, 8);
        TEST_ASSERT_EQUAL_FLOAT(240, axis ? first.panes[0].h : first.panes[0].w);
        TEST_ASSERT_EQUAL_FLOAT(180, axis ? last.panes[1].h : last.panes[1].w);
    }
}

static void test_tiny_window_stays_bounded_and_does_not_destroy_preferences(void) {
    float preferred[] = {240, 0, 280}, minimum[] = {160, 200, 160};
    PUIRegions tiny =
        pui_split_layout((PUIRect){0, 0, 30, 20}, PUI_AXIS_X, 3, 1, preferred, minimum, 9);
    float total = 0;
    for (int i = 0; i < 3; i++) {
        TEST_ASSERT_TRUE(isfinite(tiny.panes[i].w) && tiny.panes[i].w >= 0);
        total += tiny.panes[i].w;
    }
    TEST_ASSERT_FLOAT_WITHIN(.001, 30, total + tiny.grips[0].w + tiny.grips[1].w);
    TEST_ASSERT_EQUAL_FLOAT(240, preferred[0]);
    TEST_ASSERT_EQUAL_FLOAT(280, preferred[2]);
    PUIRegions restored =
        pui_split_layout((PUIRect){0, 0, 1200, 600}, PUI_AXIS_X, 3, 1, preferred, minimum, 9);
    TEST_ASSERT_EQUAL_FLOAT(240, restored.panes[0].w);
    TEST_ASSERT_EQUAL_FLOAT(280, restored.panes[2].w);
}

static void test_all_flexible_positions_and_invalid_counts(void) {
    float preferred[] = {100, 120, 140}, minimum[] = {30, 30, 30};
    for (int flex = 0; flex < 3; flex++) {
        PUIRegions a =
            pui_split_layout((PUIRect){0, 0, 600, 300}, PUI_AXIS_X, 3, flex, preferred, minimum, 5);
        PUIRegions b =
            pui_split_layout((PUIRect){0, 0, 800, 300}, PUI_AXIS_X, 3, flex, preferred, minimum, 5);
        for (int i = 0; i < 3; i++) {
            TEST_ASSERT_EQUAL_FLOAT(a.panes[i].w + (i == flex ? 200 : 0), b.panes[i].w);
        }
    }
    TEST_ASSERT_EQUAL_INT(
        0, pui_split_layout((PUIRect){0}, PUI_AXIS_X, 4, 1, preferred, minimum, 3).count);
    TEST_ASSERT_EQUAL_INT(
        0, pui_split_layout((PUIRect){0}, PUI_AXIS_X, 3, 3, preferred, minimum, 3).count);
}

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_three_columns_keep_outer_widths_when_maximized);
    RUN_TEST(test_three_rows_keep_top_and_bottom_heights);
    RUN_TEST(test_two_panes_can_keep_either_side_fixed);
    RUN_TEST(test_tiny_window_stays_bounded_and_does_not_destroy_preferences);
    RUN_TEST(test_all_flexible_positions_and_invalid_counts);
    return UNITY_END();
}
