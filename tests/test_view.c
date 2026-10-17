#include "pengui/pengui.h"
#include "unity.h"
#include <stdio.h>
#include <string.h>

static SDL_Window *window;
static SDL_Renderer *renderer;
static PUILayout *layout;
static PUIView *view;
static PUIContext pui;
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
    layout = pui_layout_parse(yaml, strlen(yaml), error, sizeof error);
    TEST_ASSERT_NOT_NULL_MESSAGE(layout, error);
    view = pui_view_new(layout, renderer);
    TEST_ASSERT_NOT_NULL(view);
    pui_init(&pui);
    actions = 0;
    pui_view_draw(view, &pui, (PUIRect){0, 0, 400, 300}, action, &actions);
}

void tearDown(void) {
    pui_view_free(view);
    pui_layout_free(layout);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
}

static void test_yaml_button_dispatches_only_application_action(void) {
    pui_pointer(&pui, 20, 15, true);
    pui_pointer(&pui, 20, 15, false);
    pui_view_draw(view, &pui, (PUIRect){0, 0, 400, 300}, action, &actions);
    TEST_ASSERT_EQUAL_INT(1, actions);
}

static void test_native_text_event_replaces_selection(void) {
    SDL_Event event = {0};
    event.type = SDL_EVENT_MOUSE_BUTTON_DOWN;
    event.button.button = SDL_BUTTON_LEFT;
    event.button.x = 20;
    event.button.y = 45;
    pui_view_event(view, &pui, window, &event);
    event.type = SDL_EVENT_KEY_DOWN;
    event.key.key = SDLK_A;
    event.key.mod = SDL_KMOD_CTRL;
    TEST_ASSERT_TRUE(pui_view_event(view, &pui, window, &event));
    event.type = SDL_EVENT_TEXT_INPUT;
    event.text.text = "Updated";
    TEST_ASSERT_TRUE(pui_view_event(view, &pui, window, &event));
    TEST_ASSERT_EQUAL_STRING("Updated", pui_view_text(view, "name"));
}

static void test_two_views_have_independent_input_state(void) {
    PUIView *other = pui_view_new(layout, renderer);
    TEST_ASSERT_TRUE(pui_view_set_text(view, "name", "Changed"));
    TEST_ASSERT_EQUAL_STRING("Initial", pui_view_text(other, "name"));
    TEST_ASSERT_EQUAL_STRING("Changed", pui_view_text(view, "name"));
    pui_view_free(other);
}

static void test_invalid_input_update_is_atomic(void) {
    TEST_ASSERT_FALSE(pui_view_set_text(view, "name", "bad\nline"));
    TEST_ASSERT_EQUAL_STRING("Initial", pui_view_text(view, "name"));
    TEST_ASSERT_FALSE(pui_view_set_text(view, "button", "wrong kind"));
    TEST_ASSERT_NULL(pui_view_text(view, "missing"));
    TEST_ASSERT_NULL(pui_image_load(renderer, "missing.png"));
}

static void test_panel_paints_and_restores_renderer_clipping(void) {
    char error[256];
    const char *yaml = "schema: pengui/v1\nroot: {type: panel, text: Test panel}";
    PUILayout *panel = pui_layout_parse(yaml, strlen(yaml), error, sizeof error);
    TEST_ASSERT_NOT_NULL_MESSAGE(panel, error);
    PUIView *panel_view = pui_view_new(panel, renderer);
    pui_panel(renderer, (SDL_FRect){0, 0, 400, 300}, PUI_CANVAS);
    pui_view_draw(panel_view, &pui, (PUIRect){0, 0, 400, 300}, NULL, NULL);
    SDL_Surface *surface = SDL_RenderReadPixels(renderer, NULL);
    TEST_ASSERT_NOT_NULL(surface);
    Uint8 r, g, b, a;
    TEST_ASSERT_TRUE(SDL_ReadSurfacePixel(surface, 200, 200, &r, &g, &b, &a));
    TEST_ASSERT_EQUAL_UINT8(PUI_PANEL >> 24, r);
    TEST_ASSERT_EQUAL_UINT8((PUI_PANEL >> 16) & 255, g);
    TEST_ASSERT_EQUAL_UINT8((PUI_PANEL >> 8) & 255, b);
    TEST_ASSERT_FALSE(SDL_RenderClipEnabled(renderer));
    SDL_DestroySurface(surface);
    pui_view_free(panel_view);
    pui_layout_free(panel);
}

static void test_window_drag_excludes_buttons_and_preserves_resize_edges(void) {
    const char *yaml =
        "schema: pengui/v1\nroot:\n  type: column\n  padding: 0\n  children:\n"
        "    - type: toolbar\n      draggable: true\n      height: 44\n      children:\n"
        "        - {type: button, width: 32, icon: minimize, action: window.minimize}\n"
        "        - {type: spacer}\n"
        "    - {type: spacer}\n";
    char error[256];
    PUILayout *description = pui_layout_parse(yaml, strlen(yaml), error, sizeof error);
    TEST_ASSERT_NOT_NULL_MESSAGE(description, error);
    PUIView *window_view = pui_view_new(description, renderer);
    pui_view_draw(window_view, &pui, (PUIRect){0, 0, 400, 300}, NULL, NULL);
    PUIWindow chrome = {window, window_view, &pui};
    TEST_ASSERT_EQUAL_INT(SDL_HITTEST_DRAGGABLE, pui_window_hit_test(&chrome, 200, 20));
    TEST_ASSERT_EQUAL_INT(SDL_HITTEST_NORMAL, pui_window_hit_test(&chrome, 20, 20));
    TEST_ASSERT_EQUAL_INT(SDL_HITTEST_NORMAL, pui_window_hit_test(&chrome, 200, 100));
    TEST_ASSERT_EQUAL_INT(SDL_HITTEST_RESIZE_TOPLEFT, pui_window_hit_test(&chrome, 1, 1));
    TEST_ASSERT_EQUAL_INT(SDL_HITTEST_RESIZE_RIGHT, pui_window_hit_test(&chrome, 399, 150));
    pui.down = true;
    TEST_ASSERT_EQUAL_INT(SDL_HITTEST_NORMAL, pui_window_hit_test(&chrome, 200, 20));
    pui.down = false;
    pui_view_free(window_view);
    pui_layout_free(description);
}

static void test_close_is_a_request_for_this_window(void) {
    SDL_Event event;
    while (SDL_PollEvent(&event)) {
    }
    TEST_ASSERT_FALSE(pui_window_action(window, "demo.unrelated"));
    TEST_ASSERT_TRUE(pui_window_action(window, "window.close"));
    bool found = false;
    while (SDL_PollEvent(&event)) {
        if (event.type == SDL_EVENT_WINDOW_CLOSE_REQUESTED) {
            TEST_ASSERT_EQUAL_UINT32(SDL_GetWindowID(window), event.window.windowID);
            found = true;
        }
    }
    TEST_ASSERT_TRUE(found);
}

static void record_pane(void *context, const char *action, const char *widget) {
    (void)action;
    snprintf(context, 16, "%s", widget);
}

static void split_frame(PUIView *split, float width, char *selected) {
    pui_view_draw(split, &pui, (PUIRect){0, 0, width, 300}, record_pane, selected);
}

static void test_three_pane_grips_and_resize_keep_outer_preferences(void) {
    char error[256], selected[16] = {0};
    const char *yaml = "schema: pengui/v1\nroot: {type: split, initial: 80, initial_last: 80, "
                       "minimum_first: 40, minimum_second: 40, minimum_last: 40, gap: 8, children: "
                       "[{type: button, id: left, action: select}, {type: button, id: center, "
                       "action: select}, {type: button, id: right, action: select}]}";
    PUILayout *description = pui_layout_parse(yaml, strlen(yaml), error, sizeof error);
    TEST_ASSERT_NOT_NULL_MESSAGE(description, error);
    PUIView *split = pui_view_new(description, renderer);
    pui_init(&pui);
    split_frame(split, 400, selected);
    pui_pointer(&pui, 84, 150, true);
    split_frame(split, 400, selected);
    pui_pointer(&pui, 124, 150, true);
    split_frame(split, 400, selected);
    pui_pointer(&pui, 124, 150, false);
    split_frame(split, 400, selected);
    pui_pointer(&pui, 115, 150, true);
    pui_pointer(&pui, 115, 150, false);
    split_frame(split, 400, selected);
    TEST_ASSERT_EQUAL_STRING("left", selected);
    pui_pointer(&pui, 316, 150, true);
    split_frame(split, 400, selected);
    pui_pointer(&pui, 276, 150, true);
    split_frame(split, 400, selected);
    pui_pointer(&pui, 276, 150, false);
    split_frame(split, 400, selected);
    split_frame(split, 600, selected);
    pui_pointer(&pui, 490, 150, true);
    pui_pointer(&pui, 490, 150, false);
    split_frame(split, 600, selected);
    TEST_ASSERT_EQUAL_STRING("right", selected);
    split_frame(split, 400, selected);
    pui_pointer(&pui, 290, 150, true);
    pui_pointer(&pui, 290, 150, false);
    split_frame(split, 400, selected);
    TEST_ASSERT_EQUAL_STRING("right", selected);
    pui_view_free(split);
    pui_layout_free(description);
}

static void assert_pixel(float x, float y, Uint32 color) {
    SDL_Surface *surface = SDL_RenderReadPixels(renderer, NULL);
    TEST_ASSERT_NOT_NULL(surface);
    Uint8 r, g, b, a;
    TEST_ASSERT_TRUE(SDL_ReadSurfacePixel(surface, (int)x, (int)y, &r, &g, &b, &a));
    TEST_ASSERT_EQUAL_UINT8(color >> 24, r);
    TEST_ASSERT_EQUAL_UINT8((color >> 16) & 255, g);
    TEST_ASSERT_EQUAL_UINT8((color >> 8) & 255, b);
    SDL_DestroySurface(surface);
}

static void test_tabs_join_content_without_background_gap(void) {
    char error[256];
    const char *yaml = "schema: pengui/v1\nroot: {type: tabs, children: [{type: tab, "
                       "text: Tab, padding: 0, children: [{type: panel}]}]}";
    PUILayout *description = pui_layout_parse(yaml, strlen(yaml), error, sizeof error);
    TEST_ASSERT_NOT_NULL_MESSAGE(description, error);
    PUIView *tabs = pui_view_new(description, renderer);
    pui_panel(renderer, (SDL_FRect){0, 0, 400, 300}, PUI_CANVAS);
    pui_view_draw(tabs, &pui, (PUIRect){0, 0, 400, 300}, NULL, NULL);
    assert_pixel(10, 29, PUI_PANEL);
    assert_pixel(10, 30, PUI_PANEL);
    assert_pixel(10, 31, PUI_PANEL);
    pui_view_free(tabs);
    pui_layout_free(description);
}

static void test_tree_selection_persists_and_moves_without_actions(void) {
    char error[256];
    const char *yaml = "schema: pengui/v1\nroot: {type: tree, children: ["
                       "{type: item, text: First}, {type: item, text: Second}, "
                       "{type: item, text: Disabled, enabled: false}]}";
    PUILayout *description = pui_layout_parse(yaml, strlen(yaml), error, sizeof error);
    TEST_ASSERT_NOT_NULL_MESSAGE(description, error);
    PUIView *tree = pui_view_new(description, renderer);
    pui_init(&pui);
    pui_pointer(&pui, 2, 14, true);
    pui_pointer(&pui, 2, 14, false);
    pui_view_draw(tree, &pui, (PUIRect){0, 0, 400, 300}, NULL, NULL);
    assert_pixel(1, 14, PUI_ACCENT);
    pui_pointer(&pui, 390, 250, false);
    pui_view_draw(tree, &pui, (PUIRect){0, 0, 400, 300}, NULL, NULL);
    assert_pixel(1, 14, PUI_ACCENT);
    pui_pointer(&pui, 20, 42, true);
    pui_pointer(&pui, 20, 42, false);
    pui_view_draw(tree, &pui, (PUIRect){0, 0, 400, 300}, NULL, NULL);
    assert_pixel(1, 14, PUI_HOVER);
    assert_pixel(1, 42, PUI_ACCENT);
    pui_pointer(&pui, 20, 70, true);
    pui_pointer(&pui, 20, 70, false);
    pui_view_draw(tree, &pui, (PUIRect){0, 0, 400, 300}, NULL, NULL);
    assert_pixel(1, 42, PUI_ACCENT);
    assert_pixel(1, 70, PUI_HOVER);
    pui_view_free(tree);
    pui_layout_free(description);
}

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_tabs_join_content_without_background_gap);
    RUN_TEST(test_tree_selection_persists_and_moves_without_actions);
    RUN_TEST(test_three_pane_grips_and_resize_keep_outer_preferences);
    RUN_TEST(test_yaml_button_dispatches_only_application_action);
    RUN_TEST(test_native_text_event_replaces_selection);
    RUN_TEST(test_two_views_have_independent_input_state);
    RUN_TEST(test_invalid_input_update_is_atomic);
    RUN_TEST(test_panel_paints_and_restores_renderer_clipping);
    RUN_TEST(test_window_drag_excludes_buttons_and_preserves_resize_edges);
    RUN_TEST(test_close_is_a_request_for_this_window);
    return UNITY_END();
}
