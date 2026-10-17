#include "editors/graphics/vector/document/document.h"
#include "editors/graphics/vector/viewport/grid.h"
#include "editors/graphics/vector/viewport/preview.h"
#include "unity.h"
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
static VeDocument doc, original;
static VeFlat flat;
static char error[256];

void setUp(void) {
    ve_init(&doc);
}

void tearDown(void) {
    remove("vector-test.svg");
    remove("vector-test.svg.tmp");
}

static int rect(float x, float y, float w, float h) {
    return ve_add_shape(&doc, 0, VE_RECT, x, y, w, h);
}

static float area(int index) {
    if (!doc.shapes[index].used) {
        return 0;
    }
    TEST_ASSERT_TRUE(ve_flatten(&doc.shapes[index], &flat));
    double sum = 0;
    for (int c = 0; c < flat.contours; c++) {
        int start = flat.start[c], n = flat.length[c];
        for (int i = 0; i < n; i++) {
            VePoint a = flat.points[start + i], b = flat.points[start + (i + 1) % n];
            sum += (double)a.x * b.y - (double)b.x * a.y;
        }
    }
    return (float)fabs(sum * .5);
}

static void test_artboard_local_positions_and_limits(void) {
    int shape = rect(10, 20, 40, 50);
    TEST_ASSERT_TRUE(ve_add_shape(&doc, 0, VE_PATH, 5, 6, 1, 1) >= 0);
    TEST_ASSERT_TRUE(ve_valid(&doc));
    int b = ve_add_board(&doc, 700, 0, 256, 256);
    TEST_ASSERT_TRUE(b >= 0);
    doc.boards[0].x = 99;
    TEST_ASSERT_EQUAL_FLOAT(10, doc.shapes[shape].x);
    TEST_ASSERT_TRUE(ve_valid(&doc));
    TEST_ASSERT_EQUAL_INT(-1, ve_add_board(&doc, 0, 0, -1, 10));
    for (int i = 2; i < VE_BOARDS; i++) {
        TEST_ASSERT_EQUAL_INT(i, ve_add_board(&doc, i * 100, 0, 64, 64));
    }
    TEST_ASSERT_EQUAL_INT(-1, ve_add_board(&doc, 0, 0, 32, 32));
}

static void test_primitive_conversion_and_curve_split(void) {
    int i = ve_add_shape(&doc, 0, VE_ELLIPSE, 10, 20, 64, 64);
    TEST_ASSERT_TRUE(ve_to_path(&doc.shapes[i]));
    TEST_ASSERT_EQUAL_INT(6, doc.shapes[i].count);
    TEST_ASSERT_TRUE(ve_flatten(&doc.shapes[i], &flat));
    TEST_ASSERT_TRUE(ve_inside(&flat, 42, 52));
    TEST_ASSERT_FALSE(ve_inside(&flat, 10, 20));
    TEST_ASSERT_FLOAT_WITHIN(20, 3.14159265f * 32 * 32, area(i));
    VeShape *s = &doc.shapes[i];
    s->count = 2;
    s->nodes[0] = (VeNode){.command = VE_MOVE, .p = {0, 0}};
    s->nodes[1] = (VeNode){.command = VE_CUBIC, .c1 = {0, 100}, .c2 = {100, 100}, .p = {100, 0}};
    TEST_ASSERT_TRUE(ve_split_node(s, 1));
    TEST_ASSERT_EQUAL_INT(3, s->count);
    TEST_ASSERT_EQUAL_FLOAT(50, s->nodes[1].p.x);
    TEST_ASSERT_EQUAL_FLOAT(75, s->nodes[1].p.y);
    TEST_ASSERT_EQUAL_FLOAT(100, s->nodes[2].p.x);
}

static void test_move_mirror_includes_control_points(void) {
    int i = ve_add_shape(&doc, 0, VE_ELLIPSE, 10, 20, 60, 40);
    VeShape *s = &doc.shapes[i];
    ve_to_path(s);
    float old = s->nodes[1].c1.x;
    ve_move(s, 7, -5);
    TEST_ASSERT_FLOAT_WITHIN(.001f, old + 7, s->nodes[1].c1.x);
    ve_mirror(s, true, 100);
    TEST_ASSERT_FLOAT_WITHIN(.001f, 200 - old - 7, s->nodes[1].c1.x);
    TEST_ASSERT_FLOAT_WITHIN(.001f, 123, s->x);
}

static void test_boolean_rectangles_and_shared_edges(void) {
    const float expected[] = {600, 200, 200, 400};
    for (int op = 0; op < 4; op++) {
        ve_init(&doc);
        int a = rect(0, 0, 20, 20), b = rect(10, 0, 20, 20);
        TEST_ASSERT_TRUE_MESSAGE(ve_boolean(&doc, a, b, (VeBoolean)op, error, sizeof error), error);
        TEST_ASSERT_FLOAT_WITHIN(.1f, expected[op], area(a));
        TEST_ASSERT_FALSE(doc.shapes[b].used);
        TEST_ASSERT_TRUE(ve_valid(&doc));
    }
}

static void test_boolean_nested_holes_and_identical_contours(void) {
    int a = rect(0, 0, 20, 20), b = rect(5, 5, 10, 10);
    TEST_ASSERT_TRUE_MESSAGE(ve_boolean(&doc, a, b, VE_DIFFERENCE, error, sizeof error), error);
    TEST_ASSERT_FLOAT_WITHIN(.1f, 300, area(a));
    TEST_ASSERT_TRUE(ve_flatten(&doc.shapes[a], &flat));
    TEST_ASSERT_FALSE(ve_inside(&flat, 10, 10));
    TEST_ASSERT_TRUE(ve_inside(&flat, 2, 2));
    for (int op = 0; op < 4; op++) {
        ve_init(&doc);
        a = rect(0, 0, 20, 20);
        b = rect(0, 0, 20, 20);
        TEST_ASSERT_TRUE_MESSAGE(ve_boolean(&doc, a, b, (VeBoolean)op, error, sizeof error), error);
        TEST_ASSERT_FLOAT_WITHIN(.1f, op < 2 ? 400 : 0, area(a));
    }
}

static void test_boolean_disjoint_and_touching(void) {
    int a = rect(0, 0, 20, 20), b = rect(40, 0, 20, 20);
    TEST_ASSERT_TRUE_MESSAGE(ve_boolean(&doc, a, b, VE_UNION, error, sizeof error), error);
    TEST_ASSERT_FLOAT_WITHIN(.1f, 800, area(a));
    ve_init(&doc);
    a = rect(0, 0, 20, 20);
    b = rect(20, 0, 20, 20);
    TEST_ASSERT_TRUE_MESSAGE(ve_boolean(&doc, a, b, VE_UNION, error, sizeof error), error);
    TEST_ASSERT_FLOAT_WITHIN(.1f, 800, area(a));
    ve_init(&doc);
    a = rect(0, 0, 20, 20);
    b = rect(20, 20, 20, 20);
    TEST_ASSERT_TRUE_MESSAGE(ve_boolean(&doc, a, b, VE_UNION, error, sizeof error), error);
    TEST_ASSERT_FLOAT_WITHIN(.1f, 800, area(a));
}

static void test_boolean_curves_and_atomic_wrong_board(void) {
    int a = ve_add_shape(&doc, 0, VE_ELLIPSE, 0, 0, 100, 100),
        b = ve_add_shape(&doc, 0, VE_ELLIPSE, 30, 0, 100, 100);
    TEST_ASSERT_TRUE_MESSAGE(ve_boolean(&doc, a, b, VE_DIFFERENCE, error, sizeof error), error);
    TEST_ASSERT_TRUE(ve_flatten(&doc.shapes[a], &flat));
    TEST_ASSERT_TRUE(ve_inside(&flat, 10, 50));
    TEST_ASSERT_FALSE(ve_inside(&flat, 60, 50));
    int board = ve_add_board(&doc, 200, 0, 100, 100);
    b = ve_add_shape(&doc, board, VE_RECT, 0, 0, 10, 10);
    original = doc;
    TEST_ASSERT_FALSE(ve_boolean(&doc, a, b, VE_UNION, error, sizeof error));
    TEST_ASSERT_EQUAL_MEMORY(&original, &doc, sizeof doc);
}

static void test_svg_metadata_roundtrip_and_atomic_invalid(void) {
    int a = rect(10, 20, 50, 60);
    strcpy(doc.shapes[a].name, "A < B & \"C\"");
    int b = ve_add_board(&doc, 700, 20, 128, 256);
    TEST_ASSERT_TRUE(b > 0);
    original = doc;
    TEST_ASSERT_TRUE_MESSAGE(ve_save(&doc, "vector-test.svg", error, sizeof error), error);
    ve_init(&doc);
    TEST_ASSERT_TRUE_MESSAGE(ve_load(&doc, "vector-test.svg", error, sizeof error), error);
    TEST_ASSERT_EQUAL_MEMORY(&original, &doc, sizeof doc);
    doc.boards[0].w = NAN;
    TEST_ASSERT_FALSE(ve_save(&doc, "vector-test.svg", error, sizeof error));
    TEST_ASSERT_TRUE(ve_load(&doc, "vector-test.svg", error, sizeof error));
    TEST_ASSERT_EQUAL_MEMORY(&original, &doc, sizeof doc);
}

static void test_external_svg_import_and_unsupported_is_atomic(void) {
    FILE *f = fopen("vector-test.svg", "wb");
    TEST_ASSERT_NOT_NULL(f);
    fputs("<svg xmlns=\"http://www.w3.org/2000/svg\" width=\"200\" height=\"100\"><g "
          "transform=\"translate(10 20)\"><rect width=\"40\" height=\"30\" "
          "fill=\"#123456\"/><circle cx=\"90\" cy=\"30\" r=\"10\"/></g></svg>",
          f);
    fclose(f);
    TEST_ASSERT_TRUE_MESSAGE(ve_load(&doc, "vector-test.svg", error, sizeof error), error);
    TEST_ASSERT_EQUAL_FLOAT(10, doc.shapes[0].x);
    TEST_ASSERT_EQUAL_FLOAT(20, doc.shapes[0].y);
    TEST_ASSERT_EQUAL_UINT32(0x123456ff, doc.shapes[0].fill);
    original = doc;
    f = fopen("vector-test.svg", "wb");
    fputs("<svg><text>unsupported</text></svg>", f);
    fclose(f);
    TEST_ASSERT_FALSE(ve_load(&doc, "vector-test.svg", error, sizeof error));
    TEST_ASSERT_EQUAL_MEMORY(&original, &doc, sizeof doc);
}

static void test_invalid_metadata_preserves_document(void) {
    rect(1, 2, 30, 40);
    original = doc;
    TEST_ASSERT_FALSE(ve_from_json(&doc, "{\"version\":1,\"next\":2,\"boards\":[],\"shapes\":[]}",
                                   error, sizeof error));
    TEST_ASSERT_EQUAL_MEMORY(&original, &doc, sizeof doc);
}

static void test_history_branching_saved_revision_and_eviction(void) {
    VeHistory h;
    TEST_ASSERT_TRUE(ve_history_init(&h, &doc));
    TEST_ASSERT_FALSE(ve_dirty(&h));
    rect(1, 2, 10, 20);
    TEST_ASSERT_TRUE(ve_history_commit(&h, &doc));
    h.saved = h.ids[h.position];
    TEST_ASSERT_FALSE(ve_dirty(&h));
    ve_move(&doc.shapes[0], 5, 6);
    TEST_ASSERT_TRUE(ve_history_commit(&h, &doc));
    TEST_ASSERT_TRUE(ve_dirty(&h));
    TEST_ASSERT_TRUE(ve_history_step(&h, &doc, false));
    TEST_ASSERT_FALSE(ve_dirty(&h));
    TEST_ASSERT_EQUAL_FLOAT(1, doc.shapes[0].x);
    ve_move(&doc.shapes[0], 8, 0);
    TEST_ASSERT_TRUE(ve_history_commit(&h, &doc));
    TEST_ASSERT_FALSE(ve_history_step(&h, &doc, true));
    for (int i = 0; i < VE_HISTORY + 10; i++) {
        ve_move(&doc.shapes[0], 1, 0);
        TEST_ASSERT_TRUE(ve_history_commit(&h, &doc));
    }
    TEST_ASSERT_EQUAL_INT(VE_HISTORY, h.count);
    TEST_ASSERT_TRUE(ve_history_step(&h, &doc, false));
    ve_history_free(&h);
}

static void test_nonzero_winding_import_is_preserved(void) {
    FILE *f = fopen("vector-test.svg", "wb");
    TEST_ASSERT_NOT_NULL(f);
    fputs("<svg width=\"100\" height=\"100\"><path fill-rule=\"nonzero\" d=\"M0 0 L40 0 L40 40 L0 "
          "40 Z M10 10 L30 10 L30 30 L10 30 Z\"/></svg>",
          f);
    fclose(f);
    TEST_ASSERT_TRUE_MESSAGE(ve_load(&doc, "vector-test.svg", error, sizeof error), error);
    TEST_ASSERT_FALSE(doc.shapes[0].evenodd);
    TEST_ASSERT_TRUE(ve_flatten(&doc.shapes[0], &flat));
    TEST_ASSERT_TRUE(ve_inside(&flat, 20, 20));
    TEST_ASSERT_TRUE(ve_save(&doc, "vector-test.svg", error, sizeof error));
    TEST_ASSERT_TRUE(ve_load(&doc, "vector-test.svg", error, sizeof error));
    TEST_ASSERT_FALSE(doc.shapes[0].evenodd);
    doc.shapes[0].evenodd = true;
    TEST_ASSERT_TRUE(ve_flatten(&doc.shapes[0], &flat));
    TEST_ASSERT_FALSE(ve_inside(&flat, 20, 20));
}

static void test_boolean_large_offsets(void) {
    int a = rect(100000, 100000, 20, 20), b = rect(100010, 100000, 20, 20);
    TEST_ASSERT_TRUE_MESSAGE(ve_boolean(&doc, a, b, VE_UNION, error, sizeof error), error);
    TEST_ASSERT_FLOAT_WITHIN(.1f, 600, area(a));
}

static void test_preview_small_selection_is_one_to_one(void) {
    VePreviewWindow result;
    TEST_ASSERT_TRUE(ve_preview_window((VeRegion){10, 20, 40, 30}, (VeRegion){100, 200, 200, 100},
                                       999, -999, &result));
    TEST_ASSERT_EQUAL_FLOAT(10, result.source.x);
    TEST_ASSERT_EQUAL_FLOAT(20, result.source.y);
    TEST_ASSERT_EQUAL_FLOAT(40, result.source.w);
    TEST_ASSERT_EQUAL_FLOAT(30, result.source.h);
    TEST_ASSERT_EQUAL_FLOAT(180, result.destination.x);
    TEST_ASSERT_EQUAL_FLOAT(235, result.destination.y);
    TEST_ASSERT_EQUAL_FLOAT(result.source.w, result.destination.w);
    TEST_ASSERT_EQUAL_FLOAT(result.source.h, result.destination.h);
}

static void test_preview_large_selection_follows_and_clamps(void) {
    VePreviewWindow result;
    VeRegion selection = {50, 60, 1000, 800}, viewport = {0, 0, 200, 100};
    TEST_ASSERT_TRUE(ve_preview_window(selection, viewport, 450, 310, &result));
    TEST_ASSERT_EQUAL_FLOAT(350, result.source.x);
    TEST_ASSERT_EQUAL_FLOAT(260, result.source.y);
    TEST_ASSERT_EQUAL_FLOAT(200, result.source.w);
    TEST_ASSERT_TRUE(ve_preview_window(selection, viewport, -400, -400, &result));
    TEST_ASSERT_EQUAL_FLOAT(50, result.source.x);
    TEST_ASSERT_EQUAL_FLOAT(60, result.source.y);
    TEST_ASSERT_TRUE(ve_preview_window(selection, viewport, 5000, 5000, &result));
    TEST_ASSERT_EQUAL_FLOAT(850, result.source.x);
    TEST_ASSERT_EQUAL_FLOAT(760, result.source.y);
    TEST_ASSERT_EQUAL_FLOAT(result.source.w, result.destination.w);
    TEST_ASSERT_EQUAL_FLOAT(result.source.h, result.destination.h);
}

static void test_preview_invalid_input_is_atomic(void) {
    VePreviewWindow result = {.source = {12, 34, 56, 78}};
    TEST_ASSERT_FALSE(
        ve_preview_window((VeRegion){0, 0, 0, 30}, (VeRegion){0, 0, 100, 100}, 0, 0, &result));
    TEST_ASSERT_FALSE(
        ve_preview_window((VeRegion){0, 0, 30, 30}, (VeRegion){0, 0, .5f, 100}, 0, 0, &result));
    TEST_ASSERT_FALSE(
        ve_preview_window((VeRegion){0, 0, 30, 30}, (VeRegion){0, 0, 100, 100}, NAN, 0, &result));
    TEST_ASSERT_EQUAL_FLOAT(12, result.source.x);
    TEST_ASSERT_EQUAL_FLOAT(78, result.source.h);
}

static void test_grid_standard_origin_and_pan(void) {
    VeGridLine lines[64];
    int n = ve_grid_lines((VeRegion){0, 0, 32, 32}, 16, 16, false, 1, lines, 64);
    TEST_ASSERT_EQUAL_INT(6, n);
    TEST_ASSERT_EQUAL_FLOAT(0, lines[0].a.x);
    TEST_ASSERT_EQUAL_FLOAT(16, lines[1].a.x);
    TEST_ASSERT_EQUAL_FLOAT(32, lines[2].a.x);
    n = ve_grid_lines((VeRegion){8, 8, 16, 16}, 16, 16, false, 1, lines, 64);
    TEST_ASSERT_EQUAL_INT(2, n);
    TEST_ASSERT_EQUAL_FLOAT(16, lines[0].a.x);
    TEST_ASSERT_EQUAL_FLOAT(8, lines[0].a.y);
    TEST_ASSERT_EQUAL_FLOAT(24, lines[0].b.y);
    TEST_ASSERT_EQUAL_FLOAT(16, lines[1].a.y);
}

static void test_grid_isometric_diamond_and_clip(void) {
    VeGridLine lines[64];
    int n = ve_grid_lines((VeRegion){0, 0, 32, 16}, 32, 16, true, 1, lines, 64);
    TEST_ASSERT_EQUAL_INT(2, n);
    for (int i = 0; i < n; i++) {
        TEST_ASSERT_FLOAT_WITHIN(.001f, 32, fabsf(lines[i].b.x - lines[i].a.x));
        TEST_ASSERT_FLOAT_WITHIN(.001f, 16, fabsf(lines[i].b.y - lines[i].a.y));
        TEST_ASSERT_TRUE(lines[i].a.y >= 0 && lines[i].a.y <= 16);
        TEST_ASSERT_TRUE(lines[i].b.y >= 0 && lines[i].b.y <= 16);
    }
    n = ve_grid_lines((VeRegion){10, 3, 9, 7}, 32, 16, true, 1, lines, 64);
    for (int i = 0; i < n; i++) {
        TEST_ASSERT_TRUE(lines[i].a.x >= 10 && lines[i].a.x <= 19);
        TEST_ASSERT_TRUE(lines[i].b.x >= 10 && lines[i].b.x <= 19);
        TEST_ASSERT_TRUE(lines[i].a.y >= 3 && lines[i].a.y <= 10);
        TEST_ASSERT_TRUE(lines[i].b.y >= 3 && lines[i].b.y <= 10);
    }
}

static void test_grid_density_and_invalid_inputs_are_bounded(void) {
    VeGridLine lines[64];
    int n = ve_grid_lines((VeRegion){0, 0, 100000, 100000}, 1, 1, true, .02f, lines, 64);
    TEST_ASSERT_TRUE(n > 0 && n <= 64);
    n = ve_grid_lines((VeRegion){0, 0, 100000, 100000}, 1, 100000, true, 32, lines, 64);
    TEST_ASSERT_TRUE(n >= 0 && n <= 64);
    lines[0].a.x = 123;
    TEST_ASSERT_EQUAL_INT(0, ve_grid_lines((VeRegion){0, 0, 32, 32}, 0, 16, false, 1, lines, 64));
    TEST_ASSERT_EQUAL_INT(0, ve_grid_lines((VeRegion){0, 0, 32, 32}, 16, NAN, true, 1, lines, 64));
    TEST_ASSERT_EQUAL_INT(0, ve_grid_lines((VeRegion){0, 0, 32, 32}, 16, 16, true, 0, lines, 64));
    TEST_ASSERT_EQUAL_FLOAT(123, lines[0].a.x);
}

static void test_grid_snap_standard_radius_and_invalid(void) {
    VePoint result = {123, 456};
    TEST_ASSERT_TRUE(ve_grid_snap((VePoint){13, 12}, 16, 16, false, 1, 5, &result));
    TEST_ASSERT_EQUAL_FLOAT(16, result.x);
    TEST_ASSERT_EQUAL_FLOAT(16, result.y);
    TEST_ASSERT_FALSE(ve_grid_snap((VePoint){12, 12}, 16, 16, false, 1, 5, &result));
    TEST_ASSERT_EQUAL_FLOAT(16, result.x);
    TEST_ASSERT_TRUE(ve_grid_snap((VePoint){-14, -17}, 16, 16, false, 1, 5, &result));
    TEST_ASSERT_EQUAL_FLOAT(-16, result.x);
    TEST_ASSERT_EQUAL_FLOAT(-16, result.y);
    TEST_ASSERT_FALSE(ve_grid_snap((VePoint){NAN, 0}, 16, 16, false, 1, 5, &result));
    TEST_ASSERT_FALSE(ve_grid_snap((VePoint){0, 0}, 0, 16, false, 1, 5, &result));
    TEST_ASSERT_FALSE(ve_grid_snap((VePoint){0, 0}, 16, 16, false, 0, 5, &result));
    TEST_ASSERT_FALSE(ve_grid_snap((VePoint){0, 0}, 16, 16, false, 1, -1, &result));
    TEST_ASSERT_FALSE(ve_grid_snap((VePoint){0, 0}, 16, 16, false, 1, 5, NULL));
    TEST_ASSERT_EQUAL_FLOAT(-16, result.x);
}

static void test_grid_snap_isometric_nearest_intersection(void) {
    VePoint result;
    TEST_ASSERT_TRUE(ve_grid_snap((VePoint){14, 9}, 32, 16, true, 1, 5, &result));
    TEST_ASSERT_EQUAL_FLOAT(16, result.x);
    TEST_ASSERT_EQUAL_FLOAT(8, result.y);
    /* Highly anisotropic diamonds: rounding transformed axes is not nearest. */
    TEST_ASSERT_TRUE(ve_grid_snap((VePoint){1, 40}, 100, 10000, true, 1, 50, &result));
    TEST_ASSERT_EQUAL_FLOAT(0, result.x);
    TEST_ASSERT_EQUAL_FLOAT(0, result.y);
    TEST_ASSERT_TRUE(ve_grid_snap((VePoint){-17, -9}, 32, 16, true, 1, 5, &result));
    TEST_ASSERT_EQUAL_FLOAT(-16, result.x);
    TEST_ASSERT_EQUAL_FLOAT(-8, result.y);
}

static void test_grid_snap_screen_tolerance_and_dense_grid(void) {
    VePoint result = {0, 0};
    TEST_ASSERT_TRUE(ve_grid_snap((VePoint){20, 16}, 16, 16, false, .5f, 5, &result));
    TEST_ASSERT_FALSE(ve_grid_snap((VePoint){20, 16}, 16, 16, false, 2, 5, &result));
    TEST_ASSERT_TRUE(ve_grid_snap((VePoint){3.2f, 7.1f}, 1, 1, false, .02f, 5, &result));
    TEST_ASSERT_EQUAL_FLOAT(3, result.x);
    TEST_ASSERT_EQUAL_FLOAT(7, result.y);
    TEST_ASSERT_TRUE(ve_grid_snap((VePoint){16, 16}, 16, 16, false, 32, 0, &result));
    TEST_ASSERT_FALSE(ve_grid_snap((VePoint){16.01f, 16}, 16, 16, false, 1, 0, &result));
}

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_grid_snap_standard_radius_and_invalid);
    RUN_TEST(test_grid_snap_isometric_nearest_intersection);
    RUN_TEST(test_grid_snap_screen_tolerance_and_dense_grid);
    RUN_TEST(test_grid_standard_origin_and_pan);
    RUN_TEST(test_grid_isometric_diamond_and_clip);
    RUN_TEST(test_grid_density_and_invalid_inputs_are_bounded);
    RUN_TEST(test_preview_small_selection_is_one_to_one);
    RUN_TEST(test_preview_large_selection_follows_and_clamps);
    RUN_TEST(test_preview_invalid_input_is_atomic);
    RUN_TEST(test_nonzero_winding_import_is_preserved);
    RUN_TEST(test_boolean_large_offsets);
    RUN_TEST(test_artboard_local_positions_and_limits);
    RUN_TEST(test_primitive_conversion_and_curve_split);
    RUN_TEST(test_move_mirror_includes_control_points);
    RUN_TEST(test_boolean_rectangles_and_shared_edges);
    RUN_TEST(test_boolean_nested_holes_and_identical_contours);
    RUN_TEST(test_boolean_disjoint_and_touching);
    RUN_TEST(test_boolean_curves_and_atomic_wrong_board);
    RUN_TEST(test_svg_metadata_roundtrip_and_atomic_invalid);
    RUN_TEST(test_external_svg_import_and_unsupported_is_atomic);
    RUN_TEST(test_invalid_metadata_preserves_document);
    RUN_TEST(test_history_branching_saved_revision_and_eviction);
    return UNITY_END();
}
