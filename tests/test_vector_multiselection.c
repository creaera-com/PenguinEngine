#include "editor_host/host.h"
#include "editors/vector/document/selection.h"
#include "editors/vector/geometry/resize.h"
#include "editors/vector/internal.h"
#include "editors/vector/rectangle/transform.h"
#include "editors/vector/workspace/selection.h"
#include "shell/shell.h"
#include "unity.h"

static SDL_Window *window;
static SDL_Renderer *renderer;
static EditorShell shell;
static EditorHost host;

static void frame(void) {
    pui_view_draw(shell.view, &shell.input, (PUIRect){0, 0, 1280, 800}, NULL, NULL);
}

static void commit(void) {
    char error[256];
    TEST_ASSERT_TRUE_MESSAGE(
        editor_host_commit(&host, shell.view, &shell.input, error, sizeof error), error);
    frame();
}

static VectorDocument *document(void) {
    VectorEditor *editor = host.instance;
    return vector_document_tabs_active(&editor->documents, shell.view);
}

static void activate(const char *id) {
    TEST_ASSERT_TRUE(editor_host_request(&host, id, false));
    commit();
}

void setUp(void) {
    TEST_ASSERT_TRUE(SDL_Init(SDL_INIT_VIDEO));
    SDL_SetModState(0);
    window = SDL_CreateWindow("multi", 1280, 800, SDL_WINDOW_HIDDEN);
    renderer = SDL_CreateRenderer(window, NULL);
    TEST_ASSERT_NOT_NULL(renderer);
    char path[1600], error[256];
    snprintf(path, sizeof path, "%s/shell.yaml", PENGUI_EDITOR_RESOURCES);
    TEST_ASSERT_TRUE_MESSAGE(editor_shell_load(&shell, renderer, path, error, sizeof error), error);
    editor_host_init(&host, (EditorServices){PENGUI_EDITOR_RESOURCES});
    activate("vector");
    commit();
    VectorDocument *doc = document();
    TEST_ASSERT_TRUE(vector_artboard_add(doc, (PUIRect){0, 0, 300, 220}));
    TEST_ASSERT_TRUE(vector_rectangle_add(doc, 1, (PUIRect){20, 20, 40, 30}));
    TEST_ASSERT_TRUE(vector_rectangle_add(doc, 1, (PUIRect){100, 70, 40, 30}));
    vector_document_select(doc, 0);
    doc->active_artboard = 0;
    commit();
}

void tearDown(void) {
    SDL_SetModState(0);
    editor_shell_destroy(&shell);
    editor_host_destroy(&host);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
}

static void pointer(SDL_EventType type, float x, float y) {
    VectorViewport *v = &document()->viewport;
    x = v->bounds.x + v->x + x * v->zoom;
    y = v->bounds.y + v->y + y * v->zoom;
    SDL_Event event = {.type = type};
    if (type == SDL_EVENT_MOUSE_MOTION) {
        event.motion.x = x;
        event.motion.y = y;
    } else {
        event.button.button = SDL_BUTTON_LEFT;
        event.button.x = x;
        event.button.y = y;
    }
    bool consumed = editor_host_event(&host, shell.view, &event);
    if (event.type != SDL_EVENT_MOUSE_BUTTON_UP) {
        TEST_ASSERT_TRUE(consumed);
    }
    commit();
}

static void click(float x, float y, bool shift) {
    SDL_SetModState(shift ? SDL_KMOD_SHIFT : 0);
    pointer(SDL_EVENT_MOUSE_BUTTON_DOWN, x, y);
    pointer(SDL_EVENT_MOUSE_BUTTON_UP, x, y);
    SDL_SetModState(0);
}

static void cancel(void) {
    SDL_Event event = {.type = SDL_EVENT_KEY_DOWN};
    event.key.key = SDLK_ESCAPE;
    TEST_ASSERT_TRUE(editor_host_event(&host, shell.view, &event));
    commit();
}

static void test_shift_selection_toggles_and_tree_supports_shift(void) {
    VectorDocument *doc = document();
    click(40, 35, false);
    click(120, 85, true);
    TEST_ASSERT_EQUAL_INT(2, doc->selection.count);
    TEST_ASSERT_TRUE(vector_document_is_selected(doc, 1));
    TEST_ASSERT_TRUE(vector_document_is_selected(doc, 2));
    TEST_ASSERT_EQUAL_STRING("2 Rechtecke ausgewaehlt",
                             pui_view_text(shell.view, "vector_selection_count"));
    click(40, 35, false);
    TEST_ASSERT_EQUAL_INT(2, doc->selection.count);
    click(40, 35, true);
    TEST_ASSERT_FALSE(vector_document_is_selected(doc, 1));
    TEST_ASSERT_TRUE(vector_document_is_selected(doc, 2));
    SDL_SetModState(SDL_KMOD_SHIFT);
    TEST_ASSERT_TRUE(editor_host_action(&host, shell.view, "vector.rectangle.select",
                                        "vector_document_1.rectangle_1"));
    SDL_SetModState(0);
    commit();
    TEST_ASSERT_EQUAL_INT(2, doc->selection.count);
    click(5, 5, false);
    TEST_ASSERT_EQUAL_INT(0, doc->selection.count);
    TEST_ASSERT_EQUAL_UINT(0, doc->active_rectangle);
    TEST_ASSERT_EQUAL_INT(3, doc->history.count);
}

static void test_marquee_zoom_reverse_additive_and_escape_restore(void) {
    VectorDocument *doc = document();
    doc->viewport.zoom = 1.5f;
    commit();
    pointer(SDL_EVENT_MOUSE_BUTTON_DOWN, 180, 120);
    pointer(SDL_EVENT_MOUSE_MOTION, 5, 5);
    TEST_ASSERT_EQUAL_INT(2, doc->selection.count);
    TEST_ASSERT_TRUE(doc->selection_tool.marquee);
    pointer(SDL_EVENT_MOUSE_BUTTON_UP, 5, 5);
    TEST_ASSERT_FALSE(doc->selection_tool.marquee);
    click(40, 35, false);
    click(120, 85, true); // remove the second rectangle
    TEST_ASSERT_EQUAL_INT(1, doc->selection.count);
    SDL_SetModState(SDL_KMOD_SHIFT);
    pointer(SDL_EVENT_MOUSE_BUTTON_DOWN, 90, 60);
    pointer(SDL_EVENT_MOUSE_MOTION, 150, 110);
    TEST_ASSERT_EQUAL_INT(2, doc->selection.count);
    cancel();
    SDL_SetModState(0);
    TEST_ASSERT_EQUAL_INT(1, doc->selection.count);
    TEST_ASSERT_TRUE(vector_document_is_selected(doc, 1));
    TEST_ASSERT_EQUAL_INT(3, doc->history.count);
}

static void test_group_move_is_one_history_step_and_survives_editor_switch(void) {
    VectorDocument *doc = document();
    click(40, 35, false);
    click(120, 85, true);
    pointer(SDL_EVENT_MOUSE_BUTTON_DOWN, 120, 85);
    pointer(SDL_EVENT_MOUSE_MOTION, 135, 95);
    TEST_ASSERT_EQUAL_FLOAT(20, doc->rectangles[0].rect.x);
    TEST_ASSERT_EQUAL_FLOAT(100, doc->rectangles[1].rect.x);
    TEST_ASSERT_EQUAL_FLOAT(35, vector_rectangle_display_rect(doc, &doc->rectangles[0]).x);
    TEST_ASSERT_EQUAL_FLOAT(115, vector_rectangle_display_rect(doc, &doc->rectangles[1]).x);
    pointer(SDL_EVENT_MOUSE_BUTTON_UP, 135, 95);
    TEST_ASSERT_EQUAL_INT(4, doc->history.count);
    TEST_ASSERT_EQUAL_FLOAT(35, doc->rectangles[0].rect.x);
    TEST_ASSERT_EQUAL_FLOAT(115, doc->rectangles[1].rect.x);
    TEST_ASSERT_TRUE(vector_history_undo(&doc->history, doc));
    commit();
    TEST_ASSERT_EQUAL_FLOAT(20, doc->rectangles[0].rect.x);
    TEST_ASSERT_EQUAL_FLOAT(100, doc->rectangles[1].rect.x);
    activate("scripts");
    activate("vector");
    commit();
    doc = document();
    TEST_ASSERT_EQUAL_INT(2, doc->selection.count);
    TEST_ASSERT_TRUE(vector_history_redo(&doc->history, doc));
    TEST_ASSERT_EQUAL_FLOAT(35, doc->rectangles[0].rect.x);
    TEST_ASSERT_EQUAL_FLOAT(115, doc->rectangles[1].rect.x);
    click(135, 95, false);
    pointer(SDL_EVENT_MOUSE_BUTTON_DOWN, 135, 95);
    pointer(SDL_EVENT_MOUSE_MOTION, 150, 110);
    cancel();
    TEST_ASSERT_EQUAL_FLOAT(35, doc->rectangles[0].rect.x);
    TEST_ASSERT_EQUAL_INT(4, doc->history.count);
}

static void test_locked_hidden_and_atomic_failure(void) {
    VectorDocument *doc = document();
    TEST_ASSERT_TRUE(vector_rectangle_set_state(doc, 2, VECTOR_OBJECT_LOCKED));
    commit();
    pointer(SDL_EVENT_MOUSE_BUTTON_DOWN, 5, 5);
    pointer(SDL_EVENT_MOUSE_BUTTON_UP, 180, 120);
    TEST_ASSERT_TRUE(vector_document_is_selected(doc, 1));
    TEST_ASSERT_FALSE(vector_document_is_selected(doc, 2));
    unsigned ids[] = {1, 2};
    int count = doc->history.count;
    TEST_ASSERT_FALSE(vector_rectangles_translate(doc, ids, 2, 10, 20));
    TEST_ASSERT_EQUAL_FLOAT(20, doc->rectangles[0].rect.x);
    TEST_ASSERT_EQUAL_INT(count, doc->history.count);
    TEST_ASSERT_TRUE(vector_rectangle_set_state(doc, 2, VECTOR_OBJECT_HIDDEN));
    commit();
    pointer(SDL_EVENT_MOUSE_BUTTON_DOWN, 5, 5);
    pointer(SDL_EVENT_MOUSE_BUTTON_UP, 180, 120);
    TEST_ASSERT_FALSE(vector_document_is_selected(doc, 2));
    TEST_ASSERT_TRUE(vector_artboard_set_state(doc, 1, VECTOR_OBJECT_LOCKED));
    commit();
    pointer(SDL_EVENT_MOUSE_BUTTON_DOWN, 5, 5);
    pointer(SDL_EVENT_MOUSE_BUTTON_UP, 180, 120);
    TEST_ASSERT_EQUAL_INT(0, doc->selection.count);
    TEST_ASSERT_EQUAL_UINT(0, doc->active_rectangle);
}

static void test_group_snapping_keeps_spacing_and_lock_change_cancels_all(void) {
    VectorDocument *doc = document();
    click(40, 35, false);
    click(120, 85, true);
    doc->grid.snap_enabled = true;
    pointer(SDL_EVENT_MOUSE_BUTTON_DOWN, 120, 85);
    pointer(SDL_EVENT_MOUSE_MOTION, 134, 99);
    PUIRect first = vector_rectangle_display_rect(doc, &doc->rectangles[0]);
    PUIRect second = vector_rectangle_display_rect(doc, &doc->rectangles[1]);
    TEST_ASSERT_EQUAL_FLOAT(80, second.x - first.x);
    TEST_ASSERT_EQUAL_FLOAT(50, second.y - first.y);
    pointer(SDL_EVENT_MOUSE_BUTTON_UP, 134, 99);
    TEST_ASSERT_EQUAL_INT(4, doc->history.count);
    first = doc->rectangles[0].rect;
    second = doc->rectangles[1].rect;
    pointer(SDL_EVENT_MOUSE_BUTTON_DOWN, second.x + 20, second.y + 15);
    pointer(SDL_EVENT_MOUSE_MOTION, second.x + 40, second.y + 35);
    // Lock a non-primary member while a group gesture is in progress.
    TEST_ASSERT_TRUE(vector_rectangle_set_state(doc, 1, VECTOR_OBJECT_LOCKED));
    commit();
    pointer(SDL_EVENT_MOUSE_MOTION, second.x + 50, second.y + 45);
    TEST_ASSERT_FALSE(doc->selection_tool.dragging);
    TEST_ASSERT_EQUAL_FLOAT(first.x, doc->rectangles[0].rect.x);
    TEST_ASSERT_EQUAL_FLOAT(second.x, doc->rectangles[1].rect.x);
    TEST_ASSERT_EQUAL_INT(5, doc->history.count);
}

static void test_editor_switch_discards_marquee_selection_preview(void) {
    click(40, 35, false);
    pointer(SDL_EVENT_MOUSE_BUTTON_DOWN, 5, 5);
    pointer(SDL_EVENT_MOUSE_MOTION, 180, 120);
    TEST_ASSERT_EQUAL_INT(2, document()->selection.count);
    activate("scripts");
    activate("vector");
    commit();
    TEST_ASSERT_EQUAL_INT(1, document()->selection.count);
    TEST_ASSERT_TRUE(vector_document_is_selected(document(), 1));
    TEST_ASSERT_FALSE(document()->selection_tool.marquee);
    TEST_ASSERT_EQUAL_INT(3, document()->history.count);
}

static void assert_geometry(PUIRect expected, PUIRect actual) {
    TEST_ASSERT_FLOAT_WITHIN(.001f, expected.x, actual.x);
    TEST_ASSERT_FLOAT_WITHIN(.001f, expected.y, actual.y);
    TEST_ASSERT_FLOAT_WITHIN(.001f, expected.w, actual.w);
    TEST_ASSERT_FLOAT_WITHIN(.001f, expected.h, actual.h);
}

static void select_pair(void) {
    vector_document_select(document(), 1);
    vector_document_toggle(document(), 2);
    commit();
}

static void test_all_group_handles_scale_relative_positions_with_single_history_steps(void) {
    VectorDocument *doc = document();
    select_pair();
    doc->viewport.zoom = 1.5f;
    commit();
    const float handles[8][2] = {{20, 20},   {80, 20},  {140, 20}, {140, 60},
                                 {140, 100}, {80, 100}, {20, 100}, {20, 60}};
    for (int i = 0; i < 8; i++) {
        unsigned edges = vector_resize_handle_edges(i);
        float x = handles[i][0], y = handles[i][1];
        pointer(SDL_EVENT_MOUSE_BUTTON_DOWN, x, y);
        TEST_ASSERT_EQUAL_UINT(edges, doc->selection_tool.resize_edges);
        pointer(SDL_EVENT_MOUSE_MOTION, x + 24, y + 16);
        TEST_ASSERT_EQUAL_INT(3, doc->history.cursor);
        assert_geometry((PUIRect){20, 20, 40, 30}, doc->rectangles[0].rect);
        float left = edges & VECTOR_RESIZE_LEFT ? 44 : 20;
        float top = edges & VECTOR_RESIZE_TOP ? 36 : 20;
        float right = edges & VECTOR_RESIZE_RIGHT ? 164 : 140;
        float bottom = edges & VECTOR_RESIZE_BOTTOM ? 116 : 100;
        float w = right - left, h = bottom - top;
        PUIRect first = {left, top, w / 3, h * .375f};
        PUIRect second = {left + w * (2.f / 3), top + h * .625f, w / 3, h * .375f};
        assert_geometry(first, vector_rectangle_display_rect(doc, &doc->rectangles[0]));
        assert_geometry(second, vector_rectangle_display_rect(doc, &doc->rectangles[1]));
        pointer(SDL_EVENT_MOUSE_BUTTON_UP, x + 24, y + 16);
        TEST_ASSERT_EQUAL_INT(4, doc->history.cursor);
        TEST_ASSERT_EQUAL_INT(4, doc->history.count);
        assert_geometry(first, doc->rectangles[0].rect);
        assert_geometry(second, doc->rectangles[1].rect);
        TEST_ASSERT_TRUE(vector_history_undo(&doc->history, doc));
        commit();
        assert_geometry((PUIRect){20, 20, 40, 30}, doc->rectangles[0].rect);
        assert_geometry((PUIRect){100, 70, 40, 30}, doc->rectangles[1].rect);
    }
}

static void test_group_resize_minimum_cancel_noop_and_checkpoint_redo(void) {
    VectorDocument *doc = document();
    select_pair();
    pointer(SDL_EVENT_MOUSE_BUTTON_DOWN, 20, 20);
    pointer(SDL_EVENT_MOUSE_BUTTON_UP, 20, 20);
    TEST_ASSERT_EQUAL_INT(3, doc->history.count);
    pointer(SDL_EVENT_MOUSE_BUTTON_DOWN, 140, 100);
    pointer(SDL_EVENT_MOUSE_MOTION, 0, 0);
    PUIRect first = vector_rectangle_display_rect(doc, &doc->rectangles[0]);
    PUIRect second = vector_rectangle_display_rect(doc, &doc->rectangles[1]);
    TEST_ASSERT_TRUE(first.w >= 1 && first.h >= 1 && second.w >= 1 && second.h >= 1);
    TEST_ASSERT_EQUAL_FLOAT(20, first.x);
    TEST_ASSERT_EQUAL_FLOAT(20, first.y);
    cancel();
    TEST_ASSERT_EQUAL_INT(3, doc->history.count);
    assert_geometry((PUIRect){20, 20, 40, 30}, doc->rectangles[0].rect);
    pointer(SDL_EVENT_MOUSE_BUTTON_DOWN, 140, 100);
    pointer(SDL_EVENT_MOUSE_BUTTON_UP, 164, 116);
    TEST_ASSERT_EQUAL_INT(4, doc->history.count);
    TEST_ASSERT_TRUE(vector_history_undo(&doc->history, doc));
    activate("scripts");
    activate("vector");
    commit();
    doc = document();
    TEST_ASSERT_TRUE(vector_history_redo(&doc->history, doc));
    commit();
    assert_geometry((PUIRect){20, 20, 48, 36}, doc->rectangles[0].rect);
    assert_geometry((PUIRect){116, 80, 48, 36}, doc->rectangles[1].rect);
    // A switch while resizing keeps the committed geometry, not the preview.
    pointer(SDL_EVENT_MOUSE_BUTTON_DOWN, 164, 116);
    pointer(SDL_EVENT_MOUSE_MOTION, 180, 130);
    activate("scripts");
    activate("vector");
    commit();
    assert_geometry((PUIRect){20, 20, 48, 36}, document()->rectangles[0].rect);
    TEST_ASSERT_FALSE(document()->selection_tool.dragging);
}

static void test_group_resize_across_artboards_and_grid_snapping(void) {
    VectorDocument *doc = document();
    TEST_ASSERT_TRUE(vector_artboard_add(doc, (PUIRect){320, 0, 180, 200}));
    TEST_ASSERT_TRUE(vector_rectangle_add(doc, 2, (PUIRect){20, 20, 40, 30}));
    vector_document_select(doc, 1);
    vector_document_toggle(doc, 3);
    commit();
    doc->grid.snap_enabled = true;
    // Shared world bounds: (20,20)-(380,50). Right edge 397 snaps to 400
    // relative to the primary artboard origin at x=320.
    pointer(SDL_EVENT_MOUSE_BUTTON_DOWN, 380, 35);
    pointer(SDL_EVENT_MOUSE_MOTION, 397, 35);
    PUIRect first = vector_rectangle_display_rect(doc, &doc->rectangles[0]);
    PUIRect third = vector_rectangle_display_rect(doc, &doc->rectangles[2]);
    TEST_ASSERT_FLOAT_WITHIN(.001f, 400, third.x + 320 + third.w);
    TEST_ASSERT_FLOAT_WITHIN(.001f, first.w, third.w);
    TEST_ASSERT_EQUAL_FLOAT(20, first.y);
    TEST_ASSERT_EQUAL_FLOAT(30, first.h);
    pointer(SDL_EVENT_MOUSE_BUTTON_UP, 397, 35);
    TEST_ASSERT_EQUAL_UINT(1, doc->rectangles[0].parent_id);
    TEST_ASSERT_EQUAL_UINT(2, doc->rectangles[2].parent_id);
    TEST_ASSERT_EQUAL_INT(6, doc->history.count);
    unsigned ids[] = {1, 3};
    PUIRect invalid[] = {{0, 0, 20, 20}, {0, 0, 0, 20}};
    PUIRect unchanged = doc->rectangles[0].rect;
    TEST_ASSERT_FALSE(vector_rectangles_set_rects(doc, ids, invalid, 2));
    assert_geometry(unchanged, doc->rectangles[0].rect);
    TEST_ASSERT_EQUAL_INT(6, doc->history.count);
}

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_all_group_handles_scale_relative_positions_with_single_history_steps);
    RUN_TEST(test_group_resize_minimum_cancel_noop_and_checkpoint_redo);
    RUN_TEST(test_group_resize_across_artboards_and_grid_snapping);
    RUN_TEST(test_editor_switch_discards_marquee_selection_preview);
    RUN_TEST(test_group_snapping_keeps_spacing_and_lock_change_cancels_all);
    RUN_TEST(test_shift_selection_toggles_and_tree_supports_shift);
    RUN_TEST(test_marquee_zoom_reverse_additive_and_escape_restore);
    RUN_TEST(test_group_move_is_one_history_step_and_survives_editor_switch);
    RUN_TEST(test_locked_hidden_and_atomic_failure);
    return UNITY_END();
}
