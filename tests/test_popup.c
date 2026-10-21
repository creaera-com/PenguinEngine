#include "pengui/pengui.h"
#include "unity.h"
#include <math.h>
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

/* Native clicks exercise modal routing and the deferred navigation during drawing. */
static void radial_click_item(const char *id) {
    SDL_Event event = {.type = SDL_EVENT_MOUSE_BUTTON_DOWN};
    event.button.button = SDL_BUTTON_LEFT;
    TEST_ASSERT_TRUE(pui_view_radial_item_center(view, id, &event.button.x, &event.button.y));
    TEST_ASSERT_TRUE(pui_view_event(view, &pui, window, &event));
    frame();
    event.type = SDL_EVENT_MOUSE_BUTTON_UP;
    TEST_ASSERT_TRUE(pui_view_event(view, &pui, window, &event));
    frame();
}

static void radial_fixture(float x, float y) {
    const char *yaml = "schema: pengui/v1\nroot: {type: column, children: ["
                       "{type: radial_menu, id: root, children: ["
                       "{type: button, id: copy, text: Copy, action: copy}, "
                       "{type: radial_menu, id: transform, text: Transform, children: ["
                       "{type: button, id: flip, text: Flip, action: flip}, "
                       "{type: radial_menu, id: align, text: Align, children: ["
                       "{type: button, id: left, text: Left, action: left}, "
                       "{type: button, id: right, text: Right, action: right}]}]}, "
                       "{type: radial_menu, id: layers, text: Layers, children: ["
                       "{type: button, id: front, text: Front, action: front}, "
                       "{type: button, id: back, text: Back, action: back}]}]}, "
                       "{type: button, id: behind, action: behind}]}";
    pui_view_free(view);
    pui_layout_free(layout);
    char error[256];
    layout = pui_layout_parse(yaml, strlen(yaml), error, sizeof error);
    TEST_ASSERT_NOT_NULL_MESSAGE(layout, error);
    view = pui_view_new(layout, renderer);
    frame();
    TEST_ASSERT_TRUE(pui_view_open_radial(view, "root", x, y));
    frame();
}

static bool radial_visible(const char *id) {
    float x, y;
    return pui_view_radial_item_center(view, id, &x, &y);
}

static void test_concentric_rings_keep_ancestors_interactive_and_replace_only_the_branch(void) {
    radial_fixture(200, 150);
    TEST_ASSERT_FALSE(radial_visible("flip"));
    radial_click_item("transform");
    radial_click_item("align");
    TEST_ASSERT_TRUE(radial_visible("copy"));
    TEST_ASSERT_TRUE(radial_visible("flip"));
    TEST_ASSERT_TRUE(radial_visible("left"));
    TEST_ASSERT_EQUAL_INT(0, actions);
    /* Click a root sibling while three rings are open. */
    radial_click_item("layers");
    TEST_ASSERT_TRUE(radial_visible("copy"));
    TEST_ASSERT_TRUE(radial_visible("front"));
    TEST_ASSERT_FALSE(radial_visible("flip"));
    TEST_ASSERT_FALSE(radial_visible("left"));
    radial_click_item("front");
    TEST_ASSERT_EQUAL_INT(1, actions);
    TEST_ASSERT_EQUAL_STRING("front", last_action);
    TEST_ASSERT_FALSE(radial_visible("copy"));
}

static void test_escape_closes_all_rings_and_root_actions_remain_available(void) {
    radial_fixture(200, 150);
    radial_click_item("transform");
    radial_click_item("align");
    TEST_ASSERT_TRUE(key(SDLK_ESCAPE));
    frame();
    TEST_ASSERT_FALSE(radial_visible("left"));
    TEST_ASSERT_FALSE(radial_visible("flip"));
    TEST_ASSERT_FALSE(radial_visible("copy"));
    radial_fixture(200, 150);
    radial_click_item("transform");
    radial_click_item("align");
    radial_click_item("copy");
    TEST_ASSERT_EQUAL_STRING("copy", last_action);
    TEST_ASSERT_EQUAL_INT(1, actions);
}

static void test_outer_rings_stay_inside_small_view_and_center_does_not_jump(void) {
    radial_fixture(399, 299);
    float before_x, before_y, after_x, after_y;
    TEST_ASSERT_TRUE(pui_view_radial_item_center(view, "copy", &before_x, &before_y));
    radial_click_item("transform");
    radial_click_item("align");
    TEST_ASSERT_TRUE(pui_view_radial_item_center(view, "copy", &after_x, &after_y));
    TEST_ASSERT_EQUAL_FLOAT(before_x, after_x);
    TEST_ASSERT_EQUAL_FLOAT(before_y, after_y);
    TEST_ASSERT_TRUE(pui_view_radial_item_center(view, "left", &after_x, &after_y));
    TEST_ASSERT_TRUE(after_x >= 0 && after_x <= 400 && after_y >= 0 && after_y <= 300);
    radial_click_item("left");
    TEST_ASSERT_EQUAL_STRING("left", last_action);
}

static void test_disabled_inner_action_never_dispatches_and_empty_center_closes_menu(void) {
    radial_fixture(200, 150);
    radial_click_item("transform");
    radial_click_item("align");
    float x, y, sibling_x, sibling_y;
    /* Top of the root ring identifies its center without relying on pixel constants. */
    TEST_ASSERT_TRUE(pui_view_radial_item_center(view, "copy", &x, &y));
    SDL_Event e = {.type = SDL_EVENT_MOUSE_BUTTON_DOWN};
    e.button.button = SDL_BUTTON_LEFT;
    TEST_ASSERT_TRUE(pui_view_radial_item_center(view, "layers", &sibling_x, &sibling_y));
    e.button.x = x;
    e.button.y = (y + 2 * sibling_y) / 3;
    TEST_ASSERT_TRUE(pui_view_event(view, &pui, window, &e));
    e.type = SDL_EVENT_MOUSE_BUTTON_UP;
    TEST_ASSERT_TRUE(pui_view_event(view, &pui, window, &e));
    frame();
    TEST_ASSERT_FALSE(radial_visible("left"));
    TEST_ASSERT_FALSE(radial_visible("flip"));
    radial_fixture(200, 150);
    TEST_ASSERT_TRUE(pui_view_set_radial_enabled(view, "copy", false));
    radial_click_item("copy");
    TEST_ASSERT_EQUAL_INT(0, actions);
    TEST_ASSERT_FALSE(radial_visible("copy"));
}

static void test_parallel_radial_gaps_and_rounded_corners_are_not_clickable(void) {
    for (int corner = 0; corner < 2; corner++) {
        radial_fixture(200, 150);
        float x[3], y[3];
        const char *ids[] = {"copy", "transform", "layers"};
        for (int i = 0; i < 3; i++) {
            TEST_ASSERT_TRUE(pui_view_radial_item_center(view, ids[i], &x[i], &y[i]));
        }
        float cx = (x[0] + x[1] + x[2]) / 3, cy = (y[0] + y[1] + y[2]) / 3;
        float middle = hypotf(x[0] - cx, y[0] - cy);
        float radius = corner ? middle * 42 / 75 + .1f : middle;
        float angle = -1.570796327f + 3.141592654f / 3;
        if (corner) {
            angle -= asinf(2 / radius) + .001f;
        }
        SDL_Event e = {.type = SDL_EVENT_MOUSE_BUTTON_DOWN};
        e.button.button = SDL_BUTTON_LEFT;
        e.button.x = cx + cosf(angle) * radius;
        e.button.y = cy + sinf(angle) * radius;
        TEST_ASSERT_TRUE(pui_view_event(view, &pui, window, &e));
        e.type = SDL_EVENT_MOUSE_BUTTON_UP;
        TEST_ASSERT_TRUE(pui_view_event(view, &pui, window, &e));
        frame();
        TEST_ASSERT_EQUAL_INT(0, actions);
        TEST_ASSERT_FALSE(radial_visible("copy"));
    }
}

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_parallel_radial_gaps_and_rounded_corners_are_not_clickable);
    RUN_TEST(test_disabled_inner_action_never_dispatches_and_empty_center_closes_menu);
    RUN_TEST(test_concentric_rings_keep_ancestors_interactive_and_replace_only_the_branch);
    RUN_TEST(test_escape_closes_all_rings_and_root_actions_remain_available);
    RUN_TEST(test_outer_rings_stay_inside_small_view_and_center_does_not_jump);
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
