#include "pengui/view.h"
#include "unity.h"
#include <string.h>

static SDL_Window *window;
static SDL_Renderer *renderer;
static UiLayout *layout;
static UiView *view;
static UiContext ui;
static int actions;

static void action(void *context, const char *name, const char *widget) {
    TEST_ASSERT_EQUAL_PTR(&actions, context);
    TEST_ASSERT_EQUAL_STRING("test.click", name);
    TEST_ASSERT_EQUAL_STRING("button", widget);
    actions++;
}

void setUp(void) {
    TEST_ASSERT_TRUE(SDL_Init(SDL_INIT_VIDEO));
    window = SDL_CreateWindow("test", 400, 300, SDL_WINDOW_HIDDEN);
    renderer = SDL_CreateRenderer(window, NULL);
    TEST_ASSERT_NOT_NULL(renderer);
    char error[256];
    const char *yaml = "schema: pengui/v1\nroot:\n  type: column\n  padding: 0\n  children:\n"
                       "    - {type: button, id: button, text: Click, action: test.click}\n"
                       "    - {type: input, id: name, text: Initial}\n"
                       "    - {type: label, id: output, text: Ready}\n";
    layout = ui_layout_parse(yaml, strlen(yaml), error, sizeof error);
    TEST_ASSERT_NOT_NULL_MESSAGE(layout, error);
    view = ui_view_new(layout, renderer);
    TEST_ASSERT_NOT_NULL(view);
    ui_init(&ui);
    actions = 0;
    ui_view_draw(view, &ui, (UiRect){0, 0, 400, 300}, action, &actions);
}

void tearDown(void) {
    ui_view_free(view);
    ui_layout_free(layout);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
}

static void test_yaml_button_dispatches_only_application_action(void) {
    ui_pointer(&ui, 20, 15, true);
    ui_pointer(&ui, 20, 15, false);
    ui_view_draw(view, &ui, (UiRect){0, 0, 400, 300}, action, &actions);
    TEST_ASSERT_EQUAL_INT(1, actions);
}

static void test_native_text_event_replaces_selection(void) {
    SDL_Event event = {0};
    event.type = SDL_EVENT_MOUSE_BUTTON_DOWN;
    event.button.button = SDL_BUTTON_LEFT;
    event.button.x = 20;
    event.button.y = 45;
    ui_view_event(view, &ui, window, &event);
    event.type = SDL_EVENT_KEY_DOWN;
    event.key.key = SDLK_A;
    event.key.mod = SDL_KMOD_CTRL;
    TEST_ASSERT_TRUE(ui_view_event(view, &ui, window, &event));
    event.type = SDL_EVENT_TEXT_INPUT;
    event.text.text = "Updated";
    TEST_ASSERT_TRUE(ui_view_event(view, &ui, window, &event));
    TEST_ASSERT_EQUAL_STRING("Updated", ui_view_text(view, "name"));
}

static void test_two_views_have_independent_input_state(void) {
    UiView *other = ui_view_new(layout, renderer);
    TEST_ASSERT_TRUE(ui_view_set_text(view, "name", "Changed"));
    TEST_ASSERT_EQUAL_STRING("Initial", ui_view_text(other, "name"));
    TEST_ASSERT_EQUAL_STRING("Changed", ui_view_text(view, "name"));
    ui_view_free(other);
}

static void test_invalid_input_update_is_atomic(void) {
    TEST_ASSERT_FALSE(ui_view_set_text(view, "name", "bad\nline"));
    TEST_ASSERT_EQUAL_STRING("Initial", ui_view_text(view, "name"));
    TEST_ASSERT_FALSE(ui_view_set_text(view, "button", "wrong kind"));
    TEST_ASSERT_NULL(ui_view_text(view, "missing"));
    TEST_ASSERT_NULL(ui_image_load(renderer, "missing.png"));
}

static void test_panel_paints_and_restores_renderer_clipping(void) {
    char error[256];
    const char *yaml = "schema: pengui/v1\nroot: {type: panel, text: Test panel}";
    UiLayout *panel = ui_layout_parse(yaml, strlen(yaml), error, sizeof error);
    TEST_ASSERT_NOT_NULL_MESSAGE(panel, error);
    UiView *panel_view = ui_view_new(panel, renderer);
    ui_panel(renderer, (SDL_FRect){0, 0, 400, 300}, UI_CANVAS);
    ui_view_draw(panel_view, &ui, (UiRect){0, 0, 400, 300}, NULL, NULL);
    SDL_Surface *surface = SDL_RenderReadPixels(renderer, NULL);
    TEST_ASSERT_NOT_NULL(surface);
    Uint8 r, g, b, a;
    TEST_ASSERT_TRUE(SDL_ReadSurfacePixel(surface, 200, 200, &r, &g, &b, &a));
    TEST_ASSERT_EQUAL_UINT8(UI_PANEL >> 24, r);
    TEST_ASSERT_EQUAL_UINT8((UI_PANEL >> 16) & 255, g);
    TEST_ASSERT_EQUAL_UINT8((UI_PANEL >> 8) & 255, b);
    TEST_ASSERT_FALSE(SDL_RenderClipEnabled(renderer));
    SDL_DestroySurface(surface);
    ui_view_free(panel_view);
    ui_layout_free(panel);
}

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_yaml_button_dispatches_only_application_action);
    RUN_TEST(test_native_text_event_replaces_selection);
    RUN_TEST(test_two_views_have_independent_input_state);
    RUN_TEST(test_invalid_input_update_is_atomic);
    RUN_TEST(test_panel_paints_and_restores_renderer_clipping);
    return UNITY_END();
}
