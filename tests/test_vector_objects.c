#include "editor_host/host.h"
#include "editors/vector/internal.h"
#include "editors/vector/object/clipboard.h"
#include "shell/shell.h"
#include "unity.h"
#include <string.h>

static SDL_Window *window;
static SDL_Renderer *renderer;
static EditorShell shell;
static EditorHost host;

static void action_callback(void *context, const char *name, const char *widget) {
    (void)context;
    TEST_ASSERT_TRUE(editor_host_action(&host, shell.view, name, widget));
}

static void frame(void) {
    pui_view_draw(shell.view, &shell.input, (PUIRect){0, 0, 1280, 800}, action_callback, NULL);
}

static void commit(void) {
    char error[256];
    TEST_ASSERT_TRUE_MESSAGE(
        editor_host_commit(&host, shell.view, &shell.input, error, sizeof error), error);
    frame();
}

static void activate(const char *id) {
    TEST_ASSERT_TRUE(editor_host_request(&host, id, false));
    commit();
}

static VectorDocument *document(void) {
    VectorEditor *editor = host.instance;
    return vector_document_tabs_active(&editor->documents, shell.view);
}

static void action(const char *name) {
    TEST_ASSERT_TRUE(editor_host_action(&host, shell.view, name, ""));
    commit();
}

static void event(SDL_Event *e) {
    if (!pui_view_event(shell.view, &shell.input, window, e)) {
        editor_host_event(&host, shell.view, e);
    }
    commit();
}

static void click(int button, float x, float y) {
    SDL_Event e = {.type = SDL_EVENT_MOUSE_BUTTON_DOWN};
    e.button.button = button;
    e.button.x = x;
    e.button.y = y;
    event(&e);
    e.type = SDL_EVENT_MOUSE_BUTTON_UP;
    event(&e);
}

static void key(SDL_Keycode key, SDL_Keymod mod) {
    SDL_Event e = {.type = SDL_EVENT_KEY_DOWN};
    e.key.key = key;
    e.key.mod = mod;
    event(&e);
}

void setUp(void) {
    TEST_ASSERT_TRUE(SDL_Init(SDL_INIT_VIDEO));
    SDL_SetModState(0);
    window = SDL_CreateWindow("objects", 1280, 800, SDL_WINDOW_HIDDEN);
    renderer = SDL_CreateRenderer(window, NULL);
    TEST_ASSERT_NOT_NULL(renderer);
    char path[1600], error[256];
    snprintf(path, sizeof path, "%s/shell.yaml", PENGUI_EDITOR_RESOURCES);
    TEST_ASSERT_TRUE_MESSAGE(editor_shell_load(&shell, renderer, path, error, sizeof error), error);
    editor_host_init(&host, (EditorServices){PENGUI_EDITOR_RESOURCES});
    activate("vector");
    commit();
    VectorDocument *d = document();
    TEST_ASSERT_TRUE(vector_artboard_add(d, (PUIRect){10, 20, 420, 340}));
    TEST_ASSERT_TRUE(vector_rectangle_add(d, 1, (PUIRect){30, 40, 70, 50}));
    TEST_ASSERT_TRUE(vector_rectangle_add(d, 1, (PUIRect){170, 110, 90, 60}));
    TEST_ASSERT_TRUE(vector_rectangle_set_angle(d, 1, 30));
    TEST_ASSERT_TRUE(
        vector_rectangle_set_stroke(d, 1, (VectorStroke){0xff8000ff, 8, VECTOR_STROKE_OUTSIDE}));
    vector_document_select(d, 1);
    vector_document_toggle(d, 2);
    commit();
}

void tearDown(void) {
    editor_shell_destroy(&shell);
    editor_host_destroy(&host);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
}

static void test_group_delete_undo_restores_order_ids_properties_and_selection(void) {
    VectorDocument *d = document();
    int history = d->history.count;
    VectorRectangle originals[2];
    memcpy(originals, d->rectangles, sizeof originals);
    action("vector.objects.delete");
    TEST_ASSERT_EQUAL_INT(0, d->rectangle_count);
    TEST_ASSERT_EQUAL_INT(history + 1, d->history.count);
    TEST_ASSERT_EQUAL_INT(1, d->artboard_count);
    action("editor.undo");
    TEST_ASSERT_EQUAL_MEMORY(originals, d->rectangles, sizeof originals);
    TEST_ASSERT_EQUAL_INT(2, d->selection.count);
    action("editor.redo");
    TEST_ASSERT_EQUAL_INT(0, d->rectangle_count);
    activate("scripts");
    activate("vector");
    action("editor.undo");
    TEST_ASSERT_EQUAL_MEMORY(originals, document()->rectangles, sizeof originals);
}

static void test_copy_across_tabs_owns_values_and_preserves_clipboard_after_editor_switch(void) {
    VectorDocument *source = document();
    action("vector.objects.copy");
    TEST_ASSERT_TRUE(vector_rectangle_set_fill(source, 1, 0x000000ff));
    TEST_ASSERT_TRUE(vector_objects_delete(source));
    action("vector.add_document");
    VectorDocument *target = document();
    TEST_ASSERT_EQUAL_INT(2, target->id);
    action("vector.objects.paste");
    TEST_ASSERT_EQUAL_INT(1, target->artboard_count);
    TEST_ASSERT_EQUAL_INT(2, target->rectangle_count);
    TEST_ASSERT_EQUAL_HEX32(VECTOR_RECTANGLE_DEFAULT_FILL, target->rectangles[0].fill);
    TEST_ASSERT_EQUAL_FLOAT(30, target->rectangles[0].angle);
    TEST_ASSERT_EQUAL_FLOAT(8, target->rectangles[0].stroke.width);
    TEST_ASSERT_EQUAL_INT(2, target->selection.count);
    TEST_ASSERT_EQUAL_INT(1, target->history.count);
    activate("scripts");
    activate("vector");
    target = document();
    TEST_ASSERT_EQUAL_INT(2, target->id);
    action("editor.undo");
    TEST_ASSERT_EQUAL_INT(0, target->artboard_count);
    TEST_ASSERT_EQUAL_INT(0, target->rectangle_count);
    action("editor.redo");
    TEST_ASSERT_EQUAL_INT(2, target->rectangle_count);
    action("vector.objects.paste");
    TEST_ASSERT_EQUAL_INT(4, target->rectangle_count);
    TEST_ASSERT_EQUAL_INT(1, target->artboard_count);
    TEST_ASSERT_NOT_EQUAL(target->rectangles[0].id, target->rectangles[2].id);
}

static void test_artboard_children_cut_and_duplicate_are_atomic_and_clipboard_is_unchanged(void) {
    VectorDocument *d = document();
    vector_document_select(d, 0);
    d->active_artboard = 1;
    commit();
    action("vector.objects.copy");
    VectorClipboard original = ((VectorEditor *)host.instance)->object_actions.clipboard;
    int count = d->history.count;
    action("vector.objects.duplicate");
    TEST_ASSERT_EQUAL_INT(2, d->artboard_count);
    TEST_ASSERT_EQUAL_INT(4, d->rectangle_count);
    TEST_ASSERT_EQUAL_INT(count + 1, d->history.count);
    TEST_ASSERT_EQUAL_INT(2, d->rectangles[2].parent_id);
    TEST_ASSERT_EQUAL_FLOAT(26, d->artboards[1].rect.x);
    TEST_ASSERT_EQUAL_FLOAT(30, d->rectangles[2].rect.x);
    TEST_ASSERT_EQUAL_MEMORY(&original, &((VectorEditor *)host.instance)->object_actions.clipboard,
                             sizeof original);
    action("vector.objects.cut");
    TEST_ASSERT_EQUAL_INT(1, d->artboard_count);
    TEST_ASSERT_EQUAL_INT(2, d->rectangle_count);
    action("editor.undo");
    TEST_ASSERT_EQUAL_INT(2, d->artboard_count);
    action("editor.undo");
    TEST_ASSERT_EQUAL_INT(1, d->artboard_count);
    action("vector.objects.paste");
    TEST_ASSERT_EQUAL_INT(2, d->artboard_count);
    TEST_ASSERT_EQUAL_INT(3, d->artboards[1].id); // Undo never recycles IDs.
    activate("scripts");
    activate("vector");
    TEST_ASSERT_EQUAL_INT(2, document()->artboard_count);
}

static void test_locked_child_and_capacity_failure_do_not_delete_or_overwrite_clipboard(void) {
    VectorDocument *d = document();
    action("vector.objects.copy");
    VectorClipboard original = ((VectorEditor *)host.instance)->object_actions.clipboard;
    TEST_ASSERT_TRUE(vector_rectangle_set_state(d, 2, VECTOR_OBJECT_LOCKED));
    vector_document_select(d, 0);
    d->active_artboard = 1;
    int count = d->history.count;
    action("vector.objects.cut");
    TEST_ASSERT_EQUAL_INT(1, d->artboard_count);
    TEST_ASSERT_EQUAL_INT(2, d->rectangle_count);
    TEST_ASSERT_EQUAL_INT(count, d->history.count);
    TEST_ASSERT_EQUAL_MEMORY(&original, &((VectorEditor *)host.instance)->object_actions.clipboard,
                             sizeof original);
    TEST_ASSERT_TRUE(vector_rectangle_set_state(d, 2, 0));
    int cursor = d->history.cursor;
    d->history.cursor = VECTOR_HISTORY_CAPACITY;
    TEST_ASSERT_FALSE(
        vector_objects_cut(d, &((VectorEditor *)host.instance)->object_actions.clipboard));
    d->history.cursor = cursor;
    TEST_ASSERT_EQUAL_INT(2, d->rectangle_count);
    TEST_ASSERT_EQUAL_MEMORY(&original, &((VectorEditor *)host.instance)->object_actions.clipboard,
                             sizeof original);
    VectorClipboard invalid = original;
    invalid.rectangles[0].parent_id = 999;
    TEST_ASSERT_FALSE(vector_objects_paste(d, &invalid));
    TEST_ASSERT_EQUAL_INT(2, d->rectangle_count);
}

static void test_radial_context_click_escape_and_shortcuts(void) {
    VectorDocument *d = document();
    int count = d->history.count;
    click(SDL_BUTTON_RIGHT, 630, 400);
    click(SDL_BUTTON_LEFT, 630, 296); // Top sector: copy.
    TEST_ASSERT_EQUAL_INT(
        2, ((VectorEditor *)host.instance)->object_actions.clipboard.rectangle_count);
    TEST_ASSERT_EQUAL_INT(count, d->history.count);
    click(SDL_BUTTON_RIGHT, 630, 400);
    key(SDLK_ESCAPE, 0);
    TEST_ASSERT_EQUAL_INT(count, d->history.count);
    key(SDLK_D, SDL_KMOD_CTRL);
    TEST_ASSERT_EQUAL_INT(4, d->rectangle_count);
    TEST_ASSERT_EQUAL_INT(2, d->selection.count);
    key(SDLK_DELETE, 0);
    TEST_ASSERT_EQUAL_INT(2, d->rectangle_count);
    key(SDLK_V, SDL_KMOD_CTRL);
    TEST_ASSERT_EQUAL_INT(4, d->rectangle_count);
    key(SDLK_X, SDL_KMOD_CTRL);
    TEST_ASSERT_EQUAL_INT(2, d->rectangle_count);
    key(SDLK_V, SDL_KMOD_GUI);
    TEST_ASSERT_EQUAL_INT(4, d->rectangle_count);
}

static void test_multi_parent_duplicate_keeps_parents_and_offsets(void) {
    VectorDocument *d = document();
    TEST_ASSERT_TRUE(vector_artboard_add(d, (PUIRect){500, 20, 200, 100}));
    TEST_ASSERT_TRUE(vector_rectangle_add(d, 2, (PUIRect){10, 10, 40, 30}));
    vector_document_select(d, 1);
    vector_document_toggle(d, 3);
    TEST_ASSERT_TRUE(vector_objects_duplicate(d));
    TEST_ASSERT_EQUAL_INT(5, d->rectangle_count);
    TEST_ASSERT_EQUAL_INT(1, d->rectangles[3].parent_id);
    TEST_ASSERT_EQUAL_INT(2, d->rectangles[4].parent_id);
    TEST_ASSERT_EQUAL_FLOAT(46, d->rectangles[3].rect.x);
    TEST_ASSERT_EQUAL_FLOAT(26, d->rectangles[4].rect.x);
    TEST_ASSERT_EQUAL_INT(2, d->selection.count);
}

static void
test_tree_context_selects_row_without_collapsing_and_text_shortcuts_stay_in_input(void) {
    VectorDocument *d = document();
    click(SDL_BUTTON_RIGHT, 65, 130); // Existing selected member keeps the group.
    TEST_ASSERT_EQUAL_INT(2, d->selection.count);
    key(SDLK_ESCAPE, 0);
    click(SDL_BUTTON_RIGHT, 65, 102); // Artboard context selects the artboard, not its children.
    TEST_ASSERT_EQUAL_INT(0, d->selection.count);
    TEST_ASSERT_EQUAL_INT(1, d->active_artboard);
    key(SDLK_ESCAPE, 0);
    click(SDL_BUTTON_RIGHT, 65, 130);
    TEST_ASSERT_EQUAL_INT(1, d->selection.count);
    TEST_ASSERT_EQUAL_INT(1, d->active_rectangle);
    key(SDLK_ESCAPE, 0);
    click(SDL_BUTTON_LEFT, 1160, 138); // Rectangle name input.
    key(SDLK_A, SDL_KMOD_CTRL);
    key(SDLK_C, SDL_KMOD_CTRL);
    TEST_ASSERT_EQUAL_INT(0, ((VectorEditor *)host.instance)->object_actions.clipboard.board_count);
    key(SDLK_BACKSPACE, 0);
    TEST_ASSERT_EQUAL_INT(2, d->rectangle_count);
    TEST_ASSERT_EQUAL_STRING("", pui_view_text(shell.view, "rectangle_name"));
    key(SDLK_V, SDL_KMOD_CTRL);
    TEST_ASSERT_EQUAL_STRING("Rechteck 1", pui_view_text(shell.view, "rectangle_name"));
    TEST_ASSERT_EQUAL_INT(2, d->rectangle_count);
}

static void test_radial_disabled_paste_outside_click_and_focus_loss_are_safe(void) {
    VectorDocument *d = document();
    int count = d->history.count;
    click(SDL_BUTTON_RIGHT, 630, 400);
    click(SDL_BUTTON_LEFT, 691, 484); // Paste is unavailable in an empty clipboard.
    TEST_ASSERT_EQUAL_INT(count, d->history.count);
    TEST_ASSERT_EQUAL_INT(2, d->rectangle_count);
    click(SDL_BUTTON_RIGHT, 630, 400);
    click(SDL_BUTTON_LEFT, 280, 70); // Dismiss over add-document: no click-through.
    TEST_ASSERT_EQUAL_INT(1, ((VectorEditor *)host.instance)->documents.count);
    click(SDL_BUTTON_RIGHT, 630, 400);
    SDL_Event lost = {.type = SDL_EVENT_WINDOW_FOCUS_LOST};
    event(&lost);
    click(SDL_BUTTON_LEFT, 630, 296);
    TEST_ASSERT_EQUAL_INT(0, ((VectorEditor *)host.instance)->object_actions.clipboard.board_count);
    TEST_ASSERT_EQUAL_INT(count, d->history.count);
}

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_tree_context_selects_row_without_collapsing_and_text_shortcuts_stay_in_input);
    RUN_TEST(test_radial_disabled_paste_outside_click_and_focus_loss_are_safe);
    RUN_TEST(test_group_delete_undo_restores_order_ids_properties_and_selection);
    RUN_TEST(test_copy_across_tabs_owns_values_and_preserves_clipboard_after_editor_switch);
    RUN_TEST(test_artboard_children_cut_and_duplicate_are_atomic_and_clipboard_is_unchanged);
    RUN_TEST(test_locked_child_and_capacity_failure_do_not_delete_or_overwrite_clipboard);
    RUN_TEST(test_radial_context_click_escape_and_shortcuts);
    RUN_TEST(test_multi_parent_duplicate_keeps_parents_and_offsets);
    return UNITY_END();
}
