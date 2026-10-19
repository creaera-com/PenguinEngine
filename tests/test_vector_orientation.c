#include "editor_host/host.h"
#include "editors/vector/internal.h"
#include "editors/vector/object/clipboard.h"
#include "editors/vector/object/orientation.h"
#include "editors/vector/path/operations.h"
#include "shell/shell.h"
#include "unity.h"
#include <math.h>
#include <string.h>

static SDL_Window *window;
static SDL_Renderer *renderer;
static EditorShell shell;
static EditorHost host;

static void callback(void *context, const char *name, const char *widget) {
    (void)context;
    TEST_ASSERT_TRUE(editor_host_action(&host, shell.view, name, widget));
}

static void commit(void) {
    char error[256];
    TEST_ASSERT_TRUE_MESSAGE(
        editor_host_commit(&host, shell.view, &shell.input, error, sizeof error), error);
    pui_view_draw(shell.view, &shell.input, (PUIRect){0, 0, 1280, 800}, callback, NULL);
}

static void activate(const char *id) {
    TEST_ASSERT_TRUE(editor_host_request(&host, id, false));
    commit();
}

static VectorDocument *document(void) {
    return vector_document_tabs_active(&((VectorEditor *)host.instance)->documents, shell.view);
}

static void action(const char *name) {
    TEST_ASSERT_TRUE(editor_host_action(&host, shell.view, name, ""));
    commit();
}

void setUp(void) {
    TEST_ASSERT_TRUE(SDL_Init(SDL_INIT_VIDEO));
    SDL_SetModState(0);
    window = SDL_CreateWindow("orientation", 1280, 800, SDL_WINDOW_HIDDEN);
    renderer = SDL_CreateRenderer(window, NULL);
    TEST_ASSERT_NOT_NULL(renderer);
    char path[1600], error[256];
    snprintf(path, sizeof path, "%s/shell.yaml", PENGUI_EDITOR_RESOURCES);
    TEST_ASSERT_TRUE_MESSAGE(editor_shell_load(&shell, renderer, path, error, sizeof error), error);
    editor_host_init(&host, (EditorServices){PENGUI_EDITOR_RESOURCES});
    activate("vector");
    commit();
    TEST_ASSERT_TRUE(vector_artboard_add(document(), (PUIRect){10, 20, 500, 400}));
    commit();
}

void tearDown(void) {
    editor_shell_destroy(&shell);
    editor_host_destroy(&host);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
}

static unsigned hole(void) {
    VectorDocument *d = document();
    TEST_ASSERT_TRUE(vector_rectangle_add(d, 1, (PUIRect){30, 40, 160, 120}));
    TEST_ASSERT_TRUE(
        vector_rectangle_set_stroke(d, 1, (VectorStroke){0xc28e5cff, 6, VECTOR_STROKE_OUTSIDE}));
    TEST_ASSERT_TRUE(vector_rectangle_add(d, 1, (PUIRect){50, 60, 40, 30}));
    vector_document_select(d, 1);
    vector_document_toggle(d, 2);
    TEST_ASSERT_TRUE(vector_paths_operate(d, VECTOR_PATH_SUBTRACT));
    commit();
    return d->active_rectangle;
}

static void assert_shape_rect(PUIRect expected, PUIRect actual) {
    TEST_ASSERT_FLOAT_WITHIN(.0001f, expected.x, actual.x);
    TEST_ASSERT_FLOAT_WITHIN(.0001f, expected.y, actual.y);
    TEST_ASSERT_EQUAL_FLOAT(expected.w, actual.w);
    TEST_ASSERT_EQUAL_FLOAT(expected.h, actual.h);
}

static void test_single_quarter_turns_preserve_center_size_style_and_ids(void) {
    VectorDocument *d = document();
    TEST_ASSERT_TRUE(vector_rectangle_add(d, 1, (PUIRect){40.1f, 50.2f, 80.3f, 40.4f}));
    TEST_ASSERT_FALSE(vector_objects_can_orient(d, VECTOR_MIRROR_HORIZONTAL));
    TEST_ASSERT_FALSE(vector_objects_can_orient(d, VECTOR_MIRROR_VERTICAL));
    VectorShape original = d->rectangles[0];
    int count = d->history.count;
    TEST_ASSERT_TRUE(vector_objects_orient(d, VECTOR_ROTATE_LEFT));
    TEST_ASSERT_EQUAL_FLOAT(-90, d->rectangles[0].angle);
    assert_shape_rect(original.rect, d->rectangles[0].rect);
    TEST_ASSERT_EQUAL_UINT(original.id, d->rectangles[0].id);
    TEST_ASSERT_EQUAL_HEX32(original.fill, d->rectangles[0].fill);
    TEST_ASSERT_EQUAL_INT(count + 1, d->history.count);
    TEST_ASSERT_TRUE(vector_history_undo(&d->history, d));
    TEST_ASSERT_EQUAL_MEMORY(&original, &d->rectangles[0], sizeof original);
    for (int i = 0; i < 4; i++) {
        TEST_ASSERT_TRUE(vector_objects_orient(d, VECTOR_ROTATE_RIGHT));
    }
    TEST_ASSERT_EQUAL_FLOAT(360, d->rectangles[0].angle);
    assert_shape_rect(original.rect, d->rectangles[0].rect);
}

static void test_mirrors_preserve_positive_winding_and_move_offcenter_holes(void) {
    unsigned id = hole();
    VectorDocument *d = document();
    VectorShape original = *vector_rectangle_find(d, id);
    TEST_ASSERT_FALSE(vector_shape_contains(&original, 70, 75));
    TEST_ASSERT_TRUE(vector_objects_orient(d, VECTOR_MIRROR_HORIZONTAL));
    VectorShape *s = vector_rectangle_find(d, id);
    TEST_ASSERT_TRUE(vector_shape_contains(s, 70, 75));
    TEST_ASSERT_FALSE(vector_shape_contains(s, 150, 75));
    TEST_ASSERT_TRUE(
        vector_path_contour_area(&s->path, 0) * vector_path_contour_area(&original.path, 0) > 0);
    TEST_ASSERT_TRUE(
        vector_path_contour_area(&s->path, 1) * vector_path_contour_area(&original.path, 1) > 0);
    TEST_ASSERT_EQUAL_MEMORY(&original.stroke, &s->stroke, sizeof original.stroke);
    TEST_ASSERT_TRUE(vector_objects_orient(d, VECTOR_MIRROR_VERTICAL));
    TEST_ASSERT_FALSE(vector_shape_contains(s, 150, 125));
    TEST_ASSERT_TRUE(vector_shape_contains(s, 150, 75));
    TEST_ASSERT_TRUE(vector_history_undo(&d->history, d));
    TEST_ASSERT_TRUE(vector_history_undo(&d->history, d));
    TEST_ASSERT_EQUAL_MEMORY(&original, s, sizeof original);
}

static void test_reflection_of_rotated_paths_matches_world_points(void) {
    unsigned id = hole();
    VectorDocument *d = document();
    TEST_ASSERT_TRUE(vector_rectangle_set_angle(d, id, 30));
    VectorShape before = *vector_rectangle_find(d, id);
    VectorPath source = vector_shape_path(&before, before.rect, before.angle);
    float pivot = before.rect.x + before.rect.w / 2;
    TEST_ASSERT_TRUE(vector_objects_orient(d, VECTOR_MIRROR_HORIZONTAL));
    VectorShape *s = vector_rectangle_find(d, id);
    VectorPath after = vector_shape_path(s, s->rect, s->angle);
    TEST_ASSERT_EQUAL_FLOAT(-30, s->angle);
    for (unsigned i = 0; i < source.count; i++) {
        bool found = false;
        float x = 2 * pivot - source.points[i].x, y = source.points[i].y;
        for (unsigned j = 0; j < after.count; j++) {
            found |= fabsf(x - after.points[j].x) < .001f && fabsf(y - after.points[j].y) < .001f;
        }
        TEST_ASSERT_TRUE(found);
    }
}

static void test_group_turn_and_mirror_keep_original_parents_and_primary_selection(void) {
    VectorDocument *d = document();
    TEST_ASSERT_TRUE(vector_rectangle_add(d, 1, (PUIRect){40, 50, 80, 40}));
    TEST_ASSERT_TRUE(vector_artboard_add(d, (PUIRect){500, 20, 200, 200}));
    TEST_ASSERT_TRUE(vector_rectangle_add(d, 2, (PUIRect){20, 80, 40, 60}));
    vector_document_select(d, 2);
    vector_document_toggle(d, 1);
    VectorRectangleSelection selection = d->selection;
    PUIRect first = d->rectangles[0].rect, second = d->rectangles[1].rect;
    int count = d->history.count;
    TEST_ASSERT_TRUE(vector_objects_orient(d, VECTOR_ROTATE_RIGHT));
    assert_shape_rect((PUIRect){280, -140, 80, 40}, d->rectangles[0].rect);
    assert_shape_rect((PUIRect){-230, 300, 40, 60}, d->rectangles[1].rect);
    TEST_ASSERT_EQUAL_INT(count + 1, d->history.count);
    TEST_ASSERT_EQUAL_UINT(1, d->rectangles[0].parent_id);
    TEST_ASSERT_EQUAL_UINT(2, d->rectangles[1].parent_id);
    TEST_ASSERT_EQUAL_UINT(1, d->active_rectangle);
    TEST_ASSERT_EQUAL_MEMORY(&selection, &d->selection, sizeof selection);
    TEST_ASSERT_TRUE(vector_objects_orient(d, VECTOR_ROTATE_LEFT));
    assert_shape_rect(first, d->rectangles[0].rect);
    assert_shape_rect(second, d->rectangles[1].rect);
    TEST_ASSERT_TRUE(vector_objects_orient(d, VECTOR_MIRROR_HORIZONTAL));
    assert_shape_rect((PUIRect){470, 50, 80, 40}, d->rectangles[0].rect);
    assert_shape_rect((PUIRect){-450, 80, 40, 60}, d->rectangles[1].rect);
    TEST_ASSERT_TRUE(vector_artboard_set_state(d, 2, VECTOR_OBJECT_LOCKED));
    first = d->rectangles[0].rect;
    count = d->history.count;
    TEST_ASSERT_FALSE(vector_objects_orient(d, VECTOR_MIRROR_VERTICAL));
    TEST_ASSERT_EQUAL_MEMORY(&first, &d->rectangles[0].rect, sizeof first);
    TEST_ASSERT_EQUAL_INT(count, d->history.count);
}

static void test_orientation_checkpoint_undo_redo_and_clipboard_are_independent(void) {
    unsigned id = hole();
    VectorDocument *d = document();
    VectorShape original = *vector_rectangle_find(d, id);
    action("vector.objects.copy");
    action("vector.objects.mirror_horizontal");
    action("vector.objects.rotate_right");
    VectorShape result = *vector_rectangle_find(d, id);
    activate("scripts");
    activate("vector");
    d = document();
    TEST_ASSERT_EQUAL_MEMORY(&result, vector_rectangle_find(d, id), sizeof result);
    action("editor.undo");
    action("editor.undo");
    TEST_ASSERT_EQUAL_MEMORY(&original, vector_rectangle_find(d, id), sizeof original);
    action("editor.redo");
    action("editor.redo");
    TEST_ASSERT_EQUAL_MEMORY(&result, vector_rectangle_find(d, id), sizeof result);
    VectorClipboard *clipboard = &((VectorEditor *)host.instance)->object_actions.clipboard;
    TEST_ASSERT_EQUAL_MEMORY(&original.path, &clipboard->rectangles[0].path, sizeof original.path);
}

static void ui_click(int button, float x, float y) {
    SDL_Event e = {.type = SDL_EVENT_MOUSE_BUTTON_DOWN};
    e.button.button = button;
    e.button.x = x;
    e.button.y = y;
    if (!pui_view_event(shell.view, &shell.input, window, &e)) {
        editor_host_event(&host, shell.view, &e);
    }
    commit();
    e.type = SDL_EVENT_MOUSE_BUTTON_UP;
    if (!pui_view_event(shell.view, &shell.input, window, &e)) {
        editor_host_event(&host, shell.view, &e);
    }
    commit();
}

static void radial(const char *id) {
    float x, y;
    TEST_ASSERT_TRUE(pui_view_radial_item_center(shell.view, id, &x, &y));
    ui_click(SDL_BUTTON_LEFT, x, y);
}

static void test_native_transform_submenu_and_single_shape_inspector_angle(void) {
    unsigned id = hole();
    ui_click(SDL_BUTTON_RIGHT, 900, 600);
    radial("vector_transform_menu"); // Transformieren.
    radial("vector_rotate_right");   // Rechts 90 Grad.
    commit();
    TEST_ASSERT_EQUAL_FLOAT(90, vector_rectangle_find(document(), id)->angle);
    TEST_ASSERT_EQUAL_STRING("90", pui_view_text(shell.view, "rectangle_angle"));
}

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_single_quarter_turns_preserve_center_size_style_and_ids);
    RUN_TEST(test_mirrors_preserve_positive_winding_and_move_offcenter_holes);
    RUN_TEST(test_reflection_of_rotated_paths_matches_world_points);
    RUN_TEST(test_group_turn_and_mirror_keep_original_parents_and_primary_selection);
    RUN_TEST(test_orientation_checkpoint_undo_redo_and_clipboard_are_independent);
    RUN_TEST(test_native_transform_submenu_and_single_shape_inspector_angle);
    return UNITY_END();
}
