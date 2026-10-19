#include "editor_host/host.h"
#include "editors/vector/geometry/constraints.h"
#include "editors/vector/geometry/resize.h"
#include "editors/vector/geometry/rotation.h"
#include "editors/vector/grid/render.h"
#include "editors/vector/grid/snap.h"
#include "editors/vector/internal.h"
#include "shell/shell.h"
#include "unity.h"

static SDL_Window *window;
static SDL_Renderer *renderer;
static EditorShell shell;
static EditorHost host;

static void commit(void) {
    char error[256];
    TEST_ASSERT_TRUE_MESSAGE(
        editor_host_commit(&host, shell.view, &shell.input, error, sizeof error), error);
    pui_view_draw(shell.view, &shell.input, (PUIRect){0, 0, 1280, 800}, NULL, NULL);
}

static VectorDocument *document(void) {
    VectorEditor *editor = host.instance;
    return vector_document_tabs_active(&editor->documents, shell.view);
}

static void action(const char *name) {
    TEST_ASSERT_TRUE(editor_host_action(&host, shell.view, name, ""));
    commit();
}

static void activate(const char *id) {
    TEST_ASSERT_TRUE(editor_host_request(&host, id, false));
    commit();
}

void setUp(void) {
    TEST_ASSERT_TRUE(SDL_Init(SDL_INIT_VIDEO));
    window = SDL_CreateWindow("grid", 1280, 800, SDL_WINDOW_HIDDEN);
    renderer = SDL_CreateRenderer(window, NULL);
    TEST_ASSERT_NOT_NULL(renderer);
    char path[1600], error[256];
    snprintf(path, sizeof path, "%s/shell.yaml", PENGUI_EDITOR_RESOURCES);
    TEST_ASSERT_TRUE_MESSAGE(editor_shell_load(&shell, renderer, path, error, sizeof error), error);
    editor_host_init(&host, (EditorServices){PENGUI_EDITOR_RESOURCES});
    activate("vector");
    commit();
    /* These feature fixtures build their own scene independently of startup content. */
    vector_history_destroy(&document()->history);
    vector_document_init(document(), document()->id);
}

void tearDown(void) {
    editor_shell_destroy(&shell);
    editor_host_destroy(&host);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
}

static void test_preferences_validate_isolate_and_survive_editor_unload(void) {
    VectorDocument *doc = document();
    TEST_ASSERT_NOT_NULL(doc);
    TEST_ASSERT_FALSE(doc->grid.visible);
    TEST_ASSERT_EQUAL_FLOAT(16, doc->grid.width);
    TEST_ASSERT_EQUAL_FLOAT(16, doc->grid.height);
    action("vector.grid.toggle");
    TEST_ASSERT_TRUE(doc->grid.visible);
    TEST_ASSERT_TRUE(pui_view_checked(shell.view, "vector_grid_toggle"));
    const char *invalid[] = {"0", "-2", "nan", "inf", "10001", "24px", "", "1e90"};
    for (unsigned i = 0; i < sizeof invalid / sizeof *invalid; i++) {
        pui_view_set_text(shell.view, "vector_grid_width", invalid[i]);
        action("vector.grid.width");
        TEST_ASSERT_EQUAL_FLOAT(16, doc->grid.width);
        TEST_ASSERT_EQUAL_STRING("16", pui_view_text(shell.view, "vector_grid_width"));
    }
    pui_view_set_text(shell.view, "vector_grid_width", "24");
    action("vector.grid.width");
    pui_view_set_text(shell.view, "vector_grid_height", "32");
    action("vector.grid.height");
    TEST_ASSERT_TRUE(pui_view_set_value(shell.view, "vector_grid_type", "hexagonal"));
    action("vector.grid.type");
    TEST_ASSERT_TRUE(pui_view_set_color(shell.view, "vector_grid_color", 0xAABBCC99));
    action("vector.grid.color");
    action("vector.grid.snap");
    TEST_ASSERT_TRUE(doc->grid.snap_enabled);
    TEST_ASSERT_EQUAL_FLOAT(5, doc->grid.snap_tolerance);
    pui_view_set_text(shell.view, "vector_grid_tolerance", "8");
    action("vector.grid.tolerance");
    pui_view_set_text(shell.view, "vector_grid_tolerance", "nan");
    action("vector.grid.tolerance");
    TEST_ASSERT_EQUAL_FLOAT(8, doc->grid.snap_tolerance);
    TEST_ASSERT_EQUAL_INT(0, doc->history.count);
    action("vector.add_document");
    TEST_ASSERT_FALSE(document()->grid.visible);
    TEST_ASSERT_EQUAL_FLOAT(16, document()->grid.width);
    activate("scripts");
    activate("vector");
    VectorEditor *editor = host.instance;
    doc = vector_documents_find(&editor->documents, 1);
    TEST_ASSERT_TRUE(doc->grid.visible);
    TEST_ASSERT_TRUE(doc->grid.snap_enabled);
    TEST_ASSERT_EQUAL_FLOAT(8, doc->grid.snap_tolerance);
    TEST_ASSERT_EQUAL_FLOAT(24, doc->grid.width);
    TEST_ASSERT_EQUAL_FLOAT(32, doc->grid.height);
    TEST_ASSERT_EQUAL_INT(VECTOR_GRID_HEXAGONAL, doc->grid.type);
    TEST_ASSERT_EQUAL_HEX32(0xAABBCC99, doc->grid.color);
    pui_pointer(&shell.input, 300, 108, true);
    pui_pointer(&shell.input, 300, 108, false);
    commit();
    commit();
    TEST_ASSERT_EQUAL_STRING("24", pui_view_text(shell.view, "vector_grid_width"));
    TEST_ASSERT_EQUAL_STRING("hexagonal", pui_view_value(shell.view, "vector_grid_type"));
    TEST_ASSERT_TRUE(pui_view_checked(shell.view, "vector_grid_toggle"));
    TEST_ASSERT_TRUE(pui_view_checked(shell.view, "vector_grid_snap"));
    TEST_ASSERT_EQUAL_STRING("8", pui_view_text(shell.view, "vector_grid_tolerance"));
}

static Uint32 pixel(SDL_Surface *surface, int x, int y) {
    Uint8 r, g, b, a;
    TEST_ASSERT_TRUE(SDL_ReadSurfacePixel(surface, x, y, &r, &g, &b, &a));
    return ((Uint32)r << 24) | ((Uint32)g << 16) | ((Uint32)b << 8) | a;
}

static void test_grids_clip_follow_zoom_and_bound_dense_rendering(void) {
    SDL_SetRenderClipRect(renderer, NULL);
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
    SDL_RenderClear(renderer);
    SDL_Rect clip = {12, 12, 140, 140};
    SDL_SetRenderClipRect(renderer, &clip);
    VectorGrid grid = {.visible = true, .width = 16, .height = 16, .color = 0xFF0000FF};
    vector_grid_draw(&grid, renderer, (SDL_FRect){20, 20, 100, 100}, 2);
    SDL_Rect restored;
    SDL_GetRenderClipRect(renderer, &restored);
    TEST_ASSERT_EQUAL_MEMORY(&clip, &restored, sizeof clip);
    SDL_Surface *surface = SDL_RenderReadPixels(renderer, NULL);
    TEST_ASSERT_NOT_NULL(surface);
    TEST_ASSERT_EQUAL_HEX32(0xFF0000FF, pixel(surface, 52, 30));
    TEST_ASSERT_EQUAL_HEX32(0x000000FF, pixel(surface, 36, 30));
    TEST_ASSERT_EQUAL_HEX32(0x000000FF, pixel(surface, 52, 121));
    TEST_ASSERT_EQUAL_HEX32(0x000000FF, pixel(surface, 52, 19));
    SDL_DestroySurface(surface);
    SDL_SetRenderClipRect(renderer, NULL);
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
    SDL_RenderClear(renderer);
    grid.type = VECTOR_GRID_HEXAGONAL;
    vector_grid_draw(&grid, renderer, (SDL_FRect){20, 20, 100, 100}, 1);
    surface = SDL_RenderReadPixels(renderer, NULL);
    TEST_ASSERT_NOT_NULL(surface);
    TEST_ASSERT_EQUAL_HEX32(0xFF0000FF, pixel(surface, 20, 28));
    TEST_ASSERT_EQUAL_HEX32(0x000000FF, pixel(surface, 28, 28));
    SDL_DestroySurface(surface);
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
    SDL_RenderClear(renderer);
    grid.type = VECTOR_GRID_ISOMETRIC;
    vector_grid_draw(&grid, renderer, (SDL_FRect){20, 20, 100, 100}, 1);
    surface = SDL_RenderReadPixels(renderer, NULL);
    TEST_ASSERT_NOT_NULL(surface);
    TEST_ASSERT_EQUAL_HEX32(0xFF0000FF, pixel(surface, 28, 28));
    TEST_ASSERT_EQUAL_HEX32(0x000000FF, pixel(surface, 36, 28));
    SDL_DestroySurface(surface);
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
    SDL_RenderClear(renderer);
    grid.width = grid.height = 1;
    vector_grid_draw(&grid, renderer, (SDL_FRect){-1000000, -1000000, 2000000, 2000000}, .1f);
    surface = SDL_RenderReadPixels(renderer, NULL);
    TEST_ASSERT_NOT_NULL(surface);
    TEST_ASSERT_EQUAL_HEX32(0x000000FF, pixel(surface, 20, 28));
    SDL_DestroySurface(surface);
}

static void test_snap_points_use_screen_radius_for_all_lattices(void) {
    VectorGrid grid = {.snap_enabled = true, .snap_tolerance = 5, .width = 16, .height = 16};
    float x = 13, y = 25;
    TEST_ASSERT_TRUE(vector_grid_snap_point(&grid, 1, &x, &y));
    TEST_ASSERT_EQUAL_FLOAT(16, x);
    TEST_ASSERT_EQUAL_FLOAT(25, y);
    x = 13;
    TEST_ASSERT_FALSE(vector_grid_snap_point(&grid, 2, &x, &y));
    grid.snap_enabled = false;
    TEST_ASSERT_FALSE(vector_grid_snap_point(&grid, 1, &x, &y));
    grid.snap_enabled = true;
    grid.type = VECTOR_GRID_ISOMETRIC;
    x = 9;
    y = 9;
    TEST_ASSERT_TRUE(vector_grid_snap_point(&grid, 1, &x, &y));
    TEST_ASSERT_EQUAL_FLOAT(8, x);
    TEST_ASSERT_EQUAL_FLOAT(8, y);
    x = -9;
    y = -9;
    TEST_ASSERT_TRUE(vector_grid_snap_point(&grid, 1, &x, &y));
    TEST_ASSERT_EQUAL_FLOAT(-8, x);
    TEST_ASSERT_EQUAL_FLOAT(-8, y);
    grid.type = VECTOR_GRID_HEXAGONAL;
    x = 9;
    y = 1;
    TEST_ASSERT_TRUE(vector_grid_snap_point(&grid, 1, &x, &y));
    TEST_ASSERT_EQUAL_FLOAT(8, x);
    TEST_ASSERT_EQUAL_FLOAT(0, y);
    x = 1;
    y = 5;
    TEST_ASSERT_TRUE(vector_grid_snap_point(&grid, 1, &x, &y));
    TEST_ASSERT_EQUAL_FLOAT(0, x);
    TEST_ASSERT_EQUAL_FLOAT(4, y);
}

static void test_snap_move_and_eight_handles_preserve_geometry_constraints(void) {
    VectorGrid grid = {.snap_enabled = true, .snap_tolerance = 5, .width = 16, .height = 16};
    PUIRect original = {16, 16, 32, 32};
    for (int i = 0; i < 8; i++) {
        unsigned edges = vector_resize_handle_edges(i);
        PUIRect rect = vector_grid_snap_rect(&grid, 1, original, edges, 14, 14, 1000);
        PUIRect expected = vector_resize_rect(original, edges, 16, 16, 1000);
        TEST_ASSERT_EQUAL_FLOAT(expected.x, rect.x);
        TEST_ASSERT_EQUAL_FLOAT(expected.y, rect.y);
        TEST_ASSERT_EQUAL_FLOAT(expected.w, rect.w);
        TEST_ASSERT_EQUAL_FLOAT(expected.h, rect.h);
    }
    PUIRect moved = vector_grid_snap_rect(&grid, 1, original, 0, 14, 14, 1000);
    TEST_ASSERT_EQUAL_FLOAT(32, moved.x);
    TEST_ASSERT_EQUAL_FLOAT(32, moved.y);
    TEST_ASSERT_EQUAL_FLOAT(original.w, moved.w);
    // An edge that is already aligned must win over another nearby edge.
    original.w = 30;
    moved = vector_grid_snap_rect(&grid, 1, original, 0, 16, 0, 1000);
    TEST_ASSERT_EQUAL_FLOAT(32, moved.x);
    original = (PUIRect){3, 3, 10, 10};
    moved = vector_grid_snap_rect(&grid, 1, original, 0, 0, 0, 1000);
    TEST_ASSERT_EQUAL_FLOAT(3, moved.x);
    TEST_ASSERT_EQUAL_FLOAT(3, moved.y);
    PUIRect tiny = vector_grid_snap_rect(&grid, 1, original, VECTOR_RESIZE_LEFT, 100, 0, 1000);
    TEST_ASSERT_TRUE(tiny.w >= 1);
    TEST_ASSERT_EQUAL_FLOAT(original.x + original.w, tiny.x + tiny.w);
}

static void test_modifier_geometry_all_handles_preserves_ratio_and_center(void) {
    PUIRect original = {40, 30, 100, 60};
    for (int i = 0; i < 8; i++) {
        unsigned edges = vector_resize_handle_edges(i);
        PUIRect target =
            vector_resize_constrained(original, edges, 20, 10, true, true, 1, 1, 1000, 1000);
        TEST_ASSERT_FLOAT_WITHIN(.001f, 100.f / 60, target.w / target.h);
        TEST_ASSERT_FLOAT_WITHIN(.001f, 90, target.x + target.w / 2);
        TEST_ASSERT_FLOAT_WITHIN(.001f, 60, target.y + target.h / 2);
        TEST_ASSERT_TRUE(target.w >= 1 && target.h >= 1);
    }
    PUIRect tiny = vector_resize_constrained(original, VECTOR_RESIZE_LEFT | VECTOR_RESIZE_TOP, 1000,
                                             1000, true, true, 1, 1, 1000, 1000);
    TEST_ASSERT_TRUE(tiny.w >= 1 && tiny.h >= 1);
    TEST_ASSERT_FLOAT_WITHIN(.001f, 100.f / 60, tiny.w / tiny.h);
    VectorGrid grid = {.snap_enabled = true, .snap_tolerance = 5, .width = 16, .height = 16};
    PUIRect snapped = vector_grid_resize_constrained(&grid, 1, original,
                                                     VECTOR_RESIZE_RIGHT | VECTOR_RESIZE_BOTTOM, 17,
                                                     11, true, true, 1, 1, 1000, 1000);
    TEST_ASSERT_FLOAT_WITHIN(.001f, 100.f / 60, snapped.w / snapped.h);
    TEST_ASSERT_FLOAT_WITHIN(.001f, 90, snapped.x + snapped.w / 2);
    TEST_ASSERT_FLOAT_WITHIN(.001f, 60, snapped.y + snapped.h / 2);
}

static void test_rotation_geometry_bounds_and_exact_intersections(void) {
    PUIRect original = {20, 20, 40, 20};
    PUIRect rotated = vector_rotated_bounds(original, 90);
    TEST_ASSERT_FLOAT_WITHIN(.001f, 30, rotated.x);
    TEST_ASSERT_FLOAT_WITHIN(.001f, 10, rotated.y);
    TEST_ASSERT_FLOAT_WITHIN(.001f, 20, rotated.w);
    TEST_ASSERT_FLOAT_WITHIN(.001f, 40, rotated.h);
    TEST_ASSERT_TRUE(vector_rotated_contains(original, 90, 40, 15));
    TEST_ASSERT_FALSE(vector_rotated_contains(original, 90, 22, 22));
    TEST_ASSERT_TRUE(vector_rotated_intersects(original, 90, (PUIRect){35, 12, 2, 2}));
    TEST_ASSERT_FALSE(vector_rotated_intersects(original, 90, (PUIRect){21, 21, 2, 2}));
    TEST_ASSERT_EQUAL_FLOAT(-180, vector_angle_normalize(180));
    TEST_ASSERT_EQUAL_FLOAT(90, vector_angle_normalize(450));
    TEST_ASSERT_EQUAL_FLOAT(270, vector_angle_turn(270));
    TEST_ASSERT_EQUAL_FLOAT(-270, vector_angle_turn(-270));
    TEST_ASSERT_EQUAL_FLOAT(360, vector_angle_turn(720));
    TEST_ASSERT_EQUAL_FLOAT(-360, vector_angle_turn(-360));
    VectorGrid grid = {.snap_enabled = true, .snap_tolerance = 5, .width = 16, .height = 16};
    PUIRect moved = vector_grid_snap_rotated_rect(&grid, 1, (PUIRect){30, 30, 40, 20}, 90, 5, 7);
    TEST_ASSERT_FLOAT_WITHIN(.001f, 34, moved.x);
    TEST_ASSERT_FLOAT_WITHIN(.001f, 34, moved.y);
}

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_rotation_geometry_bounds_and_exact_intersections);
    RUN_TEST(test_modifier_geometry_all_handles_preserves_ratio_and_center);
    RUN_TEST(test_snap_points_use_screen_radius_for_all_lattices);
    RUN_TEST(test_snap_move_and_eight_handles_preserve_geometry_constraints);
    RUN_TEST(test_preferences_validate_isolate_and_survive_editor_unload);
    RUN_TEST(test_grids_clip_follow_zoom_and_bound_dense_rendering);
    return UNITY_END();
}
