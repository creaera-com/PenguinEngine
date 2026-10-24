#include "editor_host/host.h"
#include "editors/vector/internal.h"
#include "editors/vector/object/clipboard.h"
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

static VectorDocument *document(void) {
    return vector_document_tabs_active(&((VectorEditor *)host.instance)->documents, shell.view);
}

static void activate(const char *id) {
    TEST_ASSERT_TRUE(editor_host_request(&host, id, false));
    commit();
}

static void action(const char *name) {
    TEST_ASSERT_TRUE(editor_host_action(&host, shell.view, name, ""));
    commit();
}

void setUp(void) {
    TEST_ASSERT_TRUE(SDL_Init(SDL_INIT_VIDEO));
    SDL_SetModState(0);
    window = SDL_CreateWindow("paths", 1280, 800, SDL_WINDOW_HIDDEN);
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

    TEST_ASSERT_TRUE(vector_artboard_add(document(), (PUIRect){0, 0, 450, 340}));
    commit();
}

void tearDown(void) {
    editor_shell_destroy(&shell);
    editor_host_destroy(&host);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
}

static VectorPath box(float x, float y, float w, float h) {
    return (VectorPath){.count = 4,
                        .contours = 1,
                        .points = {{x, y}, {x + w, y}, {x + w, y + h}, {x, y + h}},
                        .ends = {4}};
}

static float area(const VectorPath *p) {
    float result = 0;
    for (unsigned c = 0; c < p->contours; c++) {
        result += vector_path_contour_area(p, c);
    }
    return result;
}

static void pair(void) {
    VectorDocument *d = document();
    TEST_ASSERT_TRUE(vector_rectangle_add(d, 1, (PUIRect){20, 30, 100, 100}));
    TEST_ASSERT_TRUE(vector_rectangle_add(d, 1, (PUIRect){70, 50, 100, 100}));
    vector_document_select(d, 1);
    vector_document_toggle(d, 2);
    commit();
}

static Uint8 red(float x, float y) {
    VectorViewport *v = &document()->viewport;
    SDL_Rect pixel = {(int)(v->bounds.x + v->x + x * v->zoom),
                      (int)(v->bounds.y + v->y + y * v->zoom), 1, 1};
    SDL_Surface *surface = SDL_RenderReadPixels(renderer, &pixel);
    TEST_ASSERT_NOT_NULL(surface);
    Uint8 r, g, b, a;
    TEST_ASSERT_TRUE(SDL_ReadSurfacePixel(surface, 0, 0, &r, &g, &b, &a));
    SDL_DestroySurface(surface);
    return r;
}

static void pointer(SDL_EventType type, float x, float y) {
    VectorViewport *v = &document()->viewport;
    SDL_Event e = {.type = type};
    e.button.button = SDL_BUTTON_LEFT;
    e.button.x = v->bounds.x + v->x + x * v->zoom;
    e.button.y = v->bounds.y + v->y + y * v->zoom;
    if (!pui_view_event(shell.view, &shell.input, window, &e)) {
        editor_host_event(&host, shell.view, &e);
    }
    commit();
}

static void test_boolean_geometry_area_winding_holes_and_touching_edges(void) {
    VectorPath a = box(0, 0, 100, 100), b = box(50, 20, 100, 100), result;
    float expected[] = {16000, 6000, 4000, 12000};
    for (int op = 0; op < 4; op++) {
        TEST_ASSERT_TRUE(vector_path_combine(&a, &b, (VectorAreaOperation)op, &result));
        TEST_ASSERT_FLOAT_WITHIN(.01f, expected[op], area(&result));
        TEST_ASSERT_EQUAL_INT(op == VECTOR_AREA_UNION || op == VECTOR_AREA_INTERSECTION,
                              vector_path_contains(&result, 75, 50));
    }
    b = box(20, 20, 60, 60);
    TEST_ASSERT_TRUE(vector_path_combine(&a, &b, VECTOR_AREA_DIFFERENCE, &result));
    TEST_ASSERT_EQUAL_UINT(2, result.contours);
    TEST_ASSERT_FLOAT_WITHIN(.01f, 6400, area(&result));
    TEST_ASSERT_FALSE(vector_path_contains(&result, 50, 50));
    TEST_ASSERT_TRUE(vector_path_contains(&result, 10, 50));
    TEST_ASSERT_FALSE(vector_path_intersects(&result, (PUIRect){40, 40, 10, 10}));
    TEST_ASSERT_TRUE(vector_path_intersects(&result, (PUIRect){15, 40, 10, 10}));
    b = box(100, 0, 100, 100);
    TEST_ASSERT_TRUE(vector_path_combine(&a, &b, VECTOR_AREA_INTERSECTION, &result));
    TEST_ASSERT_EQUAL_UINT(0, result.count);
}

static void test_outline_alignment_and_hole_sides(void) {
    VectorPath source = box(0, 0, 100, 100), outline;
    float expected[] = {3600, 4000, 4400};
    for (int placement = -1; placement <= 1; placement++) {
        TEST_ASSERT_TRUE(vector_path_outline(&source, 10, placement, &outline));
        TEST_ASSERT_FLOAT_WITHIN(.1f, expected[placement + 1], area(&outline));
        TEST_ASSERT_FALSE(vector_path_contains(&outline, 50, 50));
    }
    VectorPath acute = {
        .count = 3, .contours = 1, .points = {{0, 0}, {100, 0}, {1, 1}}, .ends = {3}};
    TEST_ASSERT_TRUE(vector_path_outline(&acute, 10, 0, &outline));
    TEST_ASSERT_TRUE(vector_path_contains(&outline, 99, 0));
    TEST_ASSERT_TRUE(area(&outline) > 0);
    VectorPath hole = box(40, 40, 20, 20);
    TEST_ASSERT_TRUE(vector_path_combine(&source, &hole, VECTOR_AREA_DIFFERENCE, &source));
    TEST_ASSERT_TRUE(vector_path_outline(&source, 5, -1, &outline));
    TEST_ASSERT_TRUE(vector_path_contains(&outline, 38, 50));
    TEST_ASSERT_FALSE(vector_path_contains(&outline, 42, 50));
    TEST_ASSERT_TRUE(vector_path_outline(&source, 5, 1, &outline));
    TEST_ASSERT_FALSE(vector_path_contains(&outline, 38, 50));
    TEST_ASSERT_TRUE(vector_path_contains(&outline, 42, 50));
}

static void test_conversion_preserves_geometry_styles_order_and_pixel_fill(void) {
    pair();
    VectorDocument *d = document();
    TEST_ASSERT_TRUE(vector_rectangle_set_fill(d, 1, 0xff0000ff));
    TEST_ASSERT_TRUE(
        vector_rectangle_set_stroke(d, 1, (VectorStroke){0x0000ffff, 10, VECTOR_STROKE_OUTSIDE}));
    TEST_ASSERT_TRUE(vector_rectangle_add(d, 1, (PUIRect){220, 30, 40, 40}));
    vector_document_select(d, 1);
    vector_document_toggle(d, 3);
    commit();
    int count = d->history.count;
    vector_document_select(d, 3);
    vector_document_toggle(d, 1);
    unsigned primary = d->active_rectangle;
    VectorRectangleSelection selection = d->selection;
    TEST_ASSERT_EQUAL_UINT8(255, red(40, 60));
    TEST_ASSERT_EQUAL_UINT8(0, red(15, 60));
    action("vector.objects.path_convert");
    TEST_ASSERT_EQUAL_INT(count + 1, d->history.count);
    TEST_ASSERT_EQUAL_UINT(primary, d->active_rectangle);
    TEST_ASSERT_EQUAL_MEMORY(&selection, &d->selection, sizeof selection);
    for (int i = 0; i < 3; i++) {
        TEST_ASSERT_EQUAL_UINT(i + 1, d->rectangles[i].id);
    }
    TEST_ASSERT_EQUAL_UINT(4, d->rectangles[0].path.count);
    TEST_ASSERT_EQUAL_UINT(0, d->rectangles[1].path.count);
    TEST_ASSERT_EQUAL_UINT(4, d->rectangles[2].path.count);
    TEST_ASSERT_EQUAL_UINT8(255, red(40, 60));
    TEST_ASSERT_EQUAL_UINT8(0, red(15, 60));
    TEST_ASSERT_EQUAL_HEX32(0x0000ffff, d->rectangles[0].stroke.color);
    TEST_ASSERT_FALSE(vector_paths_can_operate(d, VECTOR_PATH_CONVERT));
    action("editor.undo");
    TEST_ASSERT_EQUAL_UINT(0, d->rectangles[0].path.count);
    action("editor.redo");
    TEST_ASSERT_EQUAL_UINT(4, d->rectangles[0].path.count);
    TEST_ASSERT_TRUE(vector_rectangle_set_angle(d, 1, 30));
    TEST_ASSERT_TRUE(vector_rectangle_update(d, 1, "Scaled path", (PUIRect){20, 30, 150, 100}));
    TEST_ASSERT_TRUE(vector_shape_contains(&d->rectangles[0], 80, 70));
}

static void test_operations_are_atomic_and_division_produces_independent_pieces(void) {
    pair();
    VectorDocument *d = document();
    VectorShape before[2];
    memcpy(before, d->rectangles, sizeof before);
    float expected[] = {16000, 6000, 4000, 12000, 16000};
    for (int op = VECTOR_PATH_UNION; op <= VECTOR_PATH_DIVIDE; op++) {
        int cursor = d->history.cursor;
        TEST_ASSERT_TRUE(vector_paths_operate(d, (VectorPathOperation)op));
        TEST_ASSERT_EQUAL_INT(op == VECTOR_PATH_DIVIDE ? 3 : 1, d->rectangle_count);
        TEST_ASSERT_EQUAL_INT(cursor + 1, d->history.cursor);
        TEST_ASSERT_EQUAL_INT(d->rectangle_count, d->selection.count);
        float total = 0;
        for (int i = 0; i < d->rectangle_count; i++) {
            VectorShape *s = &d->rectangles[i];
            TEST_ASSERT_TRUE(s->path.count > 0);
            VectorPath path = vector_shape_path(s, s->rect, s->angle);
            total += area(&path);
            TEST_ASSERT_EQUAL_UINT(1, s->parent_id);
            if (op == VECTOR_PATH_DIVIDE) {
                for (int j = 0; j < i; j++) {
                    VectorShape *other = &d->rectangles[j];
                    VectorPath b = vector_shape_path(other, other->rect, other->angle),
                               intersection;
                    TEST_ASSERT_TRUE(
                        vector_path_combine(&path, &b, VECTOR_AREA_INTERSECTION, &intersection));
                    TEST_ASSERT_FLOAT_WITHIN(.01f, 0, area(&intersection));
                }
            }
        }
        TEST_ASSERT_FLOAT_WITHIN(.1f, expected[op - VECTOR_PATH_UNION], total);
        TEST_ASSERT_TRUE(vector_history_undo(&d->history, d));
        TEST_ASSERT_EQUAL_MEMORY(before, d->rectangles, sizeof before);
    }
}

static void test_hole_rendering_hit_testing_and_clipboard_survive_editor_switch(void) {
    VectorDocument *d = document();
    TEST_ASSERT_TRUE(vector_rectangle_add(d, 1, (PUIRect){20, 20, 160, 160}));
    TEST_ASSERT_TRUE(vector_rectangle_add(d, 1, (PUIRect){60, 60, 80, 80}));
    TEST_ASSERT_TRUE(vector_rectangle_set_fill(d, 1, 0xff000080));
    vector_document_select(d, 1);
    vector_document_toggle(d, 2);
    TEST_ASSERT_TRUE(vector_paths_operate(d, VECTOR_PATH_SUBTRACT));
    commit();
    VectorShape original = d->rectangles[0];
    TEST_ASSERT_UINT8_WITHIN(1, 249, red(40, 90));
    TEST_ASSERT_EQUAL_UINT8(242, red(90, 90));
    TEST_ASSERT_FALSE(vector_shape_contains(&original, 90, 90));
    TEST_ASSERT_TRUE(
        vector_objects_copy(d, &((VectorEditor *)host.instance)->object_actions.clipboard));
    action("vector.add_document");
    action("vector.objects.paste");
    d = document();
    TEST_ASSERT_EQUAL_UINT(2, d->id);
    TEST_ASSERT_EQUAL_INT(1, d->rectangle_count);
    TEST_ASSERT_EQUAL_MEMORY(&original.path, &d->rectangles[0].path, sizeof original.path);
    activate("scripts");
    activate("vector");
    d = document();
    TEST_ASSERT_EQUAL_MEMORY(&original.path, &d->rectangles[0].path, sizeof original.path);
    action("editor.undo");
    TEST_ASSERT_EQUAL_INT(0, d->rectangle_count);
    action("editor.redo");
    TEST_ASSERT_EQUAL_INT(1, d->rectangle_count);
}

static void test_empty_result_locks_parent_guards_and_complexity_limits(void) {
    pair();
    VectorDocument *d = document();
    TEST_ASSERT_TRUE(vector_rectangle_update(d, 2, "Far", (PUIRect){300, 30, 40, 40}));
    int count = d->history.count;
    TEST_ASSERT_TRUE(vector_paths_operate(d, VECTOR_PATH_INTERSECT));
    TEST_ASSERT_EQUAL_INT(0, d->rectangle_count);
    TEST_ASSERT_EQUAL_INT(count + 1, d->history.count);
    TEST_ASSERT_TRUE(vector_history_undo(&d->history, d));
    TEST_ASSERT_TRUE(vector_rectangle_set_state(d, 2, VECTOR_OBJECT_LOCKED));
    count = d->history.count;
    TEST_ASSERT_FALSE(vector_paths_operate(d, VECTOR_PATH_UNION));
    TEST_ASSERT_EQUAL_INT(2, d->rectangle_count);
    TEST_ASSERT_EQUAL_INT(count, d->history.count);
    TEST_ASSERT_TRUE(vector_rectangle_set_state(d, 2, 0));
    for (int i = 0; i < 34; i++) {
        TEST_ASSERT_TRUE(vector_rectangle_add(d, 1, (PUIRect){(float)(i * 10), 250, 3, 3}));
    }
    vector_document_select(d, 1);
    for (int i = 1; i < d->rectangle_count; i++) {
        vector_document_toggle(d, d->rectangles[i].id);
    }
    count = d->history.count;
    TEST_ASSERT_FALSE(vector_paths_operate(d, VECTOR_PATH_UNION));
    TEST_ASSERT_EQUAL_INT(36, d->rectangle_count);
    TEST_ASSERT_EQUAL_INT(count, d->history.count);
}

static void test_stroke_to_path_preserves_band_appearance_and_is_undoable(void) {
    VectorDocument *d = document();
    TEST_ASSERT_TRUE(vector_rectangle_add(d, 1, (PUIRect){30, 30, 100, 100}));
    TEST_ASSERT_TRUE(
        vector_rectangle_set_stroke(d, 1, (VectorStroke){0xff0000ff, 10, VECTOR_STROKE_CENTER}));
    commit();
    action("vector.objects.path_stroke");
    TEST_ASSERT_TRUE(d->rectangles[0].path.count > 0);
    TEST_ASSERT_EQUAL_FLOAT(0, d->rectangles[0].stroke.width);
    TEST_ASSERT_EQUAL_HEX32(0xff0000ff, d->rectangles[0].fill);
    TEST_ASSERT_EQUAL_UINT8(255, red(31, 75));
    TEST_ASSERT_EQUAL_UINT8(242, red(70, 75));
    action("editor.undo");
    TEST_ASSERT_EQUAL_FLOAT(10, d->rectangles[0].stroke.width);
    TEST_ASSERT_EQUAL_UINT(0, d->rectangles[0].path.count);
}

static void test_path_hole_click_selects_the_visible_object_behind_it(void) {
    VectorDocument *d = document();
    TEST_ASSERT_TRUE(vector_rectangle_add(d, 1, (PUIRect){0, 0, 250, 250}));
    TEST_ASSERT_TRUE(vector_rectangle_set_fill(d, 1, 0x0000ffff));
    TEST_ASSERT_TRUE(vector_rectangle_add(d, 1, (PUIRect){20, 20, 160, 160}));
    TEST_ASSERT_TRUE(vector_rectangle_set_fill(d, 2, 0xff0000ff));
    TEST_ASSERT_TRUE(vector_rectangle_add(d, 1, (PUIRect){60, 60, 80, 80}));
    vector_document_select(d, 2);
    vector_document_toggle(d, 3);
    TEST_ASSERT_TRUE(vector_paths_operate(d, VECTOR_PATH_SUBTRACT));
    vector_document_select(d, 0);
    d->active_artboard = 0;
    commit();
    TEST_ASSERT_EQUAL_UINT8(0, red(90, 90));
    TEST_ASSERT_EQUAL_UINT8(255, red(40, 90));
    pointer(SDL_EVENT_MOUSE_BUTTON_DOWN, 90, 90);
    pointer(SDL_EVENT_MOUSE_BUTTON_UP, 90, 90);
    TEST_ASSERT_EQUAL_UINT(1, d->active_rectangle);
}

static void test_all_stroke_placements_preserve_alpha_when_converting_to_path(void) {
    VectorDocument *d = document();
    TEST_ASSERT_TRUE(vector_rectangle_add(d, 1, (PUIRect){30, 30, 100, 100}));
    TEST_ASSERT_TRUE(vector_rectangle_set_fill(d, 1, 0xff0000ff));
    for (int placement = VECTOR_STROKE_CENTER; placement <= VECTOR_STROKE_OUTSIDE; placement++) {
        TEST_ASSERT_TRUE(vector_rectangle_set_stroke(
            d, 1, (VectorStroke){0x0000ff80, 10, (VectorStrokeAlignment)placement}));
        commit();
        Uint8 before[3] = {red(27, 75), red(32, 75), red(45, 75)};
        TEST_ASSERT_TRUE(vector_paths_operate(d, VECTOR_PATH_CONVERT));
        commit();
        TEST_ASSERT_UINT8_WITHIN(1, before[0], red(27, 75));
        TEST_ASSERT_UINT8_WITHIN(1, before[1], red(32, 75));
        TEST_ASSERT_UINT8_WITHIN(1, before[2], red(45, 75));
        TEST_ASSERT_TRUE(vector_history_undo(&d->history, d));
    }
}

static void test_three_operand_division_and_disconnected_components(void) {
    pair();
    VectorDocument *d = document();
    TEST_ASSERT_TRUE(vector_rectangle_add(d, 1, (PUIRect){50, 70, 160, 40}));
    vector_document_select(d, 1);
    vector_document_toggle(d, 2);
    vector_document_toggle(d, 3);
    TEST_ASSERT_TRUE(vector_paths_operate(d, VECTOR_PATH_UNION));
    VectorShape united = d->rectangles[0];
    VectorPath coverage = vector_shape_path(&united, united.rect, united.angle);
    float expected = area(&coverage);
    TEST_ASSERT_TRUE(vector_history_undo(&d->history, d));
    TEST_ASSERT_TRUE(vector_paths_operate(d, VECTOR_PATH_DIVIDE));
    float total = 0;
    for (int i = 0; i < d->rectangle_count; i++) {
        VectorShape *s = &d->rectangles[i];
        VectorPath a = vector_shape_path(s, s->rect, s->angle);
        total += area(&a);
        for (int j = 0; j < i; j++) {
            VectorShape *other = &d->rectangles[j];
            VectorPath b = vector_shape_path(other, other->rect, other->angle), overlap;
            TEST_ASSERT_TRUE(vector_path_combine(&a, &b, VECTOR_AREA_INTERSECTION, &overlap));
            TEST_ASSERT_FLOAT_WITHIN(.01f, 0, area(&overlap));
        }
    }
    TEST_ASSERT_FLOAT_WITHIN(.1f, expected, total);
    TEST_ASSERT_TRUE(vector_history_undo(&d->history, d));
    TEST_ASSERT_TRUE(vector_rectangle_update(d, 2, "Separate", (PUIRect){300, 50, 40, 40}));
    vector_document_select(d, 1);
    vector_document_toggle(d, 2);
    TEST_ASSERT_TRUE(vector_paths_operate(d, VECTOR_PATH_UNION));
    TEST_ASSERT_EQUAL_INT(2, vector_rectangle_find(d, d->active_rectangle)->path.contours);
}

static void test_native_radial_path_conversion_and_rotated_booleans(void) {
    pair();
    VectorDocument *d = document();
    SDL_Event e = {.type = SDL_EVENT_MOUSE_BUTTON_DOWN};
    e.button.button = SDL_BUTTON_RIGHT;
    e.button.x = 630;
    e.button.y = 400;
    if (!pui_view_event(shell.view, &shell.input, window, &e)) {
        editor_host_event(&host, shell.view, &e);
    }
    commit();
    e.type = SDL_EVENT_MOUSE_BUTTON_UP;
    pui_view_event(shell.view, &shell.input, window, &e);
    commit();
    const char *items[] = {"vector_paths_menu", "vector_path_convert"};
    for (int i = 0; i < 2; i++) {
        e.type = SDL_EVENT_MOUSE_BUTTON_DOWN;
        e.button.button = SDL_BUTTON_LEFT;
        TEST_ASSERT_TRUE(
            pui_view_radial_item_center(shell.view, items[i], &e.button.x, &e.button.y));
        TEST_ASSERT_TRUE(pui_view_event(shell.view, &shell.input, window, &e));
        commit();
        e.type = SDL_EVENT_MOUSE_BUTTON_UP;
        TEST_ASSERT_TRUE(pui_view_event(shell.view, &shell.input, window, &e));
        commit();
    }
    TEST_ASSERT_EQUAL_UINT(4, d->rectangles[0].path.count);
    TEST_ASSERT_EQUAL_UINT(4, d->rectangles[1].path.count);
    vector_document_select(d, 1);
    commit();
    TEST_ASSERT_EQUAL_STRING("Pfad", pui_view_text(shell.view, "rectangle_kind"));
    TEST_ASSERT_TRUE(vector_rectangle_set_angle(d, 1, 30));
    vector_document_toggle(d, 2);
    TEST_ASSERT_TRUE(vector_paths_operate(d, VECTOR_PATH_INTERSECT));
    TEST_ASSERT_TRUE(d->rectangles[0].path.count > 3);
    TEST_ASSERT_EQUAL_FLOAT(0, d->rectangles[0].angle);
}

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_path_hole_click_selects_the_visible_object_behind_it);
    RUN_TEST(test_all_stroke_placements_preserve_alpha_when_converting_to_path);
    RUN_TEST(test_three_operand_division_and_disconnected_components);
    RUN_TEST(test_native_radial_path_conversion_and_rotated_booleans);
    RUN_TEST(test_boolean_geometry_area_winding_holes_and_touching_edges);
    RUN_TEST(test_outline_alignment_and_hole_sides);
    RUN_TEST(test_conversion_preserves_geometry_styles_order_and_pixel_fill);
    RUN_TEST(test_operations_are_atomic_and_division_produces_independent_pieces);
    RUN_TEST(test_hole_rendering_hit_testing_and_clipboard_survive_editor_switch);
    RUN_TEST(test_empty_result_locks_parent_guards_and_complexity_limits);
    RUN_TEST(test_stroke_to_path_preserves_band_appearance_and_is_undoable);
    return UNITY_END();
}
