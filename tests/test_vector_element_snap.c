#include "editor_host/host.h"
#include "editors/vector/document/selection.h"
#include "editors/vector/internal.h"
#include "editors/vector/workspace/element_snap.h"
#include "editors/vector/workspace/selection.h"
#include "shell/shell.h"
#include "unity.h"
#include <math.h>
#include <string.h>

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

static void activate(const char *id) {
    TEST_ASSERT_TRUE(editor_host_request(&host, id, false));
    commit();
}

static VectorDocument *document(void) {
    return vector_document_tabs_active(&((VectorEditor *)host.instance)->documents, shell.view);
}

void setUp(void) {
    TEST_ASSERT_TRUE(SDL_Init(SDL_INIT_VIDEO));
    window = SDL_CreateWindow("element snapping", 1280, 800, SDL_WINDOW_HIDDEN);
    renderer = SDL_CreateRenderer(window, NULL);
    TEST_ASSERT_NOT_NULL(renderer);
    char path[1600], error[256];
    snprintf(path, sizeof path, "%s/shell.yaml", PENGUI_EDITOR_RESOURCES);
    TEST_ASSERT_TRUE_MESSAGE(editor_shell_load(&shell, renderer, path, error, sizeof error), error);
    editor_host_init(&host, (EditorServices){PENGUI_EDITOR_RESOURCES});
    activate("vector");
    commit();
    VectorDocument *d = document();
    vector_history_destroy(&d->history);
    vector_document_init(d, d->id);
    TEST_ASSERT_TRUE(vector_artboard_add(d, (PUIRect){0, 0, 600, 400}));
    TEST_ASSERT_TRUE(vector_rectangle_add(d, 1, (PUIRect){50, 80, 20, 20}));
    TEST_ASSERT_TRUE(vector_rectangle_add(d, 1, (PUIRect){150, 140, 40, 40}));
    vector_document_select(d, 1);
    d->element_snap_enabled = true;
    commit();
}

void tearDown(void) {
    editor_shell_destroy(&shell);
    editor_host_destroy(&host);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
}

static void drag(float dx, float dy) {
    VectorDocument *d = document();
    d->selection_tool = (VectorSelectionTool){.dragging = true,
                                              .artboard_id = 1,
                                              .rectangle_id = 1,
                                              .before = d->selection,
                                              .offset_x = dx,
                                              .offset_y = dy};
}

static void test_geometry_alignment_and_zoom_tolerance(void) {
    PUIRect target = {0, 0, 40, 50};
    VectorElementSnap s = vector_element_snap((PUIRect){42, 53, 20, 20}, &target, 1, 5);
    TEST_ASSERT_TRUE(s.snapped_x && s.snapped_y);
    TEST_ASSERT_EQUAL_FLOAT(-2, s.dx);
    TEST_ASSERT_EQUAL_FLOAT(-3, s.dy);
    TEST_ASSERT_EQUAL_INT(2, s.guide_count);
    s = vector_element_snap((PUIRect){42, 53, 20, 20}, &target, 1, 1);
    TEST_ASSERT_FALSE(s.snapped_x || s.snapped_y);
    s = vector_element_snap((PUIRect){NAN, 0, 20, 20}, &target, 1, 5);
    TEST_ASSERT_EQUAL_INT(0, s.guide_count);
    drag(79, 59); // Moving right edge 149 -> target left edge 150; bottom 159 -> center 160.
    s = vector_workspace_element_snap(document());
    TEST_ASSERT_EQUAL_FLOAT(1, s.dx);
    TEST_ASSERT_EQUAL_FLOAT(1, s.dy);
    document()->viewport.zoom = 4;
    drag(77, 59);
    s = vector_workspace_element_snap(document());
    TEST_ASSERT_FALSE(s.snapped_x);
    TEST_ASSERT_TRUE(s.snapped_y);
}

static void test_equal_gaps_horizontal_and_vertical(void) {
    PUIRect neighbours[] = {{0, 10, 20, 20}, {100, 10, 20, 20}};
    VectorElementSnap s = vector_element_snap((PUIRect){48, 10, 20, 20}, neighbours, 2, 5);
    TEST_ASSERT_EQUAL_FLOAT(2, s.dx);
    int brackets = 0;
    for (int i = 0; i < s.guide_count; i++) {
        if (s.guides[i].spacing) {
            TEST_ASSERT_EQUAL_FLOAT(30, s.guides[i].x2 - s.guides[i].x1);
            brackets++;
        }
    }
    TEST_ASSERT_EQUAL_INT(2, brackets);
    neighbours[0] = (PUIRect){10, 0, 20, 20};
    neighbours[1] = (PUIRect){10, 100, 20, 20};
    s = vector_element_snap((PUIRect){10, 48, 20, 20}, neighbours, 2, 5);
    TEST_ASSERT_EQUAL_FLOAT(2, s.dy);
    brackets = 0;
    for (int i = 0; i < s.guide_count; i++) {
        if (s.guides[i].spacing) {
            TEST_ASSERT_EQUAL_FLOAT(30, s.guides[i].y2 - s.guides[i].y1);
            brackets++;
        }
    }
    TEST_ASSERT_EQUAL_INT(2, brackets);
    s = vector_element_snap((PUIRect){10, 48, 90, 90}, neighbours, 2, 5);
    for (int i = 0; i < s.guide_count; i++) {
        TEST_ASSERT_FALSE(s.guides[i].spacing);
    }
}

static void test_preview_commit_undo_cancel_and_independent_grid(void) {
    VectorDocument *d = document();
    PUIRect original = d->rectangles[0].rect;
    int history = d->history.count;
    drag(79, 59);
    PUIRect shown = vector_rectangle_display_rect(d, &d->rectangles[0]);
    TEST_ASSERT_EQUAL_FLOAT(130, shown.x);
    TEST_ASSERT_EQUAL_FLOAT(140, shown.y);
    TEST_ASSERT_EQUAL_MEMORY(&original, &d->rectangles[0].rect, sizeof original);
    TEST_ASSERT_EQUAL_INT(history, d->history.count);
    SDL_Event event = {.type = SDL_EVENT_MOUSE_BUTTON_UP};
    event.button.button = SDL_BUTTON_LEFT;
    event.button.x = 79;
    event.button.y = 59;
    TEST_ASSERT_TRUE(vector_selection_event(d, shell.view, &event));
    TEST_ASSERT_EQUAL_FLOAT(130, d->rectangles[0].rect.x);
    TEST_ASSERT_EQUAL_INT(history + 1, d->history.count);
    TEST_ASSERT_EQUAL_INT(0, vector_workspace_element_snap(d).guide_count);
    TEST_ASSERT_TRUE(vector_history_undo(&d->history, d));
    TEST_ASSERT_EQUAL_MEMORY(&original, &d->rectangles[0].rect, sizeof original);
    TEST_ASSERT_TRUE(vector_history_redo(&d->history, d));
    drag(50, 0);
    vector_selection_cancel(d);
    TEST_ASSERT_EQUAL_INT(0, vector_workspace_element_snap(d).guide_count);
    TEST_ASSERT_TRUE(
        editor_host_action(&host, shell.view, "vector.element.snap", "vector_element_snap"));
    TEST_ASSERT_FALSE(d->element_snap_enabled);
    drag(1, 1);
    TEST_ASSERT_EQUAL_INT(0, vector_workspace_element_snap(d).guide_count);
    TEST_ASSERT_FALSE(d->grid.snap_enabled);
}

static void test_hidden_locked_group_and_checkpoint_preferences(void) {
    VectorDocument *d = document();
    drag(79, 59);
    d->rectangles[1].flags = VECTOR_OBJECT_HIDDEN;
    TEST_ASSERT_EQUAL_INT(0, vector_workspace_element_snap(d).guide_count);
    d->rectangles[1].flags = VECTOR_OBJECT_LOCKED;
    TEST_ASSERT_TRUE(vector_workspace_element_snap(d).guide_count > 0);
    d->rectangles[1].flags = 0;
    vector_document_toggle(d, 2);
    drag(79, 59);
    TEST_ASSERT_EQUAL_INT(0, vector_workspace_element_snap(d).guide_count);
    vector_selection_cancel(d);
    /* Mutations above were reference-filter tests; persist only history-backed model values. */
    d->rectangles[1].flags = 0;
    activate("scripts");
    activate("vector");
    commit();
    d = document();
    TEST_ASSERT_TRUE(d->element_snap_enabled);
    TEST_ASSERT_TRUE(pui_view_checked(shell.view, "vector_element_snap"));
    TEST_ASSERT_FALSE(d->selection_tool.dragging);
    TEST_ASSERT_FALSE(d->grid.snap_enabled);
}

static void test_object_priority_over_grid_and_group_translation(void) {
    VectorDocument *d = document();
    d->grid.snap_enabled = true;
    d->grid.snap_tolerance = 5;
    drag(79, 59);
    PUIRect shown = vector_rectangle_display_rect(d, &d->rectangles[0]);
    TEST_ASSERT_EQUAL_FLOAT(130, shown.x); // Object edge wins over grid x=128.
    TEST_ASSERT_EQUAL_FLOAT(140, shown.y);
    TEST_ASSERT_TRUE(vector_rectangle_add(d, 1, (PUIRect){80, 80, 20, 20}));
    vector_document_select(d, 1);
    vector_document_toggle(d, 3);
    drag(49, 59); // Group right edge approaches reference x=150.
    PUIRect first = vector_rectangle_display_rect(d, &d->rectangles[0]);
    PUIRect second = vector_rectangle_display_rect(d, &d->rectangles[2]);
    TEST_ASSERT_EQUAL_FLOAT(30, second.x - first.x);
    TEST_ASSERT_EQUAL_FLOAT(0, second.y - first.y);
    TEST_ASSERT_EQUAL_FLOAT(100, first.x);
}

static void test_artboard_excludes_children_and_rotated_targets_use_world_bounds(void) {
    VectorDocument *d = document();
    d->selection_tool =
        (VectorSelectionTool){.dragging = true, .artboard_id = 1, .offset_x = 0, .offset_y = 0};
    TEST_ASSERT_EQUAL_INT(0, vector_workspace_element_snap(d).guide_count);
    TEST_ASSERT_TRUE(vector_artboard_add(d, (PUIRect){800, 0, 200, 200}));
    d->selection_tool.offset_x = 199;
    VectorElementSnap snap = vector_workspace_element_snap(d);
    TEST_ASSERT_TRUE(snap.snapped_x);
    TEST_ASSERT_EQUAL_FLOAT(1, snap.dx);
    vector_selection_cancel(d);
    vector_document_select(d, 1);
    d->rectangles[1].angle = 90;
    d->rectangles[1].rect = (PUIRect){150, 140, 60, 20}; // AABB left=170, top=120.
    drag(99, 19);                                        // Right edge=169, bottom=119.
    snap = vector_workspace_element_snap(d);
    TEST_ASSERT_EQUAL_FLOAT(1, snap.dx);
    TEST_ASSERT_EQUAL_FLOAT(1, snap.dy);
}

static void test_spacing_uses_immediate_neighbours_and_overlay_preserves_clip(void) {
    PUIRect neighbours[] = {{0, 10, 20, 20}, {100, 10, 20, 20}, {30, 10, 10, 20}};
    VectorElementSnap snap = vector_element_snap((PUIRect){48, 10, 20, 20}, neighbours, 3, 5);
    for (int i = 0; i < snap.guide_count; i++) {
        TEST_ASSERT_FALSE(snap.guides[i].spacing); // Outer pair would falsely suggest +2.
    }
    VectorDocument *d = document();
    drag(79, 59);
    SDL_Rect clip = {100, 100, 800, 600}, actual;
    SDL_SetRenderClipRect(renderer, &clip);
    vector_element_guides_draw(d, renderer);
    TEST_ASSERT_TRUE(SDL_RenderClipEnabled(renderer));
    SDL_GetRenderClipRect(renderer, &actual);
    TEST_ASSERT_EQUAL_MEMORY(&clip, &actual, sizeof clip);
    SDL_SetRenderClipRect(renderer, NULL);
}

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_object_priority_over_grid_and_group_translation);
    RUN_TEST(test_artboard_excludes_children_and_rotated_targets_use_world_bounds);
    RUN_TEST(test_spacing_uses_immediate_neighbours_and_overlay_preserves_clip);
    RUN_TEST(test_geometry_alignment_and_zoom_tolerance);
    RUN_TEST(test_equal_gaps_horizontal_and_vertical);
    RUN_TEST(test_preview_commit_undo_cancel_and_independent_grid);
    RUN_TEST(test_hidden_locked_group_and_checkpoint_preferences);
    return UNITY_END();
}
