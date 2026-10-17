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
static char last_action[64], last_value[128];

static const char *description =
    "schema: pengui/v1\nroot: {type: column, padding: 0, gap: 0, children: ["
    "{type: dropdown, id: mode, action: choose, popup_width: 180, children: ["
    "{type: option, text: Alpha, value: a}, {type: option, text: Beta, value: b}, "
    "{type: option, text: Disabled, value: c, enabled: false}, "
    "{type: option, text: Delta, value: d}]}, "
    "{type: button, id: behind, action: behind}, "
    "{type: popup_button, id: tools, text: Tools, popup_width: 220, popup_height: 180, children: ["
    "{type: panel, padding: 8, gap: 4, children: ["
    "{type: input, id: popup_name, text: Original}, "
    "{type: button, id: apply, text: Apply, action: apply}, "
    "{type: dropdown, id: nested, action: nested, popup_width: 160, children: ["
    "{type: option, text: One, value: one}, {type: option, text: Two, value: two}]}, "
    "{type: popup_button, id: more, text: More, placement: right, popup_width: 160, "
    "popup_height: 80, children: [{type: panel, padding: 4, children: ["
    "{type: button, id: extra, text: Extra, action: extra}]}]}]}]}, "
    "{type: input, id: base_name, text: Base}, {type: spacer}]}";

static void action(void *context, const char *name, const char *widget) {
    TEST_ASSERT_EQUAL_PTR(&actions, context);
    actions++;
    snprintf(last_action, sizeof last_action, "%s", name);
    const char *value = pui_view_value(view, widget);
    snprintf(last_value, sizeof last_value, "%s", value ? value : "");
}

static void frame(void) {
    pui_view_draw(view, &pui, (PUIRect){0, 0, 400, 300}, action, &actions);
}

static void click(float x, float y) {
    pui_pointer(&pui, x, y, true);
    pui_pointer(&pui, x, y, false);
    frame();
}

static bool key(SDL_Keycode code) {
    SDL_Event event = {0};
    event.type = SDL_EVENT_KEY_DOWN;
    event.key.key = code;
    return pui_view_event(view, &pui, window, &event);
}

void setUp(void) {
    TEST_ASSERT_TRUE(SDL_Init(SDL_INIT_VIDEO));
    window = SDL_CreateWindow("popups", 400, 300, SDL_WINDOW_HIDDEN);
    renderer = SDL_CreateRenderer(window, NULL);
    TEST_ASSERT_NOT_NULL(renderer);
    char error[256];
    layout = pui_layout_parse(description, strlen(description), error, sizeof error);
    TEST_ASSERT_NOT_NULL_MESSAGE(layout, error);
    view = pui_view_new(layout, renderer);
    TEST_ASSERT_NOT_NULL(view);
    pui_init(&pui);
    actions = 0;
    last_action[0] = last_value[0] = 0;
    frame();
}

void tearDown(void) {
    pui_view_free(view);
    pui_layout_free(layout);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
}

static void test_dropdown_dispatch_contains_new_value_and_setter_is_silent(void) {
    TEST_ASSERT_EQUAL_STRING("a", pui_view_value(view, "mode"));
    click(30, 15);
    TEST_ASSERT_EQUAL_INT(0, actions);
    click(30, 79);
    TEST_ASSERT_EQUAL_INT(1, actions);
    TEST_ASSERT_EQUAL_STRING("choose", last_action);
    TEST_ASSERT_EQUAL_STRING("b", last_value);
    TEST_ASSERT_TRUE(pui_view_set_value(view, "mode", "d"));
    TEST_ASSERT_FALSE(pui_view_set_value(view, "mode", "c"));
    TEST_ASSERT_FALSE(pui_view_set_value(view, "mode", "missing"));
    TEST_ASSERT_EQUAL_STRING("d", pui_view_value(view, "mode"));
    TEST_ASSERT_NULL(pui_view_value(view, "behind"));
    TEST_ASSERT_NULL(pui_view_value(NULL, "mode"));
    TEST_ASSERT_EQUAL_INT(1, actions);
}

static void test_outside_click_is_consumed_and_same_selection_does_not_fire(void) {
    click(30, 15);
    click(300, 45); /* Outside list, above the underlying button. */
    TEST_ASSERT_EQUAL_INT(0, actions);
    click(300, 45);
    TEST_ASSERT_EQUAL_INT(1, actions);
    TEST_ASSERT_EQUAL_STRING("behind", last_action);
    click(30, 15);
    click(30, 50); /* Choose the already active Alpha option. */
    TEST_ASSERT_EQUAL_INT(1, actions);
    click(300, 45);
    TEST_ASSERT_EQUAL_INT(2, actions);
}

static void test_disabled_options_and_keyboard_selection(void) {
    click(30, 15);
    click(30, 107); /* Disabled option does not close the list. */
    TEST_ASSERT_EQUAL_INT(0, actions);
    TEST_ASSERT_TRUE(key(SDLK_END));
    frame();
    TEST_ASSERT_TRUE(key(SDLK_RETURN));
    frame();
    TEST_ASSERT_EQUAL_STRING("d", pui_view_value(view, "mode"));
    TEST_ASSERT_EQUAL_INT(1, actions);
    click(30, 15);
    TEST_ASSERT_TRUE(key(SDLK_UP)); /* Skip the disabled option, move Delta -> Beta. */
    frame();
    TEST_ASSERT_TRUE(key(SDLK_RETURN));
    frame();
    TEST_ASSERT_EQUAL_STRING("b", pui_view_value(view, "mode"));
    TEST_ASSERT_EQUAL_INT(2, actions);
}

static void test_escape_closes_without_changing_value(void) {
    click(30, 15);
    TEST_ASSERT_TRUE(key(SDLK_DOWN));
    TEST_ASSERT_TRUE(key(SDLK_ESCAPE));
    frame();
    TEST_ASSERT_EQUAL_STRING("a", pui_view_value(view, "mode"));
    TEST_ASSERT_EQUAL_INT(0, actions);
    click(300, 45);
    TEST_ASSERT_EQUAL_INT(1, actions);
}

static void test_panel_children_accept_text_and_actions_without_click_through(void) {
    click(30, 75);
    SDL_Event event = {0};
    event.type = SDL_EVENT_MOUSE_BUTTON_DOWN;
    event.button.button = SDL_BUTTON_LEFT;
    event.button.x = 30;
    event.button.y = 115;
    pui_view_event(view, &pui, window, &event);
    event.type = SDL_EVENT_MOUSE_BUTTON_UP;
    pui_view_event(view, &pui, window, &event);
    frame();
    event.type = SDL_EVENT_KEY_DOWN;
    event.key.key = SDLK_A;
    event.key.mod = SDL_KMOD_CTRL;
    TEST_ASSERT_TRUE(pui_view_event(view, &pui, window, &event));
    event.type = SDL_EVENT_TEXT_INPUT;
    event.text.text = "Changed";
    TEST_ASSERT_TRUE(pui_view_event(view, &pui, window, &event));
    TEST_ASSERT_EQUAL_STRING("Changed", pui_view_text(view, "popup_name"));
    TEST_ASSERT_EQUAL_STRING("Base", pui_view_text(view, "base_name"));
    click(30, 145);
    TEST_ASSERT_EQUAL_INT(1, actions);
    TEST_ASSERT_EQUAL_STRING("apply", last_action);
    TEST_ASSERT_TRUE(key(SDLK_ESCAPE));
    frame();
    click(300, 45);
    TEST_ASSERT_EQUAL_INT(2, actions);
    TEST_ASSERT_EQUAL_STRING("behind", last_action);
}

static void test_nested_dropdown_and_side_panel_keep_parent_open(void) {
    click(30, 75);
    click(30, 180); /* Nested dropdown at y169..199; opens below at y202. */
    click(30, 248); /* Second option. */
    TEST_ASSERT_EQUAL_STRING("two", pui_view_value(view, "nested"));
    TEST_ASSERT_EQUAL_INT(1, actions);
    TEST_ASSERT_EQUAL_STRING("nested", last_action);
    click(30, 145); /* Parent's Apply button remains usable. */
    TEST_ASSERT_EQUAL_INT(2, actions);
    click(30, 215); /* More: opens a panel on the right at x215, y203. */
    click(260, 225);
    TEST_ASSERT_EQUAL_INT(3, actions);
    TEST_ASSERT_EQUAL_STRING("extra", last_action);
    TEST_ASSERT_TRUE(key(SDLK_ESCAPE));
    frame();
    click(30, 145);
    TEST_ASSERT_EQUAL_INT(4, actions);
}

static void test_dropdown_state_is_independent_between_views(void) {
    PUIView *other = pui_view_new(layout, renderer);
    TEST_ASSERT_TRUE(pui_view_set_value(view, "mode", "b"));
    TEST_ASSERT_EQUAL_STRING("a", pui_view_value(other, "mode"));
    TEST_ASSERT_EQUAL_STRING("b", pui_view_value(view, "mode"));
    pui_view_free(other);
}

static void test_long_dropdown_scrolls_selected_option_into_view(void) {
    pui_view_free(view);
    pui_layout_free(layout);
    const char *yaml =
        "schema: pengui/v1\nroot: {type: column, padding: 0, children: ["
        "{type: dropdown, id: mode, value: seven, popup_height: 64, action: choose, children: ["
        "{type: option, text: Zero, value: zero}, {type: option, text: One, value: one}, "
        "{type: option, text: Two, value: two}, {type: option, text: Three, value: three}, "
        "{type: option, text: Four, value: four}, {type: option, text: Five, value: five}, "
        "{type: option, text: Six, value: six}, {type: option, text: Seven, value: seven}]}, "
        "{type: spacer}]}";
    char error[256];
    layout = pui_layout_parse(yaml, strlen(yaml), error, sizeof error);
    TEST_ASSERT_NOT_NULL_MESSAGE(layout, error);
    view = pui_view_new(layout, renderer);
    pui_init(&pui);
    frame();
    click(30, 15);
    click(30, 50); /* Initial Seven selection reveals the final two rows: Six and Seven. */
    TEST_ASSERT_EQUAL_STRING("six", pui_view_value(view, "mode"));
    TEST_ASSERT_EQUAL_INT(1, actions);
    click(30, 15);
    TEST_ASSERT_TRUE(key(SDLK_HOME));
    frame();
    TEST_ASSERT_TRUE(key(SDLK_RETURN));
    frame();
    TEST_ASSERT_EQUAL_STRING("zero", pui_view_value(view, "mode"));
    TEST_ASSERT_EQUAL_INT(2, actions);
}

static void test_focus_loss_dismisses_popup(void) {
    click(30, 75);
    SDL_Event event = {0};
    event.type = SDL_EVENT_WINDOW_FOCUS_LOST;
    pui_view_event(view, &pui, window, &event);
    frame();
    click(300, 45);
    TEST_ASSERT_EQUAL_INT(1, actions);
    TEST_ASSERT_EQUAL_STRING("behind", last_action);
}

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_long_dropdown_scrolls_selected_option_into_view);
    RUN_TEST(test_focus_loss_dismisses_popup);
    RUN_TEST(test_dropdown_dispatch_contains_new_value_and_setter_is_silent);
    RUN_TEST(test_outside_click_is_consumed_and_same_selection_does_not_fire);
    RUN_TEST(test_disabled_options_and_keyboard_selection);
    RUN_TEST(test_escape_closes_without_changing_value);
    RUN_TEST(test_panel_children_accept_text_and_actions_without_click_through);
    RUN_TEST(test_nested_dropdown_and_side_panel_keep_parent_open);
    RUN_TEST(test_dropdown_state_is_independent_between_views);
    return UNITY_END();
}
