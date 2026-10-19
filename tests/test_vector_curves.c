#include "editor_host/host.h"
#include "editors/vector/curve/curve.h"
#include "editors/vector/curve/tool.h"
#include "editors/vector/document/selection.h"
#include "editors/vector/internal.h"
#include "editors/vector/object/clipboard.h"
#include "editors/vector/object/group.h"
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

static void action(void *context, const char *name, const char *widget) {
    TEST_ASSERT_TRUE(editor_host_action(context, shell.view, name, widget));
}

static void commit(void) {
    char error[256];
    TEST_ASSERT_TRUE_MESSAGE(
        editor_host_commit(&host, shell.view, &shell.input, error, sizeof error), error);
    pui_view_draw(shell.view, &shell.input, (PUIRect){0, 0, 1280, 800}, action, &host);
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

static VectorBezier arch(void) {
    return (VectorBezier){.count = 2,
                          .nodes = {{{0, 0}, {0, 0}, {0, 100}}, {{100, 0}, {100, 100}, {100, 0}}}};
}

static void tool(const char *name) {
    TEST_ASSERT_TRUE(editor_host_action(&host, shell.view, name, ""));
    commit();
}

static void send(SDL_Event *e) {
    if (!pui_view_event(shell.view, &shell.input, window, e)) {
        editor_host_event(&host, shell.view, e);
    }
    commit();
}

static void pointer(Uint32 type, float x, float y) {
    const VectorViewport *v = &document()->viewport;
    float sx = v->bounds.x + v->x + x * v->zoom, sy = v->bounds.y + v->y + y * v->zoom;
    SDL_Event e = {.type = type};
    if (type == SDL_EVENT_MOUSE_MOTION) {
        e.motion.x = sx;
        e.motion.y = sy;
    } else {
        e.button.button = SDL_BUTTON_LEFT;
        e.button.x = sx;
        e.button.y = sy;
    }
    send(&e);
}

static void point(float x, float y) {
    pointer(SDL_EVENT_MOUSE_BUTTON_DOWN, x, y);
    pointer(SDL_EVENT_MOUSE_BUTTON_UP, x, y);
}

static void key(SDL_Keycode code) {
    SDL_Event e = {.type = SDL_EVENT_KEY_DOWN};
    e.key.key = code;
    send(&e);
}

static void test_exact_bounds_and_adaptive_geometry(void) {
    VectorBezier c = arch();
    PUIRect b = vector_bezier_bounds(&c);
    TEST_ASSERT_EQUAL_FLOAT(0, b.x);
    TEST_ASSERT_EQUAL_FLOAT(0, b.y);
    TEST_ASSERT_EQUAL_FLOAT(100, b.w);
    TEST_ASSERT_EQUAL_FLOAT(75, b.h);
    VectorPoint p = vector_bezier_evaluate(c.nodes[0], c.nodes[1], .5f);
    TEST_ASSERT_EQUAL_FLOAT(50, p.x);
    TEST_ASSERT_EQUAL_FLOAT(75, p.y);
    VectorPath line;
    TEST_ASSERT_TRUE(vector_bezier_flatten(&c, .1f, &line));
    TEST_ASSERT_TRUE(line.count > 8 && line.count <= VECTOR_PATH_POINTS);
    TEST_ASSERT_EQUAL_FLOAT(0, line.points[0].x);
    TEST_ASSERT_EQUAL_FLOAT(100, line.points[line.count - 1].x);
    c.nodes[0].outgoing.x = NAN;
    TEST_ASSERT_FALSE(vector_bezier_valid(&c));
    TEST_ASSERT_FALSE(vector_bezier_flatten(&c, 1, &line));
}

static void test_domain_create_edit_undo_copy_and_no_rectangle_hit(void) {
    VectorDocument *d = document();
    VectorBezier c = arch();
    int history = d->history.count;
    TEST_ASSERT_TRUE(vector_curve_add(d, 1, &c));
    TEST_ASSERT_EQUAL_INT(history + 1, d->history.count);
    VectorShape original = d->rectangles[0];
    TEST_ASSERT_EQUAL_UINT(2, original.curve.count);
    TEST_ASSERT_EQUAL_UINT(0, original.path.count);
    TEST_ASSERT_TRUE(vector_rectangle_valid(original));
    TEST_ASSERT_TRUE(vector_shape_contains(&original, 50, 75));
    TEST_ASSERT_FALSE(vector_shape_contains(&original, 50, 0));
    TEST_ASSERT_FALSE(vector_shape_intersects(&original, (PUIRect){45, 0, 10, 5}));
    TEST_ASSERT_TRUE(vector_shape_intersects(&original, (PUIRect){45, 70, 10, 10}));
    c.nodes[0].outgoing.y = 120;
    TEST_ASSERT_TRUE(vector_curve_update(d, 1, &c));
    TEST_ASSERT_TRUE(vector_history_undo(&d->history, d));
    TEST_ASSERT_EQUAL_MEMORY(&original, &d->rectangles[0], sizeof original);
    TEST_ASSERT_TRUE(vector_history_redo(&d->history, d));
    VectorClipboard copy;
    TEST_ASSERT_TRUE(vector_objects_copy(d, &copy));
    TEST_ASSERT_TRUE(vector_objects_duplicate(d));
    TEST_ASSERT_EQUAL_MEMORY(&d->rectangles[0].curve, &d->rectangles[1].curve,
                             sizeof original.curve);
    TEST_ASSERT_FALSE(vector_paths_can_operate(d, VECTOR_PATH_UNION));
}

static void test_native_pen_drag_open_finish_and_history(void) {
    VectorDocument *d = document();
    int history = d->history.count;
    tool("vector.tool_pen");
    TEST_ASSERT_TRUE(pui_view_checked(shell.view, "vector_pen"));
    pointer(SDL_EVENT_MOUSE_BUTTON_DOWN, 50, 150);
    pointer(SDL_EVENT_MOUSE_MOTION, 80, 100);
    pointer(SDL_EVENT_MOUSE_BUTTON_UP, 80, 100);
    point(180, 100);
    point(300, 180);
    TEST_ASSERT_EQUAL_INT(0, d->rectangle_count);
    TEST_ASSERT_EQUAL_INT(history, d->history.count);
    key(SDLK_RETURN);
    TEST_ASSERT_EQUAL_INT(1, d->rectangle_count);
    TEST_ASSERT_EQUAL_INT(history + 1, d->history.count);
    TEST_ASSERT_EQUAL_UINT(3, d->rectangles[0].curve.count);
    TEST_ASSERT_FALSE(d->rectangles[0].curve.closed);
    VectorBezier c = vector_curve_parent(&d->rectangles[0]);
    TEST_ASSERT_FLOAT_WITHIN(.001f, 80, c.nodes[0].outgoing.x);
    TEST_ASSERT_FLOAT_WITHIN(.001f, 100, c.nodes[0].outgoing.y);
    TEST_ASSERT_FLOAT_WITHIN(.001f, 200, c.nodes[0].incoming.y);
    TEST_ASSERT_TRUE(d->curve_tool.pen);
    tool("editor.undo");
    TEST_ASSERT_EQUAL_INT(0, d->rectangle_count);
    tool("editor.redo");
    TEST_ASSERT_EQUAL_INT(1, d->rectangle_count);
}

static void test_nodes_live_preview_commit_escape_alt_and_delete(void) {
    VectorDocument *d = document();
    VectorBezier c = arch();
    for (unsigned i = 0; i < c.count; i++) {
        VectorPoint *p[] = {&c.nodes[i].point, &c.nodes[i].incoming, &c.nodes[i].outgoing};
        for (int j = 0; j < 3; j++) {
            p[j]->x += 70;
            p[j]->y += 100;
        }
    }
    TEST_ASSERT_TRUE(vector_curve_add(d, 1, &c));
    commit();
    VectorShape original = d->rectangles[0];
    int history = d->history.count;
    tool("vector.tool_nodes");
    pointer(SDL_EVENT_MOUSE_BUTTON_DOWN, 70, 100);
    pointer(SDL_EVENT_MOUSE_MOTION, 90, 110);
    TEST_ASSERT_EQUAL_MEMORY(&original, &d->rectangles[0], sizeof original);
    TEST_ASSERT_EQUAL_INT(history, d->history.count);
    const VectorShape *shown = vector_curve_display_shape(d, &d->rectangles[0]);
    VectorBezier preview = vector_curve_parent(shown);
    TEST_ASSERT_FLOAT_WITHIN(.01f, 90, preview.nodes[0].point.x);
    key(SDLK_ESCAPE);
    TEST_ASSERT_EQUAL_MEMORY(&original, &d->rectangles[0], sizeof original);
    tool("vector.tool_nodes");
    pointer(SDL_EVENT_MOUSE_BUTTON_DOWN, 70, 100);
    pointer(SDL_EVENT_MOUSE_MOTION, 90, 110);
    pointer(SDL_EVENT_MOUSE_BUTTON_UP, 90, 110);
    TEST_ASSERT_EQUAL_INT(history + 1, d->history.count);
    VectorBezier edited = vector_curve_parent(&d->rectangles[0]);
    TEST_ASSERT_FLOAT_WITHIN(.01f, 90, edited.nodes[0].point.x);
    TEST_ASSERT_FLOAT_WITHIN(.01f, 210, edited.nodes[0].outgoing.y);
    SDL_SetModState(SDL_KMOD_ALT);
    pointer(SDL_EVENT_MOUSE_BUTTON_DOWN, 90, 210);
    pointer(SDL_EVENT_MOUSE_MOTION, 120, 220);
    pointer(SDL_EVENT_MOUSE_BUTTON_UP, 120, 220);
    SDL_SetModState(0);
    VectorBezier broken = vector_curve_parent(&d->rectangles[0]);
    TEST_ASSERT_FLOAT_WITHIN(.01f, 120, broken.nodes[0].outgoing.x);
    TEST_ASSERT_FLOAT_WITHIN(.01f, edited.nodes[0].incoming.x, broken.nodes[0].incoming.x);
    TEST_ASSERT_FLOAT_WITHIN(.01f, edited.nodes[0].incoming.y, broken.nodes[0].incoming.y);
}

static void test_closed_curve_node_delete_checkpoint_and_mirror(void) {
    VectorDocument *d = document();
    tool("vector.tool_pen");
    point(60, 80);
    point(200, 80);
    point(200, 220);
    point(60, 220);
    point(60, 80);
    TEST_ASSERT_EQUAL_INT(1, d->rectangle_count);
    TEST_ASSERT_TRUE(d->rectangles[0].curve.closed);
    TEST_ASSERT_TRUE(vector_path_valid(&d->rectangles[0].path));
    tool("vector.tool_nodes");
    point(200, 80);
    key(SDLK_DELETE);
    TEST_ASSERT_EQUAL_UINT(3, d->rectangles[0].curve.count);
    tool("editor.undo");
    TEST_ASSERT_EQUAL_UINT(4, d->rectangles[0].curve.count);
    TEST_ASSERT_TRUE(vector_objects_orient(d, VECTOR_MIRROR_HORIZONTAL));
    VectorShape saved = d->rectangles[0];
    activate("scripts");
    activate("vector");
    commit();
    d = document();
    TEST_ASSERT_EQUAL_MEMORY(&saved, &d->rectangles[0], sizeof saved);
    TEST_ASSERT_FALSE(d->curve_tool.nodes || d->curve_tool.pen || d->curve_tool.dragging);
    tool("vector.tool_nodes");
    TEST_ASSERT_TRUE(pui_view_checked(shell.view, "vector_nodes"));
}

static void test_pen_cancel_grid_zoom_locked_parent_and_focus_loss(void) {
    VectorDocument *d = document();
    d->viewport.zoom = 2;
    d->grid.snap_enabled = true;
    d->grid.snap_tolerance = 5;
    commit();
    tool("vector.tool_pen");
    point(65, 65);
    point(161, 65);
    key(SDLK_RETURN);
    TEST_ASSERT_EQUAL_INT(1, d->rectangle_count);
    VectorBezier c = vector_curve_parent(&d->rectangles[0]);
    TEST_ASSERT_FLOAT_WITHIN(.01f, 64, c.nodes[0].point.x);
    TEST_ASSERT_FLOAT_WITHIN(.01f, 160, c.nodes[1].point.x);
    point(80, 100);
    key(SDLK_ESCAPE);
    TEST_ASSERT_EQUAL_INT(1, d->rectangle_count);
    tool("vector.tool_pen");
    point(80, 100);
    SDL_Event e = {.type = SDL_EVENT_WINDOW_FOCUS_LOST};
    send(&e);
    TEST_ASSERT_FALSE(d->curve_tool.pen);
    TEST_ASSERT_EQUAL_INT(1, d->rectangle_count);
    TEST_ASSERT_TRUE(vector_artboard_set_state(d, 1, VECTOR_OBJECT_LOCKED));
    tool("vector.tool_pen");
    point(100, 120);
    point(200, 120);
    key(SDLK_RETURN);
    TEST_ASSERT_EQUAL_INT(1, d->rectangle_count);
}

static void test_inserting_node_preserves_cubic_and_native_double_click(void) {
    VectorBezier original = arch(), split = original;
    TEST_ASSERT_TRUE(vector_bezier_insert(&split, 0, .5f));
    TEST_ASSERT_EQUAL_UINT(3, split.count);
    for (int i = 0; i <= 20; i++) {
        float t = i / 20.f;
        VectorPoint a = vector_bezier_evaluate(original.nodes[0], original.nodes[1], t);
        VectorPoint b = t <= .5f
                            ? vector_bezier_evaluate(split.nodes[0], split.nodes[1], t * 2)
                            : vector_bezier_evaluate(split.nodes[1], split.nodes[2], (t - .5f) * 2);
        TEST_ASSERT_FLOAT_WITHIN(.0001f, a.x, b.x);
        TEST_ASSERT_FLOAT_WITHIN(.0001f, a.y, b.y);
    }
    VectorDocument *d = document();
    TEST_ASSERT_TRUE(vector_curve_add(d, 1, &original));
    commit();
    tool("vector.tool_nodes");
    int history = d->history.count;
    SDL_Event e = {.type = SDL_EVENT_MOUSE_BUTTON_DOWN};
    e.button.button = SDL_BUTTON_LEFT;
    e.button.clicks = 2;
    e.button.x = d->viewport.bounds.x + d->viewport.x + 50;
    e.button.y = d->viewport.bounds.y + d->viewport.y + 75;
    send(&e);
    e.type = SDL_EVENT_MOUSE_BUTTON_UP;
    send(&e);
    TEST_ASSERT_EQUAL_UINT(3, d->rectangles[0].curve.count);
    TEST_ASSERT_EQUAL_INT(history + 1, d->history.count);
    TEST_ASSERT_TRUE(vector_history_undo(&d->history, d));
    TEST_ASSERT_EQUAL_UINT(2, d->rectangles[0].curve.count);
}

static void test_curve_edits_do_not_replace_other_group_members(void) {
    VectorDocument *d = document();
    VectorBezier c = arch();
    TEST_ASSERT_TRUE(vector_curve_add(d, 1, &c));
    TEST_ASSERT_TRUE(vector_rectangle_add(d, 1, (PUIRect){200, 100, 40, 40}));
    vector_document_select(d, 1);
    vector_document_toggle(d, 2);
    TEST_ASSERT_TRUE(vector_objects_group(d));
    VectorShape other = *vector_rectangle_find(d, 2);
    VectorShape original = *vector_rectangle_find(d, 1);
    c.nodes[0].outgoing.y = 120;
    TEST_ASSERT_TRUE(vector_curve_update(d, 1, &c));
    TEST_ASSERT_EQUAL_MEMORY(&other, vector_rectangle_find(d, 2), sizeof other);
    TEST_ASSERT_EQUAL_UINT(original.group_id, vector_rectangle_find(d, 1)->group_id);
    TEST_ASSERT_TRUE(vector_history_undo(&d->history, d));
    TEST_ASSERT_EQUAL_MEMORY(&original, vector_rectangle_find(d, 1), sizeof original);
}

static void test_mirror_and_explicit_polygon_conversion_preserve_expected_geometry(void) {
    VectorDocument *d = document();
    VectorBezier c = arch();
    c.closed = 1;
    TEST_ASSERT_TRUE(vector_curve_add(d, 1, &c));
    TEST_ASSERT_TRUE(vector_objects_orient(d, VECTOR_MIRROR_HORIZONTAL));
    VectorBezier mirrored = vector_curve_parent(&d->rectangles[0]);
    TEST_ASSERT_EQUAL_FLOAT(100, mirrored.nodes[0].point.x);
    TEST_ASSERT_FLOAT_WITHIN(.001f, 100, mirrored.nodes[0].outgoing.x);
    TEST_ASSERT_FLOAT_WITHIN(.001f, 100, mirrored.nodes[0].outgoing.y);
    VectorPath outline;
    TEST_ASSERT_TRUE(
        vector_shape_outline(&d->rectangles[0], d->rectangles[0].rect, 0, 2, 0, &outline));
    TEST_ASSERT_TRUE(vector_paths_can_operate(d, VECTOR_PATH_CONVERT));
    TEST_ASSERT_TRUE(vector_paths_operate(d, VECTOR_PATH_CONVERT));
    TEST_ASSERT_EQUAL_UINT(0, d->rectangles[0].curve.count);
    TEST_ASSERT_TRUE(vector_path_valid(&d->rectangles[0].path));
    TEST_ASSERT_TRUE(vector_history_undo(&d->history, d));
    TEST_ASSERT_EQUAL_UINT(2, d->rectangles[0].curve.count);
    TEST_ASSERT_TRUE(vector_paths_operate(d, VECTOR_PATH_STROKE));
    TEST_ASSERT_EQUAL_UINT(0, d->rectangles[0].curve.count);
    TEST_ASSERT_EQUAL_FLOAT(0, d->rectangles[0].stroke.width);
}

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_curve_edits_do_not_replace_other_group_members);
    RUN_TEST(test_mirror_and_explicit_polygon_conversion_preserve_expected_geometry);
    RUN_TEST(test_inserting_node_preserves_cubic_and_native_double_click);
    RUN_TEST(test_exact_bounds_and_adaptive_geometry);
    RUN_TEST(test_domain_create_edit_undo_copy_and_no_rectangle_hit);
    RUN_TEST(test_native_pen_drag_open_finish_and_history);
    RUN_TEST(test_nodes_live_preview_commit_escape_alt_and_delete);
    RUN_TEST(test_closed_curve_node_delete_checkpoint_and_mirror);
    RUN_TEST(test_pen_cancel_grid_zoom_locked_parent_and_focus_loss);
    return UNITY_END();
}
