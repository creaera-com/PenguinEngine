/** @file @brief Independent geometry tests with no UI, editor, assets or SDL dependency. */
#include "pengdraw/pengdraw.h"
#include "unity.h"
#include <math.h>

void setUp(void) {}

void tearDown(void) {}

static void test_geometry_and_boolean_operations(void) {
    PDrawShape shape = {.corner_radius = 10};
    PDrawPath path = pdraw_shape_path(&shape, (PDrawRect){0, 0, 100, 60}, 0);
    TEST_ASSERT_TRUE(pdraw_path_valid(&path));
    TEST_ASSERT_FALSE(pdraw_path_contains(&path, 1, 1));
    TEST_ASSERT_TRUE(pdraw_path_contains(&path, 50, 30));
    PDrawPath band;
    TEST_ASSERT_TRUE(pdraw_shape_outline(&shape, (PDrawRect){0, 0, 100, 60}, 0, 4, -1, &band));
    TEST_ASSERT_FALSE(pdraw_path_contains(&band, 50, 30));
    PDrawPath moved = pdraw_shape_path(&shape, (PDrawRect){50, 0, 100, 60}, 0), combined;
    TEST_ASSERT_TRUE(pdraw_path_combine(&path, &moved, PDRAW_AREA_INTERSECTION, &combined));
    TEST_ASSERT_TRUE(pdraw_path_contains(&combined, 70, 30));
    TEST_ASSERT_FALSE(pdraw_path_contains(&combined, 30, 30));
    PDrawRect box = pdraw_rotated_bounds((PDrawRect){0, 0, 100, 60}, 90);
    TEST_ASSERT_FLOAT_WITHIN(.001f, 60, box.w);
    TEST_ASSERT_FLOAT_WITHIN(.001f, 100, box.h);
}

static void test_bezier_and_gradient_values(void) {
    PDrawBezier curve = {.count = 2, .nodes = {{{0, 0}, {0, 0}, {0, 1}}, {{1, 1}, {1, 0}, {1, 1}}}};
    PDrawPath path;
    TEST_ASSERT_TRUE(pdraw_bezier_flatten(&curve, .001f, &path));
    TEST_ASSERT_EQUAL_FLOAT(0, path.points[0].x);
    TEST_ASSERT_EQUAL_FLOAT(1, path.points[path.count - 1].x);
    TEST_ASSERT_TRUE(path.count > 2);
    PDrawGradient gradient = {.kind = PDRAW_GRADIENT_LINEAR,
                              .count = 2,
                              .x2 = 1,
                              .stops = {{0, 0xff0000ff}, {1, 0x0000ffff}}};
    TEST_ASSERT_TRUE(pdraw_gradient_valid(&gradient));
    TEST_ASSERT_EQUAL_HEX32(0xff0000ff, pdraw_gradient_sample(&gradient, 0, 0, 100, 100));
    TEST_ASSERT_EQUAL_HEX32(0x0000ffff, pdraw_gradient_sample(&gradient, 100, 0, 100, 100));
    TEST_ASSERT_FALSE(pdraw_shadow_valid(&(PDrawShadow){.blur = NAN}));
}

static float advance(void *context, unsigned code, unsigned next, float size) {
    (void)context;
    (void)code;
    (void)next;
    return size * .5f;
}

static void test_text_flow_without_fonts_or_ui(void) {
    PDrawText text = {.enabled = 1,
                      .columns = 2,
                      .size = 10,
                      .line_height = 1,
                      .column_gap = 10,
                      .content = "one two three four five six"};
    PDrawTextLayout layout;
    pdraw_text_layout(&text, 70, 30, advance, NULL, &layout);
    TEST_ASSERT_TRUE(layout.count > 0);
    bool second = false;
    for (int i = 0; i < layout.count; i++) {
        second |= layout.glyphs[i].x >= 40;
    }
    TEST_ASSERT_TRUE(second);
}

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_geometry_and_boolean_operations);
    RUN_TEST(test_text_flow_without_fonts_or_ui);
    RUN_TEST(test_bezier_and_gradient_values);
    return UNITY_END();
}
