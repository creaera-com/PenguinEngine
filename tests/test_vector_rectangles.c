#include "editor_host/host.h"
#include "editors/vector/artboard/tool.h"
#include "editors/vector/internal.h"
#include "editors/vector/rectangle/tool.h"
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

static void assert_local_red(VectorDocument *doc, float x, float y, Uint8 expected) {
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
    TEST_ASSERT_EQUAL_UINT8(expected, r);
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

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_empty_click_focus_loss_and_editor_switch_discard_tool_preview);
    RUN_TEST(test_draw_rectangle_in_parent_local_coordinates_and_undo);
    RUN_TEST(test_cancellation_reverse_drag_clamping_and_parent_guards);
    RUN_TEST(test_child_follows_parent_and_uses_own_and_inherited_states);
    RUN_TEST(test_rectangle_history_and_parent_links_survive_editor_switch);
    RUN_TEST(test_capacity_and_multiple_parents_preserve_stable_identity);
    return UNITY_END();
}
