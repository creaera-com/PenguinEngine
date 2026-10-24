#include "editor_host/host.h"
#include "editors/vector/curve/curve.h"
#include "editors/vector/document/selection.h"
#include "editors/vector/ellipse/render.h"
#include "editors/vector/internal.h"
#include "editors/vector/object/clipboard.h"
#include "editors/vector/path/operations.h"
#include "editors/vector/text/text.h"
#include "editors/vector/text/tool.h"
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
        e.button.windowID = SDL_GetWindowID(window);
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
    e.key.windowID = SDL_GetWindowID(window);
    send(&e);
}

static void type(const char *text) {
    SDL_Event e = {.type = SDL_EVENT_TEXT_INPUT};
    e.text.windowID = SDL_GetWindowID(window);
    e.text.text = text;
    send(&e);
}

static float fixed_advance(void *context, unsigned code, unsigned next, float size) {
    (void)context;
    (void)code;
    (void)next;
    (void)size;
    return 10;
}

static void test_text_layout_columns_alignment_and_overflow(void) {
    VectorShape shape;
    TEST_ASSERT_TRUE(vector_text_shape((PUIRect){0, 0, 150, 40}, &shape));
    VectorText t = shape.text;
    t.size = 10;
    t.line_height = 1;
    t.columns = 2;
    t.column_gap = 10;
    snprintf(t.content, sizeof t.content, "one two three four five six seven eight nine ten");
    VectorTextLayout layout;
    vector_text_layout(&t, 150, 60, fixed_advance, NULL, &layout);
    bool column_two = false;
    for (int i = 0; i < layout.count; i++) {
        column_two |= layout.glyphs[i].x >= 80;
    }
    TEST_ASSERT_TRUE(column_two);
    TEST_ASSERT_FALSE(layout.overflow);
    vector_text_layout(&t, 150, 10, fixed_advance, NULL, &layout);
    TEST_ASSERT_TRUE(layout.overflow);
    t.columns = 1;
    snprintf(t.content, sizeof t.content, "abc");
    t.alignment = VECTOR_TEXT_CENTER;
    vector_text_layout(&t, 150, 40, fixed_advance, NULL, &layout);
    TEST_ASSERT_EQUAL_FLOAT(60, layout.glyphs[0].x);
    t.alignment = VECTOR_TEXT_RIGHT;
    vector_text_layout(&t, 150, 40, fixed_advance, NULL, &layout);
    TEST_ASSERT_EQUAL_FLOAT(120, layout.glyphs[0].x);
    t.alignment = VECTOR_TEXT_JUSTIFY;
    snprintf(t.content, sizeof t.content, "one two three four");
    vector_text_layout(&t, 100, 40, fixed_advance, NULL, &layout);
    TEST_ASSERT_EQUAL_FLOAT(100, layout.glyphs[6].x + layout.glyphs[6].advance);
    t.columns = 2;
    t.column_gap = 200;
    vector_text_layout(&t, 100, 40, fixed_advance, NULL, &layout);
    TEST_ASSERT_TRUE(layout.overflow);
}

static void test_typing_history_unicode_newlines_clipboard_and_native_tool(void) {
    VectorDocument *d = document();
    tool("vector.add_text");
    TEST_ASSERT_TRUE(pui_view_checked(shell.view, "vector_add_text"));
    int history = d->history.count;
    pointer(SDL_EVENT_MOUSE_BUTTON_DOWN, 30, 40);
    pointer(SDL_EVENT_MOUSE_MOTION, 430, 300);
    TEST_ASSERT_EQUAL_INT(0, d->rectangle_count);
    pointer(SDL_EVENT_MOUSE_BUTTON_UP, 430, 300);
    TEST_ASSERT_EQUAL_INT(history + 1, d->history.count);
    TEST_ASSERT_TRUE(d->rectangles[0].text.enabled);
    TEST_ASSERT_EQUAL_UINT(1, d->text_tool.editing_id);
    TEST_ASSERT_TRUE(SDL_TextInputActive(window));
    type("Gruesse ");
    type("äöü");
    key(SDLK_RETURN, 0);
    type("Zweite Zeile");
    TEST_ASSERT_EQUAL_STRING("Gruesse äöü\nZweite Zeile", d->rectangles[0].text.content);
    TEST_ASSERT_EQUAL_INT(history + 2, d->history.count);
    key(SDLK_A, SDL_KMOD_CTRL);
    key(SDLK_C, SDL_KMOD_CTRL);
    char *clipboard = SDL_GetClipboardText();
    TEST_ASSERT_EQUAL_STRING(d->rectangles[0].text.content, clipboard);
    SDL_free(clipboard);
    key(SDLK_RETURN, SDL_KMOD_CTRL);
    TEST_ASSERT_EQUAL_UINT(0, d->text_tool.editing_id);
    key(SDLK_Z, SDL_KMOD_CTRL);
    TEST_ASSERT_EQUAL_STRING("Text", d->rectangles[0].text.content);
    key(SDLK_Z, SDL_KMOD_CTRL | SDL_KMOD_SHIFT);
    TEST_ASSERT_EQUAL_STRING("Gruesse äöü\nZweite Zeile", d->rectangles[0].text.content);
}

static void test_typography_inspector_fonts_validation_and_checkpoint(void) {
    VectorDocument *d = document();
    TEST_ASSERT_TRUE(vector_text_add(d, 1, (PUIRect){30, 40, 400, 260}));
    commit();
    TEST_ASSERT_EQUAL_STRING("Textfeld", pui_view_text(shell.view, "rectangle_kind"));
    TEST_ASSERT_EQUAL_STRING("24", pui_view_text(shell.view, "text_size"));
    TEST_ASSERT_EQUAL_STRING("0", pui_view_value(shell.view, "text_font"));
    VectorEditor *editor = host.instance;
    TEST_ASSERT_GREATER_THAN_INT(0, pui_fonts_count(editor->fonts));
    pui_view_set_text(shell.view, "text_columns", "2");
    tool("vector.text.columns");
    pui_view_set_text(shell.view, "text_letter_spacing", "1.5");
    tool("vector.text.letter_spacing");
    pui_view_set_text(shell.view, "text_line_height", "1.6");
    tool("vector.text.line_height");
    pui_view_set_checked(shell.view, "text_bold", true);
    tool("vector.text.bold");
    TEST_ASSERT_EQUAL_UINT(2, d->rectangles[0].text.columns);
    TEST_ASSERT_EQUAL_FLOAT(1.5f, d->rectangles[0].text.letter_spacing);
    TEST_ASSERT_TRUE(d->rectangles[0].text.bold);
    TEST_ASSERT_TRUE(pui_view_set_value(shell.view, "text_alignment", "justify"));
    tool("vector.text.alignment");
    VectorShape saved = d->rectangles[0];
    pui_view_set_text(shell.view, "text_columns", "2.5");
    tool("vector.text.columns");
    TEST_ASSERT_EQUAL_UINT(2, d->rectangles[0].text.columns);
    TEST_ASSERT_FALSE(vector_paths_can_operate(d, VECTOR_PATH_CONVERT));
    activate("scripts");
    activate("vector");
    d = document();
    TEST_ASSERT_EQUAL_MEMORY(&saved, &d->rectangles[0], sizeof saved);
    TEST_ASSERT_TRUE(vector_history_undo(&d->history, d));
    TEST_ASSERT_EQUAL_UINT(VECTOR_TEXT_LEFT, d->rectangles[0].text.alignment);
    TEST_ASSERT_TRUE(vector_history_redo(&d->history, d));
    TEST_ASSERT_TRUE(vector_objects_duplicate(d));
    TEST_ASSERT_EQUAL_MEMORY(&saved.text, &d->rectangles[1].text, sizeof saved.text);
    TEST_ASSERT_TRUE(vector_rectangle_add(d, 1, (PUIRect){10, 10, 20, 20}));
    vector_document_select(d, d->active_rectangle);
    commit();
    TEST_ASSERT_NULL(pui_view_text(shell.view, "text_size"));
    VectorText invalid = saved.text;
    invalid.size = NAN;
    TEST_ASSERT_FALSE(vector_text_valid(&invalid));
    invalid = saved.text;
    invalid.content[0] = (char)0xc0;
    invalid.content[1] = 0;
    TEST_ASSERT_FALSE(vector_text_valid(&invalid));
}

static void screen_click(float x, float y) {
    SDL_Event event = {.type = SDL_EVENT_MOUSE_BUTTON_DOWN};
    event.button.windowID = SDL_GetWindowID(window);
    event.button.button = SDL_BUTTON_LEFT;
    event.button.x = x;
    event.button.y = y;
    send(&event);
    event.type = SDL_EVENT_MOUSE_BUTTON_UP;
    send(&event);
}

static void test_real_font_rasterization_and_inspector_focus(void) {
    VectorEditor *editor = host.instance;
    int w, h, x, y;
    unsigned char *bitmap = pui_fonts_bitmap(editor->fonts, 0, 'A', 32, &w, &h, &x, &y);
    TEST_ASSERT_NOT_NULL(bitmap);
    TEST_ASSERT_GREATER_THAN_INT(0, w);
    TEST_ASSERT_GREATER_THAN_INT(0, h);
    bool ink = false;
    for (int i = 0; i < w * h; i++) {
        ink |= bitmap[i] != 0;
    }
    TEST_ASSERT_TRUE(ink);
    pui_fonts_free_bitmap(bitmap);
    VectorDocument *d = document();
    TEST_ASSERT_TRUE(vector_text_add(d, 1, (PUIRect){30, 40, 400, 260}));
    commit();
    /* Typography is mounted directly; find the field, scrolling the inspector
     * when its small fixture viewport cannot show the whole form at once. */
    float field_x = -1, field_y = -1;
    for (int attempt = 0; attempt < 8 && field_x < 0; attempt++) {
        for (int y = 90; y < 580 && field_x < 0; y += 4) {
            for (int x = 1000; x < 1270; x += 4) {
                if (pui_view_hit(shell.view, "text_size", x, y)) {
                    field_x = x + 2;
                    field_y = y + 2;
                    break;
                }
            }
        }
        if (field_x < 0) {
            SDL_Event wheel = {.type = SDL_EVENT_MOUSE_WHEEL};
            wheel.wheel.mouse_x = 1200;
            wheel.wheel.mouse_y = 220;
            wheel.wheel.y = -2;
            send(&wheel);
        }
    }
    TEST_ASSERT_TRUE(field_x >= 0);
    screen_click(field_x, field_y);
    key(SDLK_A, SDL_KMOD_CTRL);
    type("32");
    key(SDLK_TAB, 0);
    TEST_ASSERT_EQUAL_FLOAT(32, d->rectangles[0].text.size);
    if (pui_fonts_count(editor->fonts) > 1) {
        TEST_ASSERT_TRUE(pui_view_set_value(shell.view, "text_font", "1"));
        tool("vector.text.font");
        const PUIFontEntry *font = pui_fonts_entry(editor->fonts, 1);
        TEST_ASSERT_EQUAL_STRING(font->path, d->rectangles[0].text.font);
        TEST_ASSERT_EQUAL_UINT(font->index, d->rectangles[0].text.face);
    }
}

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_real_font_rasterization_and_inspector_focus);
    RUN_TEST(test_text_layout_columns_alignment_and_overflow);
    RUN_TEST(test_typing_history_unicode_newlines_clipboard_and_native_tool);
    RUN_TEST(test_typography_inspector_fonts_validation_and_checkpoint);
    return UNITY_END();
}
