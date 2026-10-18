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
    assert_pixel(200, 14, PUI_ACCENT);
    pui_pointer(&pui, 390, 250, false);
    pui_view_draw(tree, &pui, (PUIRect){0, 0, 400, 300}, NULL, NULL);
    assert_pixel(1, 14, PUI_ACCENT);
    assert_pixel(200, 14, PUI_ACCENT);
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

static void click_control(PUIView *controls, float x, float y, char *selected) {
    pui_pointer(&pui, x, y, true);
    pui_pointer(&pui, x, y, false);
    pui_view_draw(controls, &pui, (PUIRect){0, 0, 400, 300}, record_pane, selected);
}

static void test_toggle_checkbox_and_groups_keep_independent_state(void) {
    char error[256], selected[16] = {0};
    const char *yaml = "schema: pengui/v1\nroot: {type: column, padding: 0, gap: 0, children: ["
                       "{type: toggle, id: toggle, checked: true, action: changed}, "
                       "{type: checkbox, id: checkbox, action: changed}, "
                       "{type: button_group, gap: 0, children: ["
                       "{type: button, id: a, checked: true, action: changed}, "
                       "{type: button, id: b, action: changed}, "
                       "{type: button, id: c, enabled: false, action: changed}]}, "
                       "{type: button_group, children: [{type: button, id: other}]}]}";
    PUILayout *description = pui_layout_parse(yaml, strlen(yaml), error, sizeof error);
    TEST_ASSERT_NOT_NULL_MESSAGE(description, error);
    PUIView *controls = pui_view_new(description, renderer);
    PUIView *independent = pui_view_new(description, renderer);
    pui_init(&pui);
    TEST_ASSERT_TRUE(pui_view_checked(controls, "toggle"));
    TEST_ASSERT_FALSE(pui_view_checked(controls, "checkbox"));
    TEST_ASSERT_TRUE(pui_view_checked(controls, "a"));
    TEST_ASSERT_TRUE(pui_view_checked(controls, "other"));
    click_control(controls, 20, 15, selected);
    TEST_ASSERT_EQUAL_STRING("toggle", selected);
    TEST_ASSERT_FALSE(pui_view_checked(controls, "toggle"));
    click_control(controls, 200, 45, selected);
    TEST_ASSERT_EQUAL_STRING("checkbox", selected);
    TEST_ASSERT_TRUE(pui_view_checked(controls, "checkbox"));
    click_control(controls, 200, 75, selected);
    TEST_ASSERT_EQUAL_STRING("b", selected);
    TEST_ASSERT_TRUE(pui_view_checked(controls, "b"));
    TEST_ASSERT_FALSE(pui_view_checked(controls, "a"));
    assert_pixel(100, 85, PUI_SURFACE);
    assert_pixel(200, 85, PUI_ACCENT);
    selected[0] = 0;
    click_control(controls, 200, 75, selected);
    TEST_ASSERT_EQUAL_STRING("", selected);
    click_control(controls, 350, 75, selected);
    TEST_ASSERT_EQUAL_STRING("", selected);
    TEST_ASSERT_TRUE(pui_view_checked(controls, "b"));
    TEST_ASSERT_FALSE(pui_view_checked(controls, "c"));
    TEST_ASSERT_FALSE(pui_view_set_checked(controls, "b", false));
    TEST_ASSERT_FALSE(pui_view_set_checked(controls, "c", true));
    TEST_ASSERT_TRUE(pui_view_set_checked(controls, "a", true));
    TEST_ASSERT_FALSE(pui_view_checked(controls, "b"));
    TEST_ASSERT_TRUE(pui_view_checked(controls, "other"));
    TEST_ASSERT_TRUE(pui_view_checked(independent, "toggle"));
    TEST_ASSERT_FALSE(pui_view_checked(independent, "checkbox"));
    TEST_ASSERT_TRUE(pui_view_set_checked(controls, "checkbox", false));
    TEST_ASSERT_FALSE(pui_view_checked(controls, "checkbox"));
    TEST_ASSERT_FALSE(pui_view_set_checked(controls, "missing", true));
    TEST_ASSERT_FALSE(pui_view_checked(NULL, "a"));
    pui_view_free(independent);
    pui_view_free(controls);
    pui_layout_free(description);
}

static void test_vertical_group_default_and_keyboard_selection(void) {
    char error[256], selected[16] = {0};
    const char *yaml = "schema: pengui/v1\nroot: {type: button_group, axis: vertical, "
                       "padding: 0, gap: 0, children: [{type: button, id: off, enabled: false}, "
                       "{type: button, id: first}, {type: button, id: last, action: changed}]}";
    PUILayout *description = pui_layout_parse(yaml, strlen(yaml), error, sizeof error);
    TEST_ASSERT_NOT_NULL_MESSAGE(description, error);
    PUIView *controls = pui_view_new(description, renderer);
    pui_init(&pui);
    TEST_ASSERT_TRUE(pui_view_checked(controls, "first"));
    pui_focus(&pui, pui_id("last"));
    pui_activate(&pui);
    pui_view_draw(controls, &pui, (PUIRect){0, 0, 400, 300}, record_pane, selected);
    TEST_ASSERT_EQUAL_STRING("last", selected);
    TEST_ASSERT_FALSE(pui_view_checked(controls, "first"));
    TEST_ASSERT_TRUE(pui_view_checked(controls, "last"));
    assert_pixel(200, 290, PUI_ACCENT);
    pui_view_free(controls);
    pui_layout_free(description);
}

static bool tab_key(PUIView *target, SDL_Keymod modifiers, bool repeat) {
    SDL_Event event = {0};
    event.type = SDL_EVENT_KEY_DOWN;
    event.key.key = SDLK_TAB;
    event.key.mod = modifiers;
    event.key.repeat = repeat;
    return pui_view_event(target, &pui, window, &event);
}

static void test_tab_focus_skips_hidden_disabled_and_wraps(void) {
    char error[256];
    const char *yaml =
        "schema: pengui/v1\nroot: {type: column, padding: 0, gap: 0, children: ["
        "{type: input, id: first, text: First}, {type: input, id: disabled, enabled: false}, "
        "{type: section, open: false, children: [{type: input, id: collapsed}]}, "
        "{type: panel, enabled: false, height: 30, padding: 0, children: [{type: input}]}, "
        "{type: input, id: last, text: Last}, "
        "{type: tabs, children: [{type: tab, text: Active}, "
        "{type: tab, text: Hidden, children: [{type: input, id: hidden}]}]}]}";
    PUILayout *description = pui_layout_parse(yaml, strlen(yaml), error, sizeof error);
    TEST_ASSERT_NOT_NULL_MESSAGE(description, error);
    PUIView *controls = pui_view_new(description, renderer);
    pui_init(&pui);
    pui_view_draw(controls, &pui, (PUIRect){0, 0, 400, 300}, NULL, NULL);
    TEST_ASSERT_TRUE(tab_key(controls, 0, false));
    TEST_ASSERT_EQUAL_UINT(pui_id("first"), pui.focus);
    TEST_ASSERT_TRUE(tab_key(controls, 0, true));
    TEST_ASSERT_EQUAL_UINT(pui_id("first"), pui.focus);
    TEST_ASSERT_FALSE(tab_key(controls, SDL_KMOD_CTRL, false));
    TEST_ASSERT_TRUE(tab_key(controls, 0, false));
    TEST_ASSERT_EQUAL_UINT(pui_id("last"), pui.focus);
    SDL_Event text = {0};
    text.type = SDL_EVENT_TEXT_INPUT;
    text.text.text = "Changed";
    TEST_ASSERT_TRUE(pui_view_event(controls, &pui, window, &text));
    TEST_ASSERT_EQUAL_STRING("Changed", pui_view_text(controls, "last"));
    TEST_ASSERT_EQUAL_STRING("First", pui_view_text(controls, "first"));
    TEST_ASSERT_TRUE(tab_key(controls, 0, false));
    TEST_ASSERT_EQUAL_UINT(pui_id("first"), pui.focus);
    TEST_ASSERT_TRUE(tab_key(controls, SDL_KMOD_SHIFT, false));
    TEST_ASSERT_EQUAL_UINT(pui_id("last"), pui.focus);
    TEST_ASSERT_TRUE(tab_key(controls, SDL_KMOD_SHIFT, false));
    TEST_ASSERT_EQUAL_UINT(pui_id("first"), pui.focus);
    pui_view_free(controls);
    pui_layout_free(description);
}

static void test_reverse_tab_entry_single_field_and_no_inputs(void) {
    /* Without an input focus, Shift+Tab starts at the last eligible field. */
    TEST_ASSERT_TRUE(tab_key(view, SDL_KMOD_SHIFT, false));
    TEST_ASSERT_EQUAL_UINT(pui_id("name"), pui.focus);
    TEST_ASSERT_TRUE(tab_key(view, 0, false));
    TEST_ASSERT_EQUAL_UINT(pui_id("name"), pui.focus);
    char error[256];
    const char *yaml = "schema: pengui/v1\nroot: {type: input, enabled: false}";
    PUILayout *description = pui_layout_parse(yaml, strlen(yaml), error, sizeof error);
    TEST_ASSERT_NOT_NULL_MESSAGE(description, error);
    PUIView *empty = pui_view_new(description, renderer);
    pui_init(&pui);
    pui_view_draw(empty, &pui, (PUIRect){0, 0, 400, 300}, NULL, NULL);
    TEST_ASSERT_FALSE(tab_key(empty, 0, false));
    TEST_ASSERT_EQUAL_UINT(0, pui.focus);
    pui_view_free(empty);
    pui_layout_free(description);
}

static void cursor_frame(PUIView *target, float x, float y, bool down, SDL_SystemCursor expected) {
    pui_pointer(&pui, x, y, down);
    pui_view_draw(target, &pui, (PUIRect){0, 0, 400, 300}, NULL, NULL);
    char message[128];
    snprintf(message, sizeof message, "cursor at %.0f,%.0f down=%d", x, y, down);
    TEST_ASSERT_EQUAL_INT_MESSAGE(expected, pui_view_cursor(target), message);
}

static void test_cursor_shapes_follow_hover_drag_and_window_leave(void) {
    char error[256];
    const char *yaml = "schema: pengui/v1\nroot: {type: split, initial: 120, "
                       "minimum_first: 40, minimum_second: 40, gap: 8, children: ["
                       "{type: column, padding: 0, gap: 0, children: [{type: input}, "
                       "{type: input, enabled: false}, {type: button}, {type: spacer}]}, "
                       "{type: split, axis: vertical, initial: 100, minimum_first: 40, "
                       "minimum_second: 40, gap: 8, children: [{type: panel}, {type: panel}]}]}";
    PUILayout *description = pui_layout_parse(yaml, strlen(yaml), error, sizeof error);
    TEST_ASSERT_NOT_NULL_MESSAGE(description, error);
    PUIView *controls = pui_view_new(description, renderer);
    pui_init(&pui);
    cursor_frame(controls, 20, 15, false, SDL_SYSTEM_CURSOR_TEXT);
    cursor_frame(controls, 20, 45, false, SDL_SYSTEM_CURSOR_DEFAULT);
    cursor_frame(controls, 20, 75, false, SDL_SYSTEM_CURSOR_DEFAULT);
    cursor_frame(controls, 124, 150, false, SDL_SYSTEM_CURSOR_EW_RESIZE);
    cursor_frame(controls, 200, 104, false, SDL_SYSTEM_CURSOR_NS_RESIZE);
    cursor_frame(controls, 200, 104, true, SDL_SYSTEM_CURSOR_NS_RESIZE);
    cursor_frame(controls, 20, 15, true, SDL_SYSTEM_CURSOR_NS_RESIZE);
    cursor_frame(controls, 20, 15, false, SDL_SYSTEM_CURSOR_TEXT);
    cursor_frame(controls, 124, 150, true, SDL_SYSTEM_CURSOR_EW_RESIZE);
    cursor_frame(controls, 20, 15, true, SDL_SYSTEM_CURSOR_EW_RESIZE);
    cursor_frame(controls, 20, 15, false, SDL_SYSTEM_CURSOR_TEXT);
    SDL_Event event = {0};
    event.type = SDL_EVENT_WINDOW_MOUSE_LEAVE;
    pui_view_event(controls, &pui, window, &event);
    pui_view_draw(controls, &pui, (PUIRect){0, 0, 400, 300}, NULL, NULL);
    TEST_ASSERT_EQUAL_INT(SDL_SYSTEM_CURSOR_DEFAULT, pui_view_cursor(controls));
    event.type = SDL_EVENT_WINDOW_MOUSE_ENTER;
    pui_view_event(controls, &pui, window, &event);
    cursor_frame(controls, 20, 15, false, SDL_SYSTEM_CURSOR_TEXT);
    TEST_ASSERT_EQUAL_INT(SDL_SYSTEM_CURSOR_DEFAULT, pui_view_cursor(NULL));
    pui_view_free(controls);
    pui_layout_free(description);
}

static void selection_pointer(Uint32 type, float x, float y) {
    SDL_Event event = {0};
    event.type = type;
    if (type == SDL_EVENT_MOUSE_MOTION) {
        event.motion.x = x;
        event.motion.y = y;
    } else {
        event.button.button = SDL_BUTTON_LEFT;
        event.button.x = x;
        event.button.y = y;
    }
    pui_view_event(view, &pui, window, &event);
}

static void replace_selection(const char *text) {
    SDL_Event event = {0};
    event.type = SDL_EVENT_TEXT_INPUT;
    event.text.text = text;
    TEST_ASSERT_TRUE(pui_view_event(view, &pui, window, &event));
}

static void test_mouse_selection_handles_fast_forward_and_reverse_utf8_drags(void) {
    TEST_ASSERT_TRUE(pui_view_set_text(view, "name", "abcdef"));
    selection_pointer(SDL_EVENT_MOUSE_BUTTON_DOWN, 15, 45); /* after a */
    selection_pointer(SDL_EVENT_MOUSE_MOTION, 39, 45);      /* after d */
    selection_pointer(SDL_EVENT_MOUSE_BUTTON_UP, 39, 45);
    replace_selection("X");
    TEST_ASSERT_EQUAL_STRING("aXef", pui_view_text(view, "name"));
    TEST_ASSERT_EQUAL_UINT(0, pui.active);
    TEST_ASSERT_TRUE(pui_view_set_text(view, "name", "aäöbc"));
    selection_pointer(SDL_EVENT_MOUSE_BUTTON_DOWN, 39, 45); /* after b */
    selection_pointer(SDL_EVENT_MOUSE_MOTION, 15, 45);      /* before ä */
    selection_pointer(SDL_EVENT_MOUSE_BUTTON_UP, 15, 45);
    replace_selection("!");
    TEST_ASSERT_EQUAL_STRING("a!c", pui_view_text(view, "name"));
}

static void test_mouse_selection_keeps_capture_and_does_not_activate_button_on_release(void) {
    TEST_ASSERT_TRUE(pui_view_set_text(view, "name", "abcdef"));
    selection_pointer(SDL_EVENT_MOUSE_BUTTON_DOWN, 15, 45);
    for (int i = 0; i < 3; i++) {
        pui_view_draw(view, &pui, (PUIRect){0, 0, 400, 300}, action, &actions);
        TEST_ASSERT_EQUAL_UINT(pui_id("name"), pui.active);
    }
    selection_pointer(SDL_EVENT_MOUSE_MOTION, 39, 15); /* over the button */
    pui_view_draw(view, &pui, (PUIRect){0, 0, 400, 300}, action, &actions);
    TEST_ASSERT_EQUAL_INT(SDL_SYSTEM_CURSOR_TEXT, pui_view_cursor(view));
    selection_pointer(SDL_EVENT_MOUSE_BUTTON_UP, 39, 15);
    pui_view_draw(view, &pui, (PUIRect){0, 0, 400, 300}, action, &actions);
    TEST_ASSERT_EQUAL_INT(0, actions);
    replace_selection("X");
    TEST_ASSERT_EQUAL_STRING("aXef", pui_view_text(view, "name"));
}

static void test_mouse_selection_autoscrolls_while_held_outside_field(void) {
    char text[121];
    memset(text, 'a', 120);
    text[120] = 0;
    TEST_ASSERT_TRUE(pui_view_set_text(view, "name", text));
    selection_pointer(SDL_EVENT_MOUSE_BUTTON_DOWN, 7, 45);
    selection_pointer(SDL_EVENT_MOUSE_MOTION, 420, 45);
    for (int i = 0; i < 90; i++) {
        pui_view_draw(view, &pui, (PUIRect){0, 0, 400, 300}, action, &actions);
    }
    selection_pointer(SDL_EVENT_MOUSE_BUTTON_UP, 420, 45);
    replace_selection("Selected");
    TEST_ASSERT_EQUAL_STRING("Selected", pui_view_text(view, "name"));
}

static void test_focus_loss_cancels_mouse_selection(void) {
    selection_pointer(SDL_EVENT_MOUSE_BUTTON_DOWN, 15, 45);
    TEST_ASSERT_EQUAL_UINT(pui_id("name"), pui.active);
    SDL_Event event = {0};
    event.type = SDL_EVENT_WINDOW_FOCUS_LOST;
    pui_view_event(view, &pui, window, &event);
    TEST_ASSERT_EQUAL_UINT(0, pui.active);
    event.type = SDL_EVENT_TEXT_INPUT;
    event.text.text = "Ignored";
    TEST_ASSERT_FALSE(pui_view_event(view, &pui, window, &event));
    TEST_ASSERT_EQUAL_STRING("Initial", pui_view_text(view, "name"));
}

static void multi_click_start(int clicks, float x) {
    SDL_Event event = {0};
    event.type = SDL_EVENT_MOUSE_BUTTON_DOWN;
    event.button.button = SDL_BUTTON_LEFT;
    event.button.clicks = (Uint8)clicks;
    event.button.x = x;
    event.button.y = 45;
    pui_view_event(view, &pui, window, &event);
}

static void test_double_click_selects_word_and_drag_extends_by_words(void) {
    TEST_ASSERT_TRUE(pui_view_set_text(view, "name", "one schöne Welt"));
    multi_click_start(2, 7 + 6 * PUI_FONT_WIDTH);
    pui_view_draw(view, &pui, (PUIRect){0, 0, 400, 300}, action, &actions);
    selection_pointer(SDL_EVENT_MOUSE_BUTTON_UP, 7 + 6 * PUI_FONT_WIDTH, 45);
    replace_selection("X");
    TEST_ASSERT_EQUAL_STRING("one X Welt", pui_view_text(view, "name"));
    TEST_ASSERT_TRUE(pui_view_set_text(view, "name", "one two three"));
    multi_click_start(2, 7 + 5 * PUI_FONT_WIDTH);
    selection_pointer(SDL_EVENT_MOUSE_MOTION, 7 + 10 * PUI_FONT_WIDTH, 45);
    selection_pointer(SDL_EVENT_MOUSE_BUTTON_UP, 7 + 10 * PUI_FONT_WIDTH, 45);
    replace_selection("X");
    TEST_ASSERT_EQUAL_STRING("one X", pui_view_text(view, "name"));
    TEST_ASSERT_TRUE(pui_view_set_text(view, "name", "one two three"));
    multi_click_start(2, 7 + 5 * PUI_FONT_WIDTH);
    selection_pointer(SDL_EVENT_MOUSE_MOTION, 7 + PUI_FONT_WIDTH, 45);
    selection_pointer(SDL_EVENT_MOUSE_BUTTON_UP, 7 + PUI_FONT_WIDTH, 45);
    replace_selection("X");
    TEST_ASSERT_EQUAL_STRING("X three", pui_view_text(view, "name"));
}

static void test_triple_click_and_control_or_command_a_select_all(void) {
    multi_click_start(3, 23);
    pui_view_draw(view, &pui, (PUIRect){0, 0, 400, 300}, action, &actions);
    selection_pointer(SDL_EVENT_MOUSE_BUTTON_UP, 23, 45);
    replace_selection("Triple");
    TEST_ASSERT_EQUAL_STRING("Triple", pui_view_text(view, "name"));
    SDL_Keymod modifiers[] = {SDL_KMOD_CTRL, SDL_KMOD_GUI};
    for (int i = 0; i < 2; i++) {
        TEST_ASSERT_TRUE(pui_view_set_text(view, "name", "Select all"));
        SDL_Event event = {0};
        event.type = SDL_EVENT_KEY_DOWN;
        event.key.key = SDLK_A;
        event.key.mod = modifiers[i];
        TEST_ASSERT_TRUE(pui_view_event(view, &pui, window, &event));
        replace_selection("All");
        TEST_ASSERT_EQUAL_STRING("All", pui_view_text(view, "name"));
    }
}

static void runtime_tree_action(void *context, const char *name, const char *widget) {
    TEST_ASSERT_EQUAL_STRING("tree.select", name);
    snprintf(context, 64, "%s", widget);
}

static void test_runtime_tree_hierarchy_updates_and_mounts_preserve_state(void) {
    pui_view_free(view);
    pui_layout_free(layout);
    const char *yaml = "schema: pengui/v1\nroot: {type: column, padding: 0, children: "
                       "[{type: tree, id: objects, height: 200}, {type: slot, id: body}]}";
    char error[256], selected[64] = {0};
    layout = pui_layout_parse(yaml, strlen(yaml), error, sizeof error);
    TEST_ASSERT_NOT_NULL(layout);
    view = pui_view_new(layout, renderer);
    PUITreeItem items[] = {
        {.id = "a", .text = "Artboard A", .action = "tree.select", .enabled = true, .open = true},
        {.id = "child", .text = "Group", .action = "tree.select", .depth = 1, .enabled = true},
        {.id = "b", .text = "Artboard B", .action = "tree.select", .enabled = true}};
    TEST_ASSERT_TRUE(pui_view_set_tree_items(view, "objects", items, 3));
    TEST_ASSERT_FALSE(pui_view_set_tree_items(view, "missing", items, 3));
    pui_init(&pui);
    pui_pointer(&pui, 2, 14, true);
    pui_pointer(&pui, 2, 14, false);
    pui_view_draw(view, &pui, (PUIRect){0, 0, 400, 300}, runtime_tree_action, selected);
    TEST_ASSERT_EQUAL_STRING("a", selected);
    assert_pixel(1, 14, PUI_ACCENT);
    /* Updating data must not reopen a root the user just collapsed. */
    TEST_ASSERT_TRUE(pui_view_set_tree_items(view, "objects", items, 3));
    for (int i = 0; i < 3; i++) {
        TEST_ASSERT_TRUE(pui_view_replace_slot(view, &pui, "body", NULL, error, sizeof error));
        TEST_ASSERT_EQUAL_INT(3, pui_view_tree_item_count(view, "objects"));
    }
    pui_pointer(&pui, 2, 42, true);
    pui_pointer(&pui, 2, 42, false);
    pui_view_draw(view, &pui, (PUIRect){0, 0, 400, 300}, runtime_tree_action, selected);
    TEST_ASSERT_EQUAL_STRING("b", selected);
    assert_pixel(1, 42, PUI_ACCENT);
    items[0].depth = 1;
    TEST_ASSERT_FALSE(pui_view_set_tree_items(view, "objects", items, 3));
    TEST_ASSERT_EQUAL_INT(3, pui_view_tree_item_count(view, "objects"));
    items[0].depth = 0;
    snprintf(items[1].id, sizeof items[1].id, "a");
    TEST_ASSERT_FALSE(pui_view_set_tree_items(view, "objects", items, 3));
    TEST_ASSERT_TRUE(pui_view_set_tree_items(view, "objects", NULL, 0));
    TEST_ASSERT_EQUAL_INT(0, pui_view_tree_item_count(view, "objects"));
}

static void test_tabler_icons_resolve_render_and_tint_cached_masks(void) {
    TEST_ASSERT_EQUAL_INT(ICON_SAVE, pui_icon_from_name("save"));
    TEST_ASSERT_EQUAL_INT(ICON_NONE, pui_icon_from_name(NULL));
    TEST_ASSERT_EQUAL_INT(ICON_NONE, pui_icon_from_name("tabler:missing-icon"));
    PUIIcon icon = pui_icon_from_name("tabler:device-floppy");
    TEST_ASSERT_TRUE(icon >= ICON_TABLER_BASE);
    TEST_ASSERT_NOT_EQUAL(icon, pui_icon_from_name("tabler:artboard"));
    pui_panel(renderer, (SDL_FRect){0, 0, 400, 300}, 0x000000FF);
    pui_icon(renderer, icon, 10, 10, 32, 0xFF8000FF);
    pui_icon(renderer, icon, 60, 10, 32, 0x0080FFFF);
    SDL_Surface *surface = SDL_RenderReadPixels(renderer, NULL);
    TEST_ASSERT_NOT_NULL(surface);
    int orange = 0, blue = 0;
    for (int y = 10; y < 42; y++) {
        for (int x = 10; x < 42; x++) {
            Uint8 r, g, b, a;
            TEST_ASSERT_TRUE(SDL_ReadSurfacePixel(surface, x, y, &r, &g, &b, &a));
            if (r > 32) {
                TEST_ASSERT_EQUAL_UINT8(0, b);
                TEST_ASSERT_TRUE(r >= g);
                orange++;
            }
            TEST_ASSERT_TRUE(SDL_ReadSurfacePixel(surface, x + 50, y, &r, &g, &b, &a));
            if (b > 32) {
                TEST_ASSERT_EQUAL_UINT8(0, r);
                TEST_ASSERT_TRUE(b >= g);
                blue++;
            }
        }
    }
    TEST_ASSERT_TRUE(orange > 50);
    TEST_ASSERT_EQUAL_INT(orange, blue);
    SDL_DestroySurface(surface);
}

static void test_complete_tabler_catalog_and_filled_variants_are_available_offline(void) {
    FILE *catalog = fopen(PENGUI_TABLER_CATALOG, "r");
    TEST_ASSERT_NOT_NULL(catalog);
    char name[256];
    int count = 0;
    while (fgets(name, sizeof name, catalog)) {
        if (name[0] == '#') {
            continue;
        }
        name[strcspn(name, "\r\n")] = 0;
        TEST_ASSERT_EQUAL_INT(ICON_TABLER_BASE + count, pui_icon_from_name(name));
        count++;
    }
    fclose(catalog);
    TEST_ASSERT_EQUAL_INT(6238, count);
    TEST_ASSERT_EQUAL_INT(pui_icon_from_name("tabler:heart"),
                          pui_icon_from_name("tabler:outline/heart"));
    PUIIcon outline = pui_icon_from_name("tabler:heart");
    PUIIcon filled = pui_icon_from_name("tabler:filled/heart");
    TEST_ASSERT_TRUE(filled >= ICON_TABLER_BASE);
    TEST_ASSERT_NOT_EQUAL(outline, filled);
    pui_panel(renderer, (SDL_FRect){0, 0, 400, 300}, 0x000000FF);
    pui_icon(renderer, outline, 10, 10, 48, 0xFFFFFFFF);
    pui_icon(renderer, filled, 80, 10, 48, 0xFFFFFFFF);
    assert_pixel(34, 34, 0x000000FF);
    assert_pixel(104, 34, 0xFFFFFFFF);
    TEST_ASSERT_NULL(pui_image_load_memory(renderer, "invalid PNG", 11));
    TEST_ASSERT_NULL(pui_image_load_memory(renderer, NULL, 0));
}

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_complete_tabler_catalog_and_filled_variants_are_available_offline);
    RUN_TEST(test_tabler_icons_resolve_render_and_tint_cached_masks);
    RUN_TEST(test_runtime_tree_hierarchy_updates_and_mounts_preserve_state);
    RUN_TEST(test_double_click_selects_word_and_drag_extends_by_words);
    RUN_TEST(test_triple_click_and_control_or_command_a_select_all);
    RUN_TEST(test_mouse_selection_handles_fast_forward_and_reverse_utf8_drags);
    RUN_TEST(test_mouse_selection_keeps_capture_and_does_not_activate_button_on_release);
    RUN_TEST(test_mouse_selection_autoscrolls_while_held_outside_field);
    RUN_TEST(test_focus_loss_cancels_mouse_selection);
    RUN_TEST(test_cursor_shapes_follow_hover_drag_and_window_leave);
    RUN_TEST(test_tab_focus_skips_hidden_disabled_and_wraps);
    RUN_TEST(test_reverse_tab_entry_single_field_and_no_inputs);
    RUN_TEST(test_toggle_checkbox_and_groups_keep_independent_state);
    RUN_TEST(test_vertical_group_default_and_keyboard_selection);
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
