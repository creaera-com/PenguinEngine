/** @file @brief Numeric scrubbing renders isolated values and commits a single history command. */
#include "editor_host/host.h"
#include "editors/vector/internal.h"
#include "shell/shell.h"
#include "unity.h"
static SDL_Window *window;
static SDL_Renderer *renderer;
static EditorHost host;
static EditorShell shell;

static void action(void *context, const char *name, const char *widget) {
    TEST_ASSERT_TRUE(editor_host_action(context, shell.view, name, widget));
}

static void frame(void) {
    char error[256];
    TEST_ASSERT_TRUE_MESSAGE(
        editor_host_commit(&host, shell.view, &shell.input, error, sizeof error), error);
    pui_view_draw(shell.view, &shell.input, (PUIRect){0, 0, 1280, 800}, action, &host);
}

static VectorDocument *document(void) {
    return vector_document_tabs_active(&((VectorEditor *)host.instance)->documents, shell.view);
}

static VectorDocument *preview(void) {
    return vector_inspector_preview_document(&((VectorEditor *)host.instance)->inspector,
                                             document());
}

void setUp(void) {
    TEST_ASSERT_TRUE(SDL_Init(SDL_INIT_VIDEO));
    window = SDL_CreateWindow("numeric preview", 1280, 800, SDL_WINDOW_HIDDEN);
    renderer = SDL_CreateRenderer(window, NULL);
    TEST_ASSERT_NOT_NULL(renderer);
    char file[1600], error[256];
    snprintf(file, sizeof file, "%s/shell.yaml", PENGUI_EDITOR_RESOURCES);
    TEST_ASSERT_TRUE_MESSAGE(editor_shell_load(&shell, renderer, file, error, sizeof error), error);
    editor_host_init(&host, (EditorServices){PENGUI_EDITOR_RESOURCES});
    TEST_ASSERT_TRUE(editor_host_request(&host, "vector", false));
    frame();
    frame();
    TEST_ASSERT_TRUE(vector_rectangle_add(document(), 1, (PUIRect){20, 20, 80, 60}));
    frame();
}

void tearDown(void) {
    editor_shell_destroy(&shell);
    editor_host_destroy(&host);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
}

static SDL_FPoint field(const char *id) {
    for (int y = 0; y < 800; y += 2) {
        for (int x = 0; x < 1280; x += 2) {
            if (pui_view_hit(shell.view, id, x, y)) {
                return (SDL_FPoint){x + 4, y + 4};
            }
        }
    }
    TEST_FAIL_MESSAGE("Inspector field is not visible");
    return (SDL_FPoint){0};
}

static void pointer(Uint32 type, SDL_FPoint p) {
    SDL_Event e = {.type = type};
    if (type == SDL_EVENT_MOUSE_MOTION) {
        e.motion.x = p.x;
        e.motion.y = p.y;
    } else {
        e.button.x = p.x;
        e.button.y = p.y;
        e.button.button = SDL_BUTTON_LEFT;
        e.button.clicks = 1;
    }
    pui_view_event(shell.view, &shell.input, window, &e);
    frame();
}

static SDL_FPoint scrub(const char *id) {
    SDL_FPoint p = field(id);
    pointer(SDL_EVENT_MOUSE_BUTTON_DOWN, p);
    SDL_Delay(550);
    frame();
    p.x += 60;
    pointer(SDL_EVENT_MOUSE_MOTION, p);
    return p;
}

static void test_live_width_is_isolated_then_commits_once(void) {
    VectorDocument *d = document();
    int count = d->history.count;
    SDL_FPoint p = scrub("rectangle_width");
    TEST_ASSERT_NOT_EQUAL(d, preview());
    TEST_ASSERT_EQUAL_FLOAT(90, preview()->rectangles[0].rect.w);
    TEST_ASSERT_EQUAL_FLOAT(80, d->rectangles[0].rect.w);
    TEST_ASSERT_EQUAL_INT(count, d->history.count);
    /* This pixel lies outside the original width but inside the rendered preview. */
    float x = d->viewport.bounds.x + d->viewport.x +
              (d->artboards[0].rect.x + d->rectangles[0].rect.x + 84) * d->viewport.zoom;
    float y = d->viewport.bounds.y + d->viewport.y +
              (d->artboards[0].rect.y + d->rectangles[0].rect.y + 24) * d->viewport.zoom;
    SDL_Surface *surface = SDL_RenderReadPixels(renderer, NULL);
    TEST_ASSERT_NOT_NULL(surface);
    Uint8 r, g, b, a;
    TEST_ASSERT_TRUE(SDL_ReadSurfacePixel(surface, (int)x, (int)y, &r, &g, &b, &a));
    TEST_ASSERT_EQUAL_UINT8(d->rectangles[0].fill >> 24, r);
    TEST_ASSERT_EQUAL_UINT8((d->rectangles[0].fill >> 16) & 255, g);
    TEST_ASSERT_EQUAL_UINT8((d->rectangles[0].fill >> 8) & 255, b);
    SDL_DestroySurface(surface);
    p.x += 30;
    pointer(SDL_EVENT_MOUSE_MOTION, p);
    TEST_ASSERT_EQUAL_FLOAT(95, preview()->rectangles[0].rect.w);
    TEST_ASSERT_EQUAL_INT(count, d->history.count);
    pointer(SDL_EVENT_MOUSE_BUTTON_UP, p);
    TEST_ASSERT_EQUAL_FLOAT(95, d->rectangles[0].rect.w);
    TEST_ASSERT_EQUAL_INT(count + 1, d->history.count);
    frame();
    TEST_ASSERT_EQUAL_PTR(d, preview());
    TEST_ASSERT_TRUE(vector_history_undo(&d->history, d));
    TEST_ASSERT_EQUAL_FLOAT(80, d->rectangles[0].rect.w);
    TEST_ASSERT_TRUE(vector_history_redo(&d->history, d));
    TEST_ASSERT_EQUAL_FLOAT(95, d->rectangles[0].rect.w);
}

static void test_escape_and_focus_loss_discard_preview(void) {
    VectorDocument *d = document();
    int count = d->history.count;
    scrub("rectangle_height");
    TEST_ASSERT_EQUAL_FLOAT(70, preview()->rectangles[0].rect.h);
    SDL_Event e = {.type = SDL_EVENT_KEY_DOWN};
    e.key.key = SDLK_ESCAPE;
    TEST_ASSERT_TRUE(pui_view_event(shell.view, &shell.input, window, &e));
    frame();
    TEST_ASSERT_EQUAL_PTR(d, preview());
    TEST_ASSERT_EQUAL_FLOAT(60, d->rectangles[0].rect.h);
    TEST_ASSERT_EQUAL_INT(count, d->history.count);
    scrub("rectangle_width");
    e.type = SDL_EVENT_WINDOW_FOCUS_LOST;
    pui_view_event(shell.view, &shell.input, window, &e);
    frame();
    TEST_ASSERT_EQUAL_PTR(d, preview());
    TEST_ASSERT_EQUAL_FLOAT(80, d->rectangles[0].rect.w);
    TEST_ASSERT_EQUAL_INT(count, d->history.count);
}

static void test_artboard_width_preview_commits_once(void) {
    VectorDocument *d = document();
    d->active_rectangle = 0;
    d->selection.count = 0;
    frame();
    float before = d->artboards[0].rect.w;
    int history = d->history.count;
    SDL_FPoint p = scrub("artboard_width");
    TEST_ASSERT_EQUAL_FLOAT(before + 10, preview()->artboards[0].rect.w);
    TEST_ASSERT_EQUAL_FLOAT(before, d->artboards[0].rect.w);
    pointer(SDL_EVENT_MOUSE_BUTTON_UP, p);
    TEST_ASSERT_EQUAL_FLOAT(before + 10, d->artboards[0].rect.w);
    TEST_ASSERT_EQUAL_INT(history + 1, d->history.count);
    TEST_ASSERT_TRUE(vector_history_undo(&d->history, d));
    frame();
    TEST_ASSERT_EQUAL_FLOAT(before, d->artboards[0].rect.w);
}

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_live_width_is_isolated_then_commits_once);
    RUN_TEST(test_escape_and_focus_loss_discard_preview);
    RUN_TEST(test_artboard_width_preview_commits_once);
    return UNITY_END();
}
