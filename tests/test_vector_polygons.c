#include "editor_host/host.h"
#include "editors/vector/curve/curve.h"
#include "editors/vector/document/selection.h"
#include "editors/vector/ellipse/render.h"
#include "editors/vector/internal.h"
#include "editors/vector/object/clipboard.h"
#include "editors/vector/path/operations.h"
#include "editors/vector/polygon/polygon.h"
#include "editors/vector/workspace/box_tool.h"
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
    VectorViewport *v = &document()->viewport;
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

static void key(SDL_Keycode code, SDL_Keymod modifiers) {
    SDL_SetModState(modifiers);
    SDL_Event e = {.type = SDL_EVENT_KEY_DOWN};
    e.key.key = code;
    e.key.mod = modifiers;
    send(&e);
}

static void test_polygon_parameters_conversion_and_history(void) {
    VectorDocument *d = document();
    TEST_ASSERT_TRUE(vector_polygon_add(d, 1, (PUIRect){50, 50, 200, 150}, 6));
    VectorShape before = d->rectangles[0];
    TEST_ASSERT_EQUAL_UINT(VECTOR_CURVE_POLYGON, before.curve_kind);
    TEST_ASSERT_EQUAL_UINT(6, before.curve.count);
    TEST_ASSERT_TRUE(vector_shape_contains(&before, 150, 125));
    TEST_ASSERT_FALSE(vector_shape_contains(&before, 51, 51));
    commit();
    TEST_ASSERT_EQUAL_STRING("Polygon", pui_view_text(shell.view, "rectangle_kind"));
    TEST_ASSERT_EQUAL_STRING("6", pui_view_text(shell.view, "polygon_sides"));
    // The optional slot must reserve a visible, interactive row above the large sections.
    TEST_ASSERT_TRUE(pui_view_hit(shell.view, "polygon_sides", 1150, 130));
    pui_view_set_text(shell.view, "polygon_sides", "3");
    tool("vector.polygon.sides");
    TEST_ASSERT_EQUAL_UINT(3, d->rectangles[0].curve.count);
    TEST_ASSERT_EQUAL_STRING(before.name, d->rectangles[0].name);
    TEST_ASSERT_EQUAL_MEMORY(&before.rect, &d->rectangles[0].rect, sizeof before.rect);
    TEST_ASSERT_TRUE(vector_history_undo(&d->history, d));
    TEST_ASSERT_EQUAL_MEMORY(&before, &d->rectangles[0], sizeof before);
    TEST_ASSERT_TRUE(vector_history_redo(&d->history, d));
    TEST_ASSERT_TRUE(vector_paths_operate(d, VECTOR_PATH_CONVERT));
    commit();
    TEST_ASSERT_EQUAL_UINT(VECTOR_CURVE_FREE, d->rectangles[0].curve_kind);
    TEST_ASSERT_EQUAL_UINT(0, d->rectangles[0].curve.count);
    TEST_ASSERT_NULL(pui_view_text(shell.view, "polygon_sides"));
    TEST_ASSERT_FALSE(vector_polygon_set_sides(d, 1, 8));
    TEST_ASSERT_TRUE(vector_history_undo(&d->history, d));
    commit();
    TEST_ASSERT_EQUAL_STRING("3", pui_view_text(shell.view, "polygon_sides"));
    TEST_ASSERT_FALSE(vector_polygon_set_sides(d, 1, 2));
    TEST_ASSERT_FALSE(vector_polygon_set_sides(d, 1, VECTOR_POLYGON_MAX_SIDES + 1));
    TEST_ASSERT_TRUE(vector_polygon_set_sides(d, 1, VECTOR_POLYGON_MAX_SIDES));
    TEST_ASSERT_TRUE(vector_rectangle_set_state(d, 1, VECTOR_OBJECT_LOCKED));
    TEST_ASSERT_FALSE(vector_polygon_set_sides(d, 1, 5));
}

static void test_polygon_native_gesture_preview_modifiers_and_switch(void) {
    VectorDocument *d = document();
    tool("vector.add_polygon");
    TEST_ASSERT_TRUE(pui_view_checked(shell.view, "vector_add_polygon"));
    TEST_ASSERT_FALSE(pui_view_checked(shell.view, "vector_add_rect"));
    int history = d->history.count;
    pointer(SDL_EVENT_MOUSE_BUTTON_DOWN, 160, 160);
    key(SDLK_LSHIFT, SDL_KMOD_SHIFT | SDL_KMOD_ALT);
    pointer(SDL_EVENT_MOUSE_MOTION, 200, 180);
    TEST_ASSERT_EQUAL_INT(0, d->rectangle_count);
    TEST_ASSERT_EQUAL_INT(history, d->history.count);
    pointer(SDL_EVENT_MOUSE_BUTTON_UP, 200, 180);
    TEST_ASSERT_EQUAL_INT(1, d->rectangle_count);
    TEST_ASSERT_EQUAL_UINT(6, d->rectangles[0].curve.count);
    TEST_ASSERT_EQUAL_FLOAT(d->rectangles[0].rect.w, d->rectangles[0].rect.h);
    TEST_ASSERT_EQUAL_FLOAT(160, d->rectangles[0].rect.x + d->rectangles[0].rect.w / 2);
    TEST_ASSERT_TRUE(d->rectangle_tool.enabled);
    key(SDLK_LSHIFT, 0);
    tool("vector.add_rect");
    TEST_ASSERT_FALSE(pui_view_checked(shell.view, "vector_add_polygon"));
    tool("vector.add_polygon");
    pointer(SDL_EVENT_MOUSE_BUTTON_DOWN, 20, 20);
    key(SDLK_ESCAPE, 0);
    TEST_ASSERT_EQUAL_INT(1, d->rectangle_count);
    TEST_ASSERT_EQUAL_INT(history + 1, d->history.count);
    TEST_ASSERT_FALSE(d->rectangle_tool.enabled);
}

static void test_polygon_checkpoint_and_style(void) {
    VectorDocument *d = document();
    TEST_ASSERT_TRUE(vector_polygon_add(d, 1, (PUIRect){50, 50, 200, 150}, 5));
    TEST_ASSERT_TRUE(
        vector_rectangle_set_stroke(d, 1, (VectorStroke){0xff8000ff, 4, VECTOR_STROKE_INSIDE}));
    TEST_ASSERT_TRUE(vector_polygon_set_sides(d, 1, 8));
    VectorShape saved = d->rectangles[0];
    activate("scripts");
    activate("vector");
    d = document();
    TEST_ASSERT_EQUAL_MEMORY(&saved, &d->rectangles[0], sizeof saved);
    TEST_ASSERT_TRUE(vector_history_undo(&d->history, d));
    TEST_ASSERT_EQUAL_UINT(5, d->rectangles[0].curve.count);
    TEST_ASSERT_EQUAL_UINT(saved.stroke.color, d->rectangles[0].stroke.color);
    TEST_ASSERT_TRUE(vector_history_redo(&d->history, d));
    TEST_ASSERT_EQUAL_MEMORY(&saved, &d->rectangles[0], sizeof saved);
    TEST_ASSERT_FALSE(vector_polygon_add(d, 1, (PUIRect){0, 0, NAN, 20}, 6));
}

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_polygon_parameters_conversion_and_history);
    RUN_TEST(test_polygon_native_gesture_preview_modifiers_and_switch);
    RUN_TEST(test_polygon_checkpoint_and_style);
    return UNITY_END();
}
