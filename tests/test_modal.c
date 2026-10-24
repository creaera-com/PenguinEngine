/** @file @brief Modal containment, focus, nested overlays and reusable YAML configuration. */
#include "pengui/pengui.h"
#include "unity.h"
#include <string.h>
static SDL_Window *window;
static SDL_Renderer *renderer;
static PUILayout *layout;
static PUIView *view;
static PUIContext input;
static int actions;
static const char *yaml =
    "schema: pengui/v1\nroot: {type: row, padding: 0, gap: 0, children: ["
    "{type: panel, padding: 0, width: 100, children: ["
    "{type: popup_button, id: dialog, text: Open, modal: true, modal_title: Settings, "
    "modal_target: work, popup_width: 180, popup_height: 200, children: ["
    "{type: panel, padding: 8, scroll: true, children: ["
    "{type: input, id: field, text: Before},"
    "{type: dropdown, id: mode, children: [{type: option, text: One, value: one}, {type: option, "
    "text: Two, value: two}]},"
    "{type: button, id: apply, action: apply, text: Apply}]}]},"
    "{type: button, id: behind, action: behind}, {type: spacer}]},"
    "{type: canvas, id: work}]}";

static void action(void *context, const char *name, const char *widget) {
    (void)context;
    (void)name;
    (void)widget;
    actions++;
}

static void frame(void) {
    pui_view_draw(view, &input, (PUIRect){0, 0, 400, 300}, action, NULL);
}

static void click(float x, float y) {
    pui_pointer(&input, x, y, true);
    pui_pointer(&input, x, y, false);
    frame();
}

static bool key(SDL_Keycode value) {
    SDL_Event e = {.type = SDL_EVENT_KEY_DOWN};
    e.key.key = value;
    return pui_view_event(view, &input, window, &e);
}

static void click_widget(const char *id) {
    for (int y = 0; y < 300; y += 2) {
        for (int x = 0; x < 400; x += 2) {
            if (pui_view_hit(view, id, (float)x, (float)y)) {
                click((float)x + 1, (float)y + 1);
                return;
            }
        }
    }
    TEST_FAIL_MESSAGE("Visible widget not found");
}

void setUp(void) {
    PUITheme theme = pui_dark_theme();
    pui_set_theme(&theme);
    TEST_ASSERT_TRUE(SDL_Init(SDL_INIT_VIDEO));
    window = SDL_CreateWindow("modal", 400, 300, SDL_WINDOW_HIDDEN);
    renderer = SDL_CreateRenderer(window, NULL);
    TEST_ASSERT_NOT_NULL(renderer);
    char error[256];
    layout = pui_layout_parse(yaml, strlen(yaml), error, sizeof error);
    TEST_ASSERT_NOT_NULL_MESSAGE(layout, error);
    view = pui_view_new(layout, renderer);
    TEST_ASSERT_NOT_NULL(view);
    pui_init(&input);
    actions = 0;
    frame();
}

void tearDown(void) {
    pui_view_free(view);
    pui_layout_free(layout);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
}

static void test_modal_centers_in_target_and_blocks_background_until_escape(void) {
    click_widget("dialog");
    TEST_ASSERT_TRUE(pui_view_modal_active(view));
    TEST_ASSERT_TRUE(pui_view_hit(view, "field", 180, 100));
    TEST_ASSERT_FALSE(pui_view_hit(view, "behind", 10, 40));
    click(10, 40);
    TEST_ASSERT_TRUE(pui_view_modal_active(view));
    TEST_ASSERT_EQUAL_INT(0, actions);
    TEST_ASSERT_TRUE(key(SDLK_Z));
    click_widget("field");
    TEST_ASSERT_TRUE(key(SDLK_TAB));
    TEST_ASSERT_TRUE(key(SDLK_ESCAPE));
    frame();
    TEST_ASSERT_FALSE(pui_view_modal_active(view));
    TEST_ASSERT_EQUAL_UINT32(pui_id("dialog"), input.focus);
    click_widget("behind");
    TEST_ASSERT_EQUAL_INT(1, actions);
}

static void test_nested_dropdown_and_focus_loss_preserve_modal(void) {
    TEST_ASSERT_TRUE(pui_view_open_modal(view, &input, "dialog"));
    frame();
    click_widget("mode");
    TEST_ASSERT_TRUE(key(SDLK_ESCAPE));
    frame();
    TEST_ASSERT_TRUE(pui_view_modal_active(view));
    SDL_Event lost = {.type = SDL_EVENT_WINDOW_FOCUS_LOST};
    pui_view_event(view, &input, window, &lost);
    frame();
    TEST_ASSERT_TRUE(pui_view_modal_active(view));
    click(320, 64);
    frame();
    TEST_ASSERT_FALSE(pui_view_modal_active(view));
}

static void test_modal_resizes_and_keeps_form_inside_view(void) {
    TEST_ASSERT_TRUE(pui_view_open_modal(view, &input, "dialog"));
    pui_view_draw(view, &input, (PUIRect){0, 0, 240, 160}, action, NULL);
    bool found = false;
    for (int y = 0; y < 160; y++) {
        for (int x = 0; x < 240; x++) {
            if (pui_view_hit(view, "field", (float)x, (float)y)) {
                found = true;
            }
        }
    }
    TEST_ASSERT_TRUE(found);
    TEST_ASSERT_FALSE(pui_view_hit(view, "field", 260, 100));
    TEST_ASSERT_TRUE(key(SDLK_ESCAPE));
    frame();
    TEST_ASSERT_FALSE(pui_view_modal_active(view));
}

static void test_validation_and_tab_reference_prefix(void) {
    TEST_ASSERT_FALSE(pui_view_open_modal(view, &input, "behind"));
    const char *invalid[] = {"schema: pengui/v1\nroot: {type: button, modal: true}",
                             "schema: pengui/v1\nroot: {type: popup_button, modal_target: work, "
                             "children: [{type: panel}]}"};
    char error[256];
    for (int i = 0; i < 2; i++) {
        PUILayout *bad = pui_layout_parse(invalid[i], strlen(invalid[i]), error, sizeof error);
        TEST_ASSERT_NULL(bad);
    }
    const char *base = "schema: pengui/v1\nroot: {type: tabs, id: docs}";
    PUILayout *tabs = pui_layout_parse(base, strlen(base), error, sizeof error);
    TEST_ASSERT_NOT_NULL(tabs);
    PUILayout *composed =
        pui_layout_add_tab(tabs, "docs", "page", "Page", layout, error, sizeof error);
    TEST_ASSERT_NOT_NULL_MESSAGE(composed, error);
    bool found = false;
    for (int i = 0; i < composed->count; i++) {
        if (!strcmp(composed->nodes[i].name, "page.dialog")) {
            TEST_ASSERT_EQUAL_STRING("page.work", composed->nodes[i].modal_target);
            found = true;
        }
    }
    TEST_ASSERT_TRUE(found);
    pui_layout_free(composed);
    pui_layout_free(tabs);
}

static Uint8 shade(int x, int y) {
    SDL_Surface *surface = SDL_RenderReadPixels(renderer, NULL);
    TEST_ASSERT_NOT_NULL(surface);
    Uint8 r, g, b, a;
    TEST_ASSERT_TRUE(SDL_ReadSurfacePixel(surface, x, y, &r, &g, &b, &a));
    SDL_DestroySurface(surface);
    return r;
}

static void test_modal_and_backdrop_fade_without_delaying_input_release(void) {
    PUITheme theme = pui_dark_theme();
    theme.button_transition_ms = 200;
    pui_set_theme(&theme);
    frame();
    Uint8 background = shade(10, 150);
    TEST_ASSERT_TRUE(pui_view_open_modal(view, &input, "dialog"));
    frame();
    TEST_ASSERT_EQUAL_UINT8(background, shade(10, 150));
    SDL_Delay(100);
    frame();
    Uint8 middle = shade(10, 150);
    SDL_Delay(150);
    frame();
    Uint8 open = shade(10, 150);
    TEST_ASSERT_TRUE(background > middle && middle > open);
    /* The dialog itself also fades: its title-region pixel differs from the canvas. */
    Uint8 dialog = shade(180, 65);
    TEST_ASSERT_TRUE(dialog > open);
    TEST_ASSERT_TRUE(key(SDLK_ESCAPE));
    TEST_ASSERT_FALSE(pui_view_modal_active(view));
    frame();
    SDL_Delay(100);
    frame();
    Uint8 closing = shade(10, 150);
    TEST_ASSERT_TRUE(closing > open && closing < background);
    SDL_Delay(150);
    frame();
    TEST_ASSERT_EQUAL_UINT8(background, shade(10, 150));
    TEST_ASSERT_TRUE(pui_view_hit(view, "behind", 10, 40));
}

static void test_zero_duration_switches_immediately_and_preserves_render_target(void) {
    for (int i = 0; i < layout->count; i++) {
        if (layout->nodes[i].modal) {
            layout->nodes[i].transition_ms = 0;
        }
    }
    SDL_Texture *target =
        SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGBA8888, SDL_TEXTUREACCESS_TARGET, 400, 300);
    TEST_ASSERT_NOT_NULL(target);
    TEST_ASSERT_TRUE(SDL_SetRenderTarget(renderer, target));
    frame();
    Uint8 background = shade(10, 150);
    TEST_ASSERT_TRUE(pui_view_open_modal(view, &input, "dialog"));
    frame();
    TEST_ASSERT_EQUAL_PTR(target, SDL_GetRenderTarget(renderer));
    TEST_ASSERT_LESS_THAN_UINT8(background, shade(10, 150));
    TEST_ASSERT_TRUE(key(SDLK_ESCAPE));
    frame();
    TEST_ASSERT_EQUAL_UINT8(background, shade(10, 150));
    TEST_ASSERT_EQUAL_PTR(target, SDL_GetRenderTarget(renderer));
    SDL_SetRenderTarget(renderer, NULL);
    SDL_DestroyTexture(target);
}

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_zero_duration_switches_immediately_and_preserves_render_target);
    RUN_TEST(test_modal_and_backdrop_fade_without_delaying_input_release);
    RUN_TEST(test_modal_resizes_and_keeps_form_inside_view);
    RUN_TEST(test_modal_centers_in_target_and_blocks_background_until_escape);
    RUN_TEST(test_nested_dropdown_and_focus_loss_preserve_modal);
    RUN_TEST(test_validation_and_tab_reference_prefix);
    return UNITY_END();
}
