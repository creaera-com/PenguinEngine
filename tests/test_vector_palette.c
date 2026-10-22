#include "editor_host/host.h"
#include "editors/vector/internal.h"
#include "shell/shell.h"
#include "unity.h"

static SDL_Window *window;
static SDL_Renderer *renderer;
static EditorShell shell;
static EditorHost host;

static void commit(void) {
    char error[256];
    TEST_ASSERT_TRUE_MESSAGE(
        editor_host_commit(&host, shell.view, &shell.input, error, sizeof error), error);
    pui_view_draw(shell.view, &shell.input, (PUIRect){0, 0, 1280, 800}, NULL, NULL);
}

static VectorDocument *document(void) {
    VectorEditor *editor = host.instance;
    return vector_document_tabs_active(&editor->documents, shell.view);
}

static void action(const char *name) {
    TEST_ASSERT_TRUE(editor_host_action(&host, shell.view, name, ""));
    commit();
}

static void activate(const char *id) {
    TEST_ASSERT_TRUE(editor_host_request(&host, id, false));
    commit();
}

void setUp(void) {
    TEST_ASSERT_TRUE(SDL_Init(SDL_INIT_VIDEO));
    window = SDL_CreateWindow("grid", 1280, 800, SDL_WINDOW_HIDDEN);
    renderer = SDL_CreateRenderer(window, NULL);
    TEST_ASSERT_NOT_NULL(renderer);
    char path[1600], error[256];
    snprintf(path, sizeof path, "%s/shell.yaml", PENGUI_EDITOR_RESOURCES);
    TEST_ASSERT_TRUE_MESSAGE(editor_shell_load(&shell, renderer, path, error, sizeof error), error);
    editor_host_init(&host, (EditorServices){PENGUI_EDITOR_RESOURCES});
    activate("vector");
    commit();
    /* These feature fixtures build their own scene independently of startup content. */
    vector_history_destroy(&document()->history);
    vector_document_init(document(), document()->id);
}

void tearDown(void) {
    editor_shell_destroy(&shell);
    editor_host_destroy(&host);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
}

#include "editors/vector/palette/palette.h"
#include <math.h>

static void test_harmonies_preserve_base_and_alpha(void) {
    const int counts[] = {5, 5, 3, 4, 2, 3, 4};
    uint32_t base = pui_color_from_hsl((PUIHSLColor){.h = 350, .s = .8f, .l = .5f, .a = .5f});
    for (int mode = 0; mode < 7; mode++) {
        uint32_t palette[5];
        int count = vector_palette_generate(base, (VectorHarmony)mode, 30, palette);
        TEST_ASSERT_EQUAL_INT(counts[mode], count);
        TEST_ASSERT_EQUAL_HEX32(base, palette[0]);
        for (int i = 0; i < count; i++) {
            TEST_ASSERT_EQUAL_UINT8(base & 255, palette[i] & 255);
        }
        if (mode == VECTOR_HARMONY_TRIAD) {
            TEST_ASSERT_FLOAT_WITHIN(1, 110, pui_color_to_hsl(palette[1]).h);
            TEST_ASSERT_FLOAT_WITHIN(1, 230, pui_color_to_hsl(palette[2]).h);
        }
    }
}

static void test_palette_actions_leave_artwork_unchanged(void) {
    VectorEditor *editor = host.instance;
    TEST_ASSERT_TRUE(pui_view_set_color(shell.view, "palette_base", 0x3498db80));
    action("vector.palette.base");
    TEST_ASSERT_EQUAL_HEX32(0x3498db80, editor->palette_ui.base);
    TEST_ASSERT_TRUE(pui_view_set_value(shell.view, "palette_harmony", "2"));
    action("vector.palette.harmony");
    TEST_ASSERT_EQUAL_INT(VECTOR_HARMONY_TRIAD, editor->palette_ui.harmony);
    TEST_ASSERT_TRUE(pui_view_set_text(shell.view, "palette_spread", "500"));
    action("vector.palette.spread");
    TEST_ASSERT_EQUAL_STRING("90", pui_view_text(shell.view, "palette_spread"));
    action("vector.palette.copy");
    char *clipboard = SDL_GetClipboardText();
    TEST_ASSERT_NOT_NULL(strstr(clipboard, "#3498DB80"));
    SDL_free(clipboard);
    TEST_ASSERT_EQUAL_INT(0, document()->rectangle_count);
    TEST_ASSERT_EQUAL_INT(0, document()->artboard_count);
}

static void test_wheel_drag_changes_base_only_in_visible_canvas(void) {
    VectorEditor *editor = host.instance;
    /* Locate the tab through the actual rendered layout rather than assuming column widths. */
    for (int x = 700; x < 1280 && editor->palette_ui.area.w == 0; x += 20) {
        pui_pointer(&shell.input, (float)x, 64, true);
        pui_pointer(&shell.input, (float)x, 64, false);
        pui_view_draw(shell.view, &shell.input, (PUIRect){0, 0, 1280, 800}, NULL, NULL);
    }
    PUIRect area = editor->palette_ui.area;
    TEST_ASSERT_GREATER_THAN_FLOAT(0, area.w);
    float radius = fminf(area.w - 24, area.h - 78) * .5f;
    float x = area.x + area.w / 2 + radius * .5f, y = area.y + radius + 8;
    SDL_Event e = {.type = SDL_EVENT_MOUSE_BUTTON_DOWN};
    e.button.button = SDL_BUTTON_LEFT;
    e.button.x = x;
    e.button.y = y;
    TEST_ASSERT_TRUE(vector_editor_event(editor, shell.view, &e));
    TEST_ASSERT_TRUE(editor->palette_ui.dragging);
    PUIHSLColor color = pui_color_to_hsl(editor->palette_ui.base);
    TEST_ASSERT_FLOAT_WITHIN(1, 0, color.h);
    TEST_ASSERT_FLOAT_WITHIN(.02f, .5f, color.s);
    e.type = SDL_EVENT_MOUSE_BUTTON_UP;
    TEST_ASSERT_TRUE(vector_editor_event(editor, shell.view, &e));
    TEST_ASSERT_FALSE(editor->palette_ui.dragging);
    TEST_ASSERT_EQUAL_INT(0, document()->rectangle_count);
}

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_wheel_drag_changes_base_only_in_visible_canvas);
    RUN_TEST(test_harmonies_preserve_base_and_alpha);
    RUN_TEST(test_palette_actions_leave_artwork_unchanged);
    return UNITY_END();
}
