#include "pengui/pengui.h"
#include "unity.h"
#include <string.h>
static SDL_Window *window;
static SDL_Renderer *renderer;
static PUILayout *layout;
static PUIView *view;
static PUIContext input;
static int commits, previews;
static uint32_t last;

static void action(void *context, const char *name, const char *widget) {
    (void)context;
    TEST_ASSERT_EQUAL_STRING("paint", widget);
    TEST_ASSERT_TRUE(pui_view_color(view, widget, &last));
    if (!strcmp(name, "color.commit")) {
        commits++;
    } else {
        TEST_ASSERT_EQUAL_STRING("color.preview", name);
        previews++;
    }
}

static void frame(void) {
    pui_view_draw(view, &input, (PUIRect){0, 0, 240, 320}, action, NULL);
}

void setUp(void) {
    TEST_ASSERT_TRUE(SDL_Init(SDL_INIT_VIDEO));
    window = SDL_CreateWindow("picker", 300, 640, SDL_WINDOW_HIDDEN);
    renderer = SDL_CreateRenderer(window, NULL);
    TEST_ASSERT_NOT_NULL(renderer);
    const char *yaml = "schema: pengui/v1\nroot: {type: color_picker, id: paint, value: "
                       "'#7393AF80', action: color.commit, preview_action: color.preview}";
    char error[256];
    layout = pui_layout_parse(yaml, strlen(yaml), error, sizeof error);
    TEST_ASSERT_NOT_NULL_MESSAGE(layout, error);
    view = pui_view_new(layout, renderer);
    TEST_ASSERT_NOT_NULL(view);
    pui_init(&input);
    commits = previews = 0;
    last = 0;
    frame();
}

void tearDown(void) {
    pui_view_free(view);
    pui_layout_free(layout);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
}

static void event(SDL_Event *event) {
    pui_view_event(view, &input, window, event);
    frame();
}

static void pointer(SDL_EventType type, float x, float y) {
    SDL_Event e = {.type = type};
    if (type == SDL_EVENT_MOUSE_MOTION) {
        e.motion.x = x;
        e.motion.y = y;
    } else {
        e.button.button = SDL_BUTTON_LEFT;
        e.button.x = x;
        e.button.y = y;
    }
    event(&e);
}

static void click(float x, float y) {
    pointer(SDL_EVENT_MOUSE_BUTTON_DOWN, x, y);
    pointer(SDL_EVENT_MOUSE_BUTTON_UP, x, y);
}

static void key(SDL_Keycode value, SDL_Keymod mod) {
    SDL_Event e = {.type = SDL_EVENT_KEY_DOWN};
    e.key.key = value;
    e.key.mod = mod;
    event(&e);
}

static void text(const char *value) {
    SDL_Event e = {.type = SDL_EVENT_TEXT_INPUT};
    e.text.text = value;
    event(&e);
}

static uint32_t color(void) {
    uint32_t rgba;
    TEST_ASSERT_TRUE(pui_view_color(view, "paint", &rgba));
    return rgba;
}

static void test_slider_preview_release_and_escape(void) {
    pointer(SDL_EVENT_MOUSE_BUTTON_DOWN, 120, 70);
    TEST_ASSERT_TRUE(pui_view_color_editing(view, "paint"));
    TEST_ASSERT_GREATER_THAN_INT(0, previews);
    TEST_ASSERT_EQUAL_INT(0, commits);
    pointer(SDL_EVENT_MOUSE_MOTION, 200, 70);
    TEST_ASSERT_EQUAL_INT(0, commits);
    pointer(SDL_EVENT_MOUSE_BUTTON_UP, 200, 70);
    TEST_ASSERT_EQUAL_INT(1, commits);
    TEST_ASSERT_FALSE(pui_view_color_editing(view, "paint"));
    uint32_t committed = color();
    pointer(SDL_EVENT_MOUSE_BUTTON_DOWN, 20, 70);
    pointer(SDL_EVENT_MOUSE_MOTION, 70, 70);
    key(SDLK_ESCAPE, 0);
    TEST_ASSERT_EQUAL_HEX32(committed, color());
    TEST_ASSERT_EQUAL_INT(1, commits);
    TEST_ASSERT_FALSE(pui_view_color_editing(view, "paint"));
    pointer(SDL_EVENT_MOUSE_BUTTON_DOWN, 10, 70);
    SDL_Event lost = {.type = SDL_EVENT_WINDOW_FOCUS_LOST};
    event(&lost);
    TEST_ASSERT_EQUAL_HEX32(committed, color());
    TEST_ASSERT_EQUAL_INT(1, commits);
}

static void test_hsl_fields_commit_on_tab_and_keep_next_selection(void) {
    key(SDLK_TAB, 0);
    text("120");
    key(SDLK_TAB, 0);
    TEST_ASSERT_EQUAL_INT(1, commits);
    text("100");
    key(SDLK_TAB, 0);
    TEST_ASSERT_EQUAL_STRING("100.00", pui_view_text(view, "paint.s"));
    text("50");
    key(SDLK_TAB, 0);
    TEST_ASSERT_EQUAL_HEX32(0x00ff0080, color());
    TEST_ASSERT_EQUAL_INT(3, commits);
    text("100");
    key(SDLK_RETURN, 0);
    TEST_ASSERT_EQUAL_HEX32(0x00ff00ff, color());
    TEST_ASSERT_EQUAL_STRING("#00FF00FF", pui_view_text(view, "paint.hex"));
}

static void test_rgba_mode_preserves_alpha_and_hex_validation(void) {
    click(100, 12); // RGBA mode; changing the mode itself is not an edit.
    TEST_ASSERT_EQUAL_INT(0, commits);
    TEST_ASSERT_EQUAL_HEX32(0x7393af80, color());
    pointer(SDL_EVENT_MOUSE_BUTTON_DOWN, 200, 70);
    pointer(SDL_EVENT_MOUSE_BUTTON_UP, 200, 70);
    TEST_ASSERT_EQUAL_UINT8(128, color() & 255);
    TEST_ASSERT_EQUAL_INT(1, commits);
    TEST_ASSERT_TRUE(pui_view_set_text(view, "paint.hex", "#F008"));
    click(130, 244);
    key(SDLK_RETURN, 0);
    TEST_ASSERT_EQUAL_HEX32(0xff000088, color());
    TEST_ASSERT_EQUAL_INT(2, commits);
    TEST_ASSERT_TRUE(pui_view_set_text(view, "paint.hex", "#GG0000"));
    key(SDLK_RETURN, 0);
    TEST_ASSERT_EQUAL_HEX32(0xff000088, color());
    TEST_ASSERT_EQUAL_INT(2, commits);
    key(SDLK_ESCAPE, 0);
    TEST_ASSERT_EQUAL_STRING("#FF000088", pui_view_text(view, "paint.hex"));
    TEST_ASSERT_TRUE(pui_view_set_text(view, "paint.r", "256"));
    click(200, 48);
    key(SDLK_RETURN, 0);
    TEST_ASSERT_EQUAL_HEX32(0xff000088, color());
    TEST_ASSERT_EQUAL_INT(2, commits);
}

static void test_picker_layout_validation_mount_retention_and_tab_prefixes(void) {
    char error[256];
    const char *invalid[] = {
        "schema: pengui/v1\nroot: {type: color_picker, value: '#GG0000'}",
        "schema: pengui/v1\nroot: {type: color_picker, mode: hsv}",
        "schema: pengui/v1\nroot: {type: button, color_input: paint}",
        "schema: pengui/v1\nroot: {type: color_picker, children: [{type: input}]}"};
    for (size_t i = 0; i < sizeof invalid / sizeof *invalid; i++) {
        PUILayout *bad = pui_layout_parse(invalid[i], strlen(invalid[i]), error, sizeof error);
        TEST_ASSERT_NULL(bad);
    }
    const char *yaml =
        "schema: pengui/v1\nroot: {type: column, padding: 0, children: [{type: slot, id: body}, "
        "{type: color_picker, id: other, height: 280, value: '#FF0000'}]}";
    PUILayout *base = pui_layout_parse(yaml, strlen(yaml), error, sizeof error);
    TEST_ASSERT_NOT_NULL(base);
    PUIView *second = pui_view_new(base, renderer);
    TEST_ASSERT_NOT_NULL(second);
    PUIContext ctx;
    pui_init(&ctx);
    TEST_ASSERT_TRUE(pui_view_set_color(second, "other", 0x12345678));
    TEST_ASSERT_TRUE(pui_view_replace_slot(second, &ctx, "body", layout, error, sizeof error));
    uint32_t rgba;
    TEST_ASSERT_TRUE(pui_view_color(second, "other", &rgba));
    TEST_ASSERT_EQUAL_HEX32(0x12345678, rgba);
    TEST_ASSERT_TRUE(pui_view_color(second, "paint", &rgba));
    TEST_ASSERT_EQUAL_HEX32(0x7393af80, rgba);
    TEST_ASSERT_TRUE(pui_view_set_color(second, "paint", 0xabcdef22));
    TEST_ASSERT_EQUAL_HEX32(0x7393af80, color());
    pui_view_free(second);
    pui_layout_free(base);
    const char *tabs_yaml = "schema: pengui/v1\nroot: {type: tabs, id: documents}";
    base = pui_layout_parse(tabs_yaml, strlen(tabs_yaml), error, sizeof error);
    TEST_ASSERT_NOT_NULL(base);
    second = pui_view_new(base, renderer);
    pui_init(&ctx);
    TEST_ASSERT_TRUE_MESSAGE(
        pui_view_add_tab(second, &ctx, "documents", "doc", "Document", layout, error, sizeof error),
        error);
    TEST_ASSERT_TRUE(pui_view_color(second, "doc.paint", &rgba));
    TEST_ASSERT_EQUAL_HEX32(0x7393af80, rgba);
    TEST_ASSERT_EQUAL_STRING("#7393AF80", pui_view_text(second, "doc.paint.hex"));
    pui_view_free(second);
    pui_layout_free(base);
}

static void test_programmatic_replacement_cancels_capture_without_blocking_next_gesture(void) {
    pointer(SDL_EVENT_MOUSE_BUTTON_DOWN, 120, 70);
    TEST_ASSERT_TRUE(pui_view_color_editing(view, "paint"));
    TEST_ASSERT_TRUE(pui_view_set_color(view, "paint", 0x12345678));
    frame();
    TEST_ASSERT_FALSE(pui_view_color_editing(view, "paint"));
    pointer(SDL_EVENT_MOUSE_BUTTON_UP, 120, 70);
    click(100, 12);
    pointer(SDL_EVENT_MOUSE_BUTTON_DOWN, 200, 70);
    TEST_ASSERT_TRUE(pui_view_color_editing(view, "paint"));
    pointer(SDL_EVENT_MOUSE_BUTTON_UP, 200, 70);
    TEST_ASSERT_EQUAL_UINT8(0x78, color() & 255);
    TEST_ASSERT_EQUAL_INT(1, commits);
}

static void test_popup_swatch_tracks_color_and_transparency(void) {
    const char *yaml = "schema: pengui/v1\nroot: {type: popup_button, id: swatch, "
                       "color_input: paint, height: 28, children: [{type: panel, children: [{type: "
                       "color_picker, id: paint, "
                       "value: '#FF0000'}]}]}";
    char error[256];
    pui_view_free(view);
    view = NULL;
    pui_layout_free(layout);
    layout = pui_layout_parse(yaml, strlen(yaml), error, sizeof error);
    TEST_ASSERT_NOT_NULL_MESSAGE(layout, error);
    view = pui_view_new(layout, renderer);
    TEST_ASSERT_NOT_NULL(view);
    const uint32_t colors[] = {0xff0000ff, 0x00ff00ff, 0x00000000};
    for (int i = 0; i < 3; i++) {
        TEST_ASSERT_TRUE(pui_view_set_color(view, "paint", colors[i]));
        frame();
        SDL_Surface *surface = SDL_RenderReadPixels(renderer, NULL);
        TEST_ASSERT_NOT_NULL(surface);
        Uint8 r, g, b, a;
        TEST_ASSERT_TRUE(SDL_ReadSurfacePixel(surface, 4, 4, &r, &g, &b, &a));
        if (i < 2) {
            TEST_ASSERT_EQUAL_UINT8(i == 0 ? 255 : 0, r);
            TEST_ASSERT_EQUAL_UINT8(i == 1 ? 255 : 0, g);
            TEST_ASSERT_EQUAL_UINT8(0, b);
        } else {
            TEST_ASSERT_EQUAL_UINT8(176, r);
            TEST_ASSERT_TRUE(SDL_ReadSurfacePixel(surface, 10, 4, &r, &g, &b, &a));
            TEST_ASSERT_EQUAL_UINT8(150, r);
        }
        SDL_DestroySurface(surface);
    }
    const char *tabs_yaml = "schema: pengui/v1\nroot: {type: tabs, id: docs}";
    PUILayout *tabs = pui_layout_parse(tabs_yaml, strlen(tabs_yaml), error, sizeof error);
    TEST_ASSERT_NOT_NULL(tabs);
    PUILayout *composed =
        pui_layout_add_tab(tabs, "docs", "doc", "Document", layout, error, sizeof error);
    TEST_ASSERT_NOT_NULL_MESSAGE(composed, error);
    bool found = false;
    for (int i = 0; i < composed->count; i++) {
        if (!strcmp(composed->nodes[i].name, "doc.swatch")) {
            TEST_ASSERT_EQUAL_STRING("doc.paint", composed->nodes[i].color_input);
            found = true;
        }
    }
    TEST_ASSERT_TRUE(found);
    pui_layout_free(composed);
    pui_layout_free(tabs);
    click(15, 12);
    SDL_Event escape = {.type = SDL_EVENT_KEY_DOWN};
    escape.key.key = SDLK_ESCAPE;
    TEST_ASSERT_TRUE(pui_view_event(view, &input, window, &escape));
}

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_popup_swatch_tracks_color_and_transparency);
    RUN_TEST(test_programmatic_replacement_cancels_capture_without_blocking_next_gesture);
    RUN_TEST(test_slider_preview_release_and_escape);
    RUN_TEST(test_hsl_fields_commit_on_tab_and_keep_next_selection);
    RUN_TEST(test_rgba_mode_preserves_alpha_and_hex_validation);
    RUN_TEST(test_picker_layout_validation_mount_retention_and_tab_prefixes);
    return UNITY_END();
}
