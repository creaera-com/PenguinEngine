#include "editor_host/host.h"
#include "editors/vector/artboard/tool.h"
#include "editors/vector/geometry/resize.h"
#include "editors/vector/internal.h"
#include "editors/vector/rectangle/tool.h"
#include "editors/vector/workspace/selection.h"
#include "shell/shell.h"
#include "unity.h"
#include <string.h>

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

static void activate(const char *id) {
    TEST_ASSERT_TRUE(editor_host_request(&host, id, false));
    commit();
}

static VectorDocument *document(void) {
    VectorEditor *editor = host.instance;
    return vector_document_tabs_active(&editor->documents, shell.view);
}

static void action(const char *name, const char *widget) {
    TEST_ASSERT_TRUE(editor_host_action(&host, shell.view, name, widget));
    commit();
}

void setUp(void) {
    TEST_ASSERT_TRUE(SDL_Init(SDL_INIT_VIDEO));
    window = SDL_CreateWindow("rectangles", 1280, 800, SDL_WINDOW_HIDDEN);
    renderer = SDL_CreateRenderer(window, NULL);
    TEST_ASSERT_NOT_NULL(renderer);
    char path[1600], error[256];
    snprintf(path, sizeof path, "%s/shell.yaml", PENGUI_EDITOR_RESOURCES);
    TEST_ASSERT_TRUE_MESSAGE(editor_shell_load(&shell, renderer, path, error, sizeof error), error);
    editor_host_init(&host, (EditorServices){PENGUI_EDITOR_RESOURCES});
    activate("vector");
    commit();
}

void tearDown(void) {
    editor_shell_destroy(&shell);
    editor_host_destroy(&host);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
}

static VectorDocument *parent(void) {
    VectorDocument *doc = document();
    TEST_ASSERT_TRUE(vector_artboard_add(doc, (PUIRect){10, 20, 200, 120}));
    commit();
    return doc;
}

static void pointer(SDL_EventType type, float x, float y) {
    SDL_Event event = {.type = type};
    if (type == SDL_EVENT_MOUSE_MOTION) {
        event.motion.x = x;
        event.motion.y = y;
    } else {
        event.button.button = SDL_BUTTON_LEFT;
        event.button.x = x;
        event.button.y = y;
    }
    TEST_ASSERT_TRUE(editor_host_event(&host, shell.view, &event));
    frame();
}

static void local_pointer(VectorDocument *doc, SDL_EventType type, float x, float y) {
    VectorViewport *v = &doc->viewport;
    PUIRect board = doc->artboards[0].rect;
    pointer(type, v->bounds.x + v->x + (board.x + x) * v->zoom,
            v->bounds.y + v->y + (board.y + y) * v->zoom);
}

static void assert_rect(PUIRect expected, PUIRect actual) {
    TEST_ASSERT_EQUAL_FLOAT(expected.x, actual.x);
    TEST_ASSERT_EQUAL_FLOAT(expected.y, actual.y);
    TEST_ASSERT_EQUAL_FLOAT(expected.w, actual.w);
    TEST_ASSERT_EQUAL_FLOAT(expected.h, actual.h);
}

static Uint8 local_red(VectorDocument *doc, float x, float y) {
    VectorViewport *v = &doc->viewport;
    PUIRect board = doc->artboards[0].rect;
    SDL_Surface *surface = SDL_RenderReadPixels(renderer, NULL);
    TEST_ASSERT_NOT_NULL(surface);
    Uint8 r, g, b, a;
    bool ok =
        SDL_ReadSurfacePixel(surface, (int)(v->bounds.x + v->x + (board.x + x) * v->zoom),
                             (int)(v->bounds.y + v->y + (board.y + y) * v->zoom), &r, &g, &b, &a);
    SDL_DestroySurface(surface);
    TEST_ASSERT_TRUE(ok);
    return r;
}

static void assert_local_red(VectorDocument *doc, float x, float y, Uint8 expected) {
    TEST_ASSERT_EQUAL_UINT8(expected, local_red(doc, x, y));
}

static void test_draw_rectangle_in_parent_local_coordinates_and_undo(void) {
    VectorDocument *doc = parent();
    doc->viewport.zoom = 2;
    commit();
    action("vector.add_rect", "");
    TEST_ASSERT_TRUE(pui_view_checked(shell.view, "vector_add_rect"));
    TEST_ASSERT_FALSE(pui_view_checked(shell.view, "vector_selection"));
    action("vector.add_rect", "");
    TEST_ASSERT_FALSE(doc->rectangle_tool.enabled);
    action("vector.add_rect", "");
    local_pointer(doc, SDL_EVENT_MOUSE_BUTTON_DOWN, 20, 30);
    local_pointer(doc, SDL_EVENT_MOUSE_MOTION, 120, 100);
    TEST_ASSERT_EQUAL_INT(0, doc->rectangle_count);
    TEST_ASSERT_EQUAL_INT(1, doc->history.count);
    local_pointer(doc, SDL_EVENT_MOUSE_BUTTON_UP, 120, 100);
    commit();
    TEST_ASSERT_EQUAL_INT(1, doc->rectangle_count);
    TEST_ASSERT_EQUAL_INT(2, doc->history.count);
    TEST_ASSERT_EQUAL_UINT(1, doc->rectangles[0].parent_id);
    assert_rect((PUIRect){20, 30, 100, 70}, doc->rectangles[0].rect);
    TEST_ASSERT_EQUAL_UINT(1, doc->active_rectangle);
    TEST_ASSERT_FALSE(doc->rectangle_tool.enabled);
    TEST_ASSERT_TRUE(pui_view_checked(shell.view, "vector_selection"));
    TEST_ASSERT_EQUAL_INT(2, pui_view_tree_item_count(shell.view, "vector_document_tree"));
    TEST_ASSERT_EQUAL_STRING("Rechteck 1", pui_view_text(shell.view, "rectangle_name"));
    assert_local_red(doc, 40, 50, 115);
    action("editor.undo", "");
    TEST_ASSERT_EQUAL_INT(0, doc->rectangle_count);
    TEST_ASSERT_EQUAL_INT(1, pui_view_tree_item_count(shell.view, "vector_document_tree"));
    action("editor.redo", "");
    TEST_ASSERT_EQUAL_INT(1, doc->rectangle_count);
    assert_rect((PUIRect){20, 30, 100, 70}, doc->rectangles[0].rect);
}

static void test_cancellation_reverse_drag_clamping_and_parent_guards(void) {
    VectorDocument *doc = parent();
    TEST_ASSERT_FALSE(vector_rectangle_add(doc, 999, (PUIRect){0, 0, 10, 10}));
    TEST_ASSERT_FALSE(vector_rectangle_add(doc, 1, (PUIRect){190, 0, 20, 10}));
    TEST_ASSERT_TRUE(vector_artboard_set_state(doc, 1, VECTOR_OBJECT_LOCKED));
    TEST_ASSERT_FALSE(vector_rectangle_add(doc, 1, (PUIRect){0, 0, 10, 10}));
    TEST_ASSERT_TRUE(vector_artboard_set_state(doc, 1, VECTOR_OBJECT_HIDDEN));
    TEST_ASSERT_FALSE(vector_rectangle_add(doc, 1, (PUIRect){0, 0, 10, 10}));
    TEST_ASSERT_TRUE(vector_artboard_set_state(doc, 1, 0));
    commit();
    int count = doc->history.count;
    action("vector.add_rect", "");
    local_pointer(doc, SDL_EVENT_MOUSE_BUTTON_DOWN, 20, 20);
    local_pointer(doc, SDL_EVENT_MOUSE_MOTION, 80, 70);
    SDL_Event key = {.type = SDL_EVENT_KEY_DOWN};
    key.key.key = SDLK_ESCAPE;
    TEST_ASSERT_TRUE(editor_host_event(&host, shell.view, &key));
    TEST_ASSERT_FALSE(doc->rectangle_tool.dragging);
    TEST_ASSERT_EQUAL_INT(count, doc->history.count);
    TEST_ASSERT_EQUAL_INT(0, doc->rectangle_count);
    action("vector.add_rect", "");
    local_pointer(doc, SDL_EVENT_MOUSE_BUTTON_DOWN, 20, 20);
    action("vector.tool_selection", "");
    TEST_ASSERT_FALSE(doc->rectangle_tool.enabled);
    action("vector.add_rect", "");
    local_pointer(doc, SDL_EVENT_MOUSE_BUTTON_DOWN, 180, 90);
    local_pointer(doc, SDL_EVENT_MOUSE_BUTTON_UP, -40, -20);
    commit();
    assert_rect((PUIRect){0, 0, 180, 90}, doc->rectangles[0].rect);
    TEST_ASSERT_EQUAL_INT(count + 1, doc->history.count);
}

static void test_child_follows_parent_and_uses_own_and_inherited_states(void) {
    VectorDocument *doc = parent();
    TEST_ASSERT_TRUE(vector_rectangle_add(doc, 1, (PUIRect){20, 30, 80, 60}));
    commit();
    TEST_ASSERT_TRUE(vector_artboard_move(doc, 1, 110, 40));
    commit();
    assert_rect((PUIRect){20, 30, 80, 60}, doc->rectangles[0].rect);
    assert_local_red(doc, 40, 50, 115);
    action("vector.rectangle.visibility", "vector_document_1.rectangle_1");
    TEST_ASSERT_FALSE(pui_view_checked(shell.view, "rectangle_visible"));
    assert_local_red(doc, 40, 50, 242);
    TEST_ASSERT_TRUE(pui_view_set_checked(shell.view, "rectangle_visible", true));
    action("vector.rectangle.visible", "rectangle_visible");
    TEST_ASSERT_TRUE(vector_artboard_set_state(doc, 1, VECTOR_OBJECT_HIDDEN));
    commit();
    assert_local_red(doc, 40, 50, 104);
    TEST_ASSERT_TRUE(vector_artboard_set_state(doc, 1, VECTOR_OBJECT_LOCKED));
    commit();
    TEST_ASSERT_FALSE(vector_rectangle_update(doc, 1, "Blocked", (PUIRect){20, 30, 90, 60}));
    TEST_ASSERT_TRUE(vector_artboard_set_state(doc, 1, 0));
    commit();
    action("vector.rectangle.lock", "vector_document_1.rectangle_1");
    TEST_ASSERT_TRUE(pui_view_checked(shell.view, "rectangle_locked"));
    TEST_ASSERT_FALSE(vector_rectangle_update(doc, 1, "Blocked", (PUIRect){20, 30, 90, 60}));
    action("vector.rectangle.lock", "vector_document_1.rectangle_1");
    TEST_ASSERT_TRUE(pui_view_set_text(shell.view, "rectangle_name", "Button"));
    action("vector.rectangle.name", "rectangle_name");
    TEST_ASSERT_EQUAL_STRING("Button", doc->rectangles[0].name);
    action("vector.artboard.select", "vector_document_1.artboard_1");
    TEST_ASSERT_EQUAL_UINT(0, doc->active_rectangle);
    TEST_ASSERT_NOT_NULL(pui_view_text(shell.view, "artboard_name"));
    local_pointer(doc, SDL_EVENT_MOUSE_BUTTON_DOWN, 40, 50);
    commit();
    TEST_ASSERT_EQUAL_UINT(1, doc->active_rectangle);
    TEST_ASSERT_NOT_NULL(pui_view_text(shell.view, "rectangle_name"));
}

static void test_rectangle_history_and_parent_links_survive_editor_switch(void) {
    VectorDocument *doc = parent();
    TEST_ASSERT_TRUE(vector_rectangle_add(doc, 1, (PUIRect){20, 30, 80, 60}));
    TEST_ASSERT_TRUE(vector_rectangle_update(doc, 1, "Card", (PUIRect){20, 30, 100, 70}));
    TEST_ASSERT_TRUE(vector_rectangle_set_state(doc, 1, VECTOR_OBJECT_LOCKED));
    TEST_ASSERT_TRUE(vector_history_undo(&doc->history, doc));
    TEST_ASSERT_TRUE(vector_history_undo(&doc->history, doc));
    commit();
    activate("scripts");
    activate("vector");
    commit();
    doc = document();
    TEST_ASSERT_EQUAL_INT(1, doc->rectangle_count);
    TEST_ASSERT_EQUAL_UINT(1, doc->rectangles[0].parent_id);
    TEST_ASSERT_EQUAL_STRING("Rechteck 1", doc->rectangles[0].name);
    TEST_ASSERT_TRUE(vector_history_redo(&doc->history, doc));
    TEST_ASSERT_TRUE(vector_history_redo(&doc->history, doc));
    commit();
    TEST_ASSERT_EQUAL_STRING("Card", pui_view_text(shell.view, "rectangle_name"));
    TEST_ASSERT_TRUE(pui_view_checked(shell.view, "rectangle_locked"));
    TEST_ASSERT_EQUAL_FLOAT(100, doc->rectangles[0].rect.w);
    action("editor.undo", "");
    action("editor.undo", "");
    action("editor.undo", "");
    TEST_ASSERT_EQUAL_INT(0, doc->rectangle_count);
    activate("game");
    activate("vector");
    commit();
    doc = document();
    action("editor.redo", "");
    TEST_ASSERT_EQUAL_INT(1, doc->rectangle_count);
    TEST_ASSERT_EQUAL_UINT(1, doc->rectangles[0].parent_id);
}

static void test_capacity_and_multiple_parents_preserve_stable_identity(void) {
    VectorDocument *doc = parent();
    TEST_ASSERT_TRUE(vector_artboard_add(doc, (PUIRect){250, 20, 200, 120}));
    for (int i = 0; i < VECTOR_MAX_RECTANGLES; i++) {
        TEST_ASSERT_TRUE(vector_rectangle_add(doc, (unsigned)(i % 2) + 1, (PUIRect){1, 2, 10, 20}));
    }
    unsigned next = doc->next_rectangle_id;
    TEST_ASSERT_FALSE(vector_rectangle_add(doc, 1, (PUIRect){1, 2, 10, 20}));
    TEST_ASSERT_EQUAL_UINT(next, doc->next_rectangle_id);
    commit();
    TEST_ASSERT_EQUAL_INT(VECTOR_MAX_RECTANGLES + 2,
                          pui_view_tree_item_count(shell.view, "vector_document_tree"));
    TEST_ASSERT_TRUE(vector_history_undo(&doc->history, doc));
    TEST_ASSERT_TRUE(vector_rectangle_add(doc, 1, (PUIRect){1, 2, 10, 20}));
    TEST_ASSERT_EQUAL_UINT(next, doc->rectangles[doc->rectangle_count - 1].id);
    TEST_ASSERT_FALSE(vector_history_redo(&doc->history, doc));
}

static void test_empty_click_focus_loss_and_editor_switch_discard_tool_preview(void) {
    VectorDocument *doc = parent();
    action("vector.add_rect", "");
    local_pointer(doc, SDL_EVENT_MOUSE_BUTTON_DOWN, 20, 20);
    local_pointer(doc, SDL_EVENT_MOUSE_BUTTON_UP, 20, 20);
    TEST_ASSERT_EQUAL_INT(0, doc->rectangle_count);
    TEST_ASSERT_EQUAL_INT(1, doc->history.count);
    TEST_ASSERT_TRUE(doc->rectangle_tool.enabled);
    local_pointer(doc, SDL_EVENT_MOUSE_BUTTON_DOWN, 20, 20);
    local_pointer(doc, SDL_EVENT_MOUSE_MOTION, 80, 80);
    SDL_Event lost = {.type = SDL_EVENT_WINDOW_FOCUS_LOST};
    doc->viewport.panning = true;
    TEST_ASSERT_TRUE(editor_host_event(&host, shell.view, &lost));
    TEST_ASSERT_FALSE(doc->rectangle_tool.dragging);
    TEST_ASSERT_FALSE(doc->viewport.panning);
    TEST_ASSERT_EQUAL_INT(0, doc->rectangle_count);
    action("vector.add_rect", "");
    local_pointer(doc, SDL_EVENT_MOUSE_BUTTON_DOWN, 20, 20);
    local_pointer(doc, SDL_EVENT_MOUSE_MOTION, 80, 80);
    activate("scripts");
    activate("vector");
    commit();
    doc = document();
    TEST_ASSERT_FALSE(doc->rectangle_tool.enabled);
    TEST_ASSERT_FALSE(doc->rectangle_tool.dragging);
    TEST_ASSERT_EQUAL_INT(0, doc->rectangle_count);
    TEST_ASSERT_EQUAL_INT(1, doc->history.count);
}

static void test_rectangle_all_eight_handles_preview_and_single_history_commit(void) {
    VectorDocument *doc = parent();
    PUIRect initial = {40, 30, 100, 60};
    TEST_ASSERT_TRUE(vector_rectangle_add(doc, 1, initial));
    doc->viewport.zoom = 2;
    commit();
    const PUIRect expected[8] = {{60, 40, 80, 50},  {40, 40, 100, 50}, {40, 40, 120, 50},
                                 {40, 30, 120, 60}, {40, 30, 120, 70}, {40, 30, 100, 70},
                                 {60, 30, 80, 70},  {60, 30, 80, 60}};
    for (int i = 0; i < 8; i++) {
        PUIRect bounds;
        TEST_ASSERT_TRUE(vector_selection_bounds(doc, &bounds));
        PUIRect h = vector_resize_handle_rect(bounds, i, 7);
        float x = h.x + 3.5f, y = h.y + 3.5f;
        pointer(SDL_EVENT_MOUSE_BUTTON_DOWN, x, y);
        TEST_ASSERT_EQUAL_UINT(1, doc->selection_tool.rectangle_id);
        TEST_ASSERT_EQUAL_UINT(vector_resize_handle_edges(i), doc->selection_tool.resize_edges);
        pointer(SDL_EVENT_MOUSE_MOTION, x + 40, y + 20);
        assert_rect(initial, doc->rectangles[0].rect);
        assert_rect(expected[i], vector_rectangle_display_rect(doc, &doc->rectangles[0]));
        assert_rect((PUIRect){10, 20, 200, 120}, doc->artboards[0].rect);
        TEST_ASSERT_EQUAL_INT(2, doc->history.cursor);
        pointer(SDL_EVENT_MOUSE_BUTTON_UP, x + 40, y + 20);
        commit();
        assert_rect(expected[i], doc->rectangles[0].rect);
        TEST_ASSERT_EQUAL_INT(3, doc->history.cursor);
        TEST_ASSERT_EQUAL_INT(3, doc->history.count);
        TEST_ASSERT_TRUE(vector_history_undo(&doc->history, doc));
        assert_rect(initial, doc->rectangles[0].rect);
        TEST_ASSERT_TRUE(vector_history_redo(&doc->history, doc));
        assert_rect(expected[i], doc->rectangles[0].rect);
        TEST_ASSERT_TRUE(vector_history_undo(&doc->history, doc));
        commit();
    }
}

static void test_rectangle_live_fill_minimum_escape_and_body_movement(void) {
    VectorDocument *doc = parent();
    PUIRect initial = {40, 30, 100, 60};
    TEST_ASSERT_TRUE(vector_rectangle_add(doc, 1, initial));
    commit();
    local_pointer(doc, SDL_EVENT_MOUSE_BUTTON_DOWN, 140, 90);
    local_pointer(doc, SDL_EVENT_MOUSE_MOTION, 160, 100);
    assert_local_red(doc, 150, 95, 115); // Added rectangle area fills before release.
    TEST_ASSERT_EQUAL_INT(2, doc->history.count);
    assert_rect(initial, doc->rectangles[0].rect);
    local_pointer(doc, SDL_EVENT_MOUSE_MOTION, 120, 70);
    assert_local_red(doc, 130, 80, 242); // Shrinking immediately uncovers the artboard.
    SDL_Event escape = {.type = SDL_EVENT_KEY_DOWN};
    escape.key.key = SDLK_ESCAPE;
    TEST_ASSERT_TRUE(editor_host_event(&host, shell.view, &escape));
    frame();
    assert_rect(initial, doc->rectangles[0].rect);
    TEST_ASSERT_EQUAL_INT(2, doc->history.count);
    local_pointer(doc, SDL_EVENT_MOUSE_BUTTON_DOWN, 40, 30);
    local_pointer(doc, SDL_EVENT_MOUSE_MOTION, 540, 530);
    assert_rect((PUIRect){139, 89, 1, 1}, vector_rectangle_display_rect(doc, &doc->rectangles[0]));
    TEST_ASSERT_TRUE(editor_host_event(&host, shell.view, &escape));
    frame();
    local_pointer(doc, SDL_EVENT_MOUSE_BUTTON_DOWN, 60, 50);
    TEST_ASSERT_EQUAL_UINT(0, doc->selection_tool.resize_edges);
    local_pointer(doc, SDL_EVENT_MOUSE_MOTION, 80, 60);
    assert_rect(initial, doc->rectangles[0].rect);
    local_pointer(doc, SDL_EVENT_MOUSE_BUTTON_UP, 80, 60);
    commit();
    assert_rect((PUIRect){60, 40, 100, 60}, doc->rectangles[0].rect);
    TEST_ASSERT_EQUAL_INT(3, doc->history.count);
    TEST_ASSERT_EQUAL_UINT(1, doc->rectangles[0].parent_id);
    TEST_ASSERT_TRUE(vector_history_undo(&doc->history, doc));
    commit();
    local_pointer(doc, SDL_EVENT_MOUSE_BUTTON_DOWN, 60, 50);
    local_pointer(doc, SDL_EVENT_MOUSE_BUTTON_UP, 300, 50);
    commit();
    assert_rect((PUIRect){280, 30, 100, 60}, doc->rectangles[0].rect);
    // Selected bounds stay available outside the artboard, allowing the child to be brought back.
    local_pointer(doc, SDL_EVENT_MOUSE_BUTTON_DOWN, 300, 50);
    local_pointer(doc, SDL_EVENT_MOUSE_BUTTON_UP, 60, 50);
    commit();
    assert_rect(initial, doc->rectangles[0].rect);
}

static void test_rectangle_mouse_respects_locks_hiding_and_cancellation(void) {
    VectorDocument *doc = parent();
    PUIRect initial = {40, 30, 100, 60};
    TEST_ASSERT_TRUE(vector_rectangle_add(doc, 1, initial));
    commit();
    unsigned states[2] = {VECTOR_OBJECT_LOCKED, VECTOR_OBJECT_HIDDEN};
    for (int i = 0; i < 2; i++) {
        TEST_ASSERT_TRUE(vector_rectangle_set_state(doc, 1, states[i]));
        commit();
        PUIRect bounds;
        TEST_ASSERT_FALSE(vector_selection_bounds(doc, &bounds));
        int count = doc->history.count;
        local_pointer(doc, SDL_EVENT_MOUSE_BUTTON_DOWN, 40, 30);
        TEST_ASSERT_FALSE(doc->selection_tool.dragging);
        TEST_ASSERT_EQUAL_INT(count, doc->history.count);
        TEST_ASSERT_TRUE(vector_rectangle_set_state(doc, 1, 0));
        action("vector.rectangle.select", "vector_document_1.rectangle_1");
        TEST_ASSERT_TRUE(vector_artboard_set_state(doc, 1, states[i]));
        commit();
        TEST_ASSERT_FALSE(vector_selection_bounds(doc, &bounds));
        local_pointer(doc, SDL_EVENT_MOUSE_BUTTON_DOWN, 40, 30);
        TEST_ASSERT_FALSE(doc->selection_tool.dragging);
        TEST_ASSERT_TRUE(vector_artboard_set_state(doc, 1, 0));
        action("vector.rectangle.select", "vector_document_1.rectangle_1");
    }
    int count = doc->history.count;
    local_pointer(doc, SDL_EVENT_MOUSE_BUTTON_DOWN, 140, 90);
    local_pointer(doc, SDL_EVENT_MOUSE_MOTION, 160, 100);
    SDL_Event lost = {.type = SDL_EVENT_WINDOW_FOCUS_LOST};
    TEST_ASSERT_TRUE(editor_host_event(&host, shell.view, &lost));
    frame();
    TEST_ASSERT_FALSE(doc->selection_tool.dragging);
    assert_rect(initial, doc->rectangles[0].rect);
    local_pointer(doc, SDL_EVENT_MOUSE_BUTTON_DOWN, 140, 90);
    local_pointer(doc, SDL_EVENT_MOUSE_MOTION, 160, 100);
    action("vector.add_rect", "");
    TEST_ASSERT_FALSE(doc->selection_tool.dragging);
    assert_rect(initial, doc->rectangles[0].rect);
    TEST_ASSERT_EQUAL_INT(count, doc->history.count);
}

static void test_rectangle_mouse_geometry_redo_and_inspector_survive_switch(void) {
    VectorDocument *doc = parent();
    TEST_ASSERT_TRUE(vector_rectangle_add(doc, 1, (PUIRect){40, 30, 100, 60}));
    commit();
    local_pointer(doc, SDL_EVENT_MOUSE_BUTTON_DOWN, 140, 90);
    local_pointer(doc, SDL_EVENT_MOUSE_BUTTON_UP, 160, 100);
    commit();
    TEST_ASSERT_EQUAL_STRING("120", pui_view_text(shell.view, "rectangle_width"));
    TEST_ASSERT_EQUAL_STRING("70", pui_view_text(shell.view, "rectangle_height"));
    action("editor.undo", "");
    activate("scripts");
    activate("vector");
    commit();
    doc = document();
    assert_rect((PUIRect){40, 30, 100, 60}, doc->rectangles[0].rect);
    action("editor.redo", "");
    assert_rect((PUIRect){40, 30, 120, 70}, doc->rectangles[0].rect);
    TEST_ASSERT_EQUAL_STRING("120", pui_view_text(shell.view, "rectangle_width"));
    TEST_ASSERT_EQUAL_UINT(1, doc->rectangles[0].parent_id);
}

static void color_action(void *context, const char *name, const char *widget) {
    (void)context;
    TEST_ASSERT_TRUE(editor_host_action(&host, shell.view, name, widget));
}

static void color_event(SDL_Event *event) {
    if (!pui_view_event(shell.view, &shell.input, window, event)) {
        editor_host_event(&host, shell.view, event);
    }
    pui_view_draw(shell.view, &shell.input, (PUIRect){0, 0, 1280, 800}, color_action, NULL);
    commit();
}

static void color_pointer(SDL_EventType type, float x, float y) {
    SDL_Event event = {.type = type};
    if (type == SDL_EVENT_MOUSE_MOTION) {
        event.motion.x = x;
        event.motion.y = y;
    } else {
        event.button.button = SDL_BUTTON_LEFT;
        event.button.x = x;
        event.button.y = y;
    }
    color_event(&event);
}

static void color_origin(float *x, float *y) {
    for (int row = 0; row < 800; row += 4) {
        for (int column = 0; column < 1280; column += 4) {
            if (pui_view_hit(shell.view, "rectangle_fill", column, row)) {
                *x = (float)column;
                *y = (float)row;
                return;
            }
        }
    }
    TEST_FAIL_MESSAGE("rectangle color picker must be visible");
}

static void test_rectangle_color_slider_preview_single_commit_and_cancel(void) {
    VectorDocument *doc = parent();
    TEST_ASSERT_TRUE(vector_rectangle_add(doc, 1, (PUIRect){20, 30, 80, 60}));
    commit();
    float x = 0, y = 0;
    color_origin(&x, &y);
    color_pointer(SDL_EVENT_MOUSE_BUTTON_DOWN, x + 20, y + 70);
    TEST_ASSERT_TRUE(doc->fill_preview.active);
    TEST_ASSERT_EQUAL_INT(2, doc->history.count);
    TEST_ASSERT_EQUAL_HEX32(VECTOR_RECTANGLE_DEFAULT_FILL, doc->rectangles[0].fill);
    uint32_t preview;
    TEST_ASSERT_TRUE(pui_view_color(shell.view, "rectangle_fill", &preview));
    assert_local_red(doc, 40, 50, preview >> 24);
    color_pointer(SDL_EVENT_MOUSE_MOTION, x + 110, y + 70);
    TEST_ASSERT_EQUAL_INT(2, doc->history.count);
    color_pointer(SDL_EVENT_MOUSE_BUTTON_UP, x + 110, y + 70);
    TEST_ASSERT_EQUAL_INT(3, doc->history.count);
    TEST_ASSERT_FALSE(doc->fill_preview.active);
    uint32_t committed = doc->rectangles[0].fill;
    TEST_ASSERT_NOT_EQUAL(VECTOR_RECTANGLE_DEFAULT_FILL, committed);
    color_pointer(SDL_EVENT_MOUSE_BUTTON_DOWN, x + 20, y + 70);
    SDL_Event escape = {.type = SDL_EVENT_KEY_DOWN};
    escape.key.key = SDLK_ESCAPE;
    color_event(&escape);
    TEST_ASSERT_FALSE(doc->fill_preview.active);
    TEST_ASSERT_EQUAL_INT(3, doc->history.count);
    TEST_ASSERT_EQUAL_HEX32(committed, doc->rectangles[0].fill);
    assert_local_red(doc, 40, 50, committed >> 24);
    action("editor.undo", "");
    TEST_ASSERT_EQUAL_HEX32(VECTOR_RECTANGLE_DEFAULT_FILL, doc->rectangles[0].fill);
    action("editor.redo", "");
    TEST_ASSERT_EQUAL_HEX32(committed, doc->rectangles[0].fill);
    int before_tool = doc->history.count;
    color_pointer(SDL_EVENT_MOUSE_BUTTON_DOWN, x + 20, y + 70);
    TEST_ASSERT_TRUE(doc->fill_preview.active);
    action("vector.add_rect", "");
    TEST_ASSERT_FALSE(pui_view_color_editing(shell.view, "rectangle_fill"));
    TEST_ASSERT_FALSE(doc->fill_preview.active);
    color_pointer(SDL_EVENT_MOUSE_BUTTON_UP, x + 20, y + 70);
    TEST_ASSERT_EQUAL_INT(before_tool, doc->history.count);
    action("vector.tool_selection", "");
    action("vector.rectangle.lock", "vector_document_1.rectangle_1");
    int count = doc->history.count;
    color_pointer(SDL_EVENT_MOUSE_BUTTON_DOWN, x + 10, y + 70);
    color_pointer(SDL_EVENT_MOUSE_BUTTON_UP, x + 10, y + 70);
    TEST_ASSERT_EQUAL_INT(count, doc->history.count);
    TEST_ASSERT_EQUAL_HEX32(committed, doc->rectangles[0].fill);
    TEST_ASSERT_TRUE(pui_view_color(shell.view, "rectangle_fill", &preview));
    TEST_ASSERT_EQUAL_HEX32(committed, preview);
}

static void test_fill_alpha_history_and_checkpoint_ignore_transient_color(void) {
    VectorDocument *doc = parent();
    TEST_ASSERT_TRUE(vector_rectangle_add(doc, 1, (PUIRect){20, 30, 80, 60}));
    TEST_ASSERT_TRUE(vector_rectangle_set_fill(doc, 1, 0xff000000));
    commit();
    assert_local_red(doc, 40, 50, 242);
    TEST_ASSERT_TRUE(vector_rectangle_set_fill(doc, 1, 0xff000080));
    commit();
    TEST_ASSERT_UINT8_WITHIN(1, 249, local_red(doc, 40, 50));
    TEST_ASSERT_TRUE(vector_rectangle_set_fill(doc, 1, 0x00ff00ff));
    TEST_ASSERT_TRUE(vector_history_undo(&doc->history, doc));
    commit();
    doc->fill_preview = (VectorFillPreview){.rectangle_id = 1, .color = 0x0000ffff, .active = true};
    activate("scripts");
    activate("vector");
    commit();
    doc = document();
    TEST_ASSERT_FALSE(doc->fill_preview.active);
    TEST_ASSERT_EQUAL_HEX32(0xff000080, doc->rectangles[0].fill);
    uint32_t rgba;
    TEST_ASSERT_TRUE(pui_view_color(shell.view, "rectangle_fill", &rgba));
    TEST_ASSERT_EQUAL_HEX32(0xff000080, rgba);
    TEST_ASSERT_TRUE(vector_history_redo(&doc->history, doc));
    commit();
    assert_local_red(doc, 40, 50, 0);
    TEST_ASSERT_TRUE(vector_artboard_set_state(doc, 1, VECTOR_OBJECT_LOCKED));
    TEST_ASSERT_FALSE(vector_rectangle_set_fill(doc, 1, 0xffffffff));
    TEST_ASSERT_EQUAL_HEX32(0x00ff00ff, doc->rectangles[0].fill);
}

static void test_rectangle_hex_field_uses_the_same_fill_history(void) {
    VectorDocument *doc = parent();
    TEST_ASSERT_TRUE(vector_rectangle_add(doc, 1, (PUIRect){20, 30, 80, 60}));
    commit();
    float x = 0, y = 0;
    color_origin(&x, &y);
    TEST_ASSERT_TRUE(pui_view_set_text(shell.view, "rectangle_fill.hex", "#FF800040"));
    color_pointer(SDL_EVENT_MOUSE_BUTTON_DOWN, x + 80, y + 244);
    color_pointer(SDL_EVENT_MOUSE_BUTTON_UP, x + 80, y + 244);
    SDL_Event enter = {.type = SDL_EVENT_KEY_DOWN};
    enter.key.key = SDLK_RETURN;
    color_event(&enter);
    TEST_ASSERT_EQUAL_HEX32(0xff800040, doc->rectangles[0].fill);
    TEST_ASSERT_EQUAL_INT(3, doc->history.count);
    color_event(&enter);
    TEST_ASSERT_EQUAL_INT(3, doc->history.count);
    TEST_ASSERT_TRUE(pui_view_set_text(shell.view, "rectangle_fill.hex", "#00FF00FF"));
    SDL_Event tab = {.type = SDL_EVENT_KEY_DOWN};
    tab.key.key = SDLK_TAB;
    color_event(&tab);
    TEST_ASSERT_EQUAL_HEX32(0x00ff00ff, doc->rectangles[0].fill);
    TEST_ASSERT_EQUAL_INT(4, doc->history.count);
    action("editor.undo", "");
    TEST_ASSERT_EQUAL_HEX32(0xff800040, doc->rectangles[0].fill);
}

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_rectangle_hex_field_uses_the_same_fill_history);
    RUN_TEST(test_rectangle_color_slider_preview_single_commit_and_cancel);
    RUN_TEST(test_fill_alpha_history_and_checkpoint_ignore_transient_color);
    RUN_TEST(test_rectangle_all_eight_handles_preview_and_single_history_commit);
    RUN_TEST(test_rectangle_live_fill_minimum_escape_and_body_movement);
    RUN_TEST(test_rectangle_mouse_respects_locks_hiding_and_cancellation);
    RUN_TEST(test_rectangle_mouse_geometry_redo_and_inspector_survive_switch);
    RUN_TEST(test_empty_click_focus_loss_and_editor_switch_discard_tool_preview);
    RUN_TEST(test_draw_rectangle_in_parent_local_coordinates_and_undo);
    RUN_TEST(test_cancellation_reverse_drag_clamping_and_parent_guards);
    RUN_TEST(test_child_follows_parent_and_uses_own_and_inherited_states);
    RUN_TEST(test_rectangle_history_and_parent_links_survive_editor_switch);
    RUN_TEST(test_capacity_and_multiple_parents_preserve_stable_identity);
    return UNITY_END();
}
