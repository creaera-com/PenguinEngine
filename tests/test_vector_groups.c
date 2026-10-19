#include "editor_host/host.h"
#include "editors/vector/document/selection.h"
#include "editors/vector/internal.h"
#include "editors/vector/object/group.h"
#include "editors/vector/rectangle/transform.h"
#include "editors/vector/ui/document_tree/groups.h"
#include "editors/vector/workspace/selection.h"
#include "shell/shell.h"
#include "unity.h"
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
    TEST_ASSERT_TRUE(vector_rectangle_add(d, 1, (PUIRect){50, 80, 20, 20}));
    TEST_ASSERT_TRUE(vector_rectangle_add(d, 1, (PUIRect){150, 140, 40, 40}));
    TEST_ASSERT_TRUE(vector_rectangle_add(d, 1, (PUIRect){250, 80, 30, 30}));
    vector_document_select(d, 1);
    vector_document_toggle(d, 3);

    commit();
}

void tearDown(void) {
    editor_shell_destroy(&shell);
    editor_host_destroy(&host);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
}

static void test_group_ungroup_preserves_values_order_and_undo_redo(void) {
    VectorDocument *d = document();
    VectorShape before[3];
    memcpy(before, d->rectangles, sizeof before);
    int history = d->history.count;
    TEST_ASSERT_TRUE(vector_objects_can_group(d));
    TEST_ASSERT_TRUE(vector_objects_group(d));
    TEST_ASSERT_EQUAL_INT(history + 1, d->history.count);
    TEST_ASSERT_EQUAL_UINT(2, d->rectangles[0].id);
    TEST_ASSERT_EQUAL_UINT(1, d->rectangles[1].id);
    TEST_ASSERT_EQUAL_UINT(3, d->rectangles[2].id);
    TEST_ASSERT_EQUAL_UINT(1, d->rectangles[1].group_id);
    TEST_ASSERT_EQUAL_UINT(1, d->rectangles[2].group_id);
    for (int i = 0; i < 3; i++) {
        VectorShape *r = vector_rectangle_find(d, before[i].id);
        TEST_ASSERT_EQUAL_MEMORY(&before[i].rect, &r->rect, sizeof r->rect);
        TEST_ASSERT_EQUAL_UINT(before[i].fill, r->fill);
    }
    TEST_ASSERT_FALSE(vector_objects_can_group(d));
    TEST_ASSERT_TRUE(vector_objects_can_ungroup(d));
    TEST_ASSERT_TRUE(vector_history_undo(&d->history, d));
    TEST_ASSERT_EQUAL_MEMORY(before, d->rectangles, sizeof before);
    TEST_ASSERT_TRUE(vector_history_redo(&d->history, d));
    TEST_ASSERT_TRUE(vector_objects_ungroup(d));
    for (int i = 0; i < 3; i++) {
        TEST_ASSERT_EQUAL_UINT(0, d->rectangles[i].group_id);
    }
    TEST_ASSERT_TRUE(vector_history_undo(&d->history, d));
    TEST_ASSERT_EQUAL_UINT(1, vector_rectangle_find(d, 1)->group_id);
    TEST_ASSERT_TRUE(vector_history_redo(&d->history, d));
    TEST_ASSERT_FALSE(vector_objects_can_ungroup(d));
}

static void test_click_toggle_and_group_translation_keep_members_together(void) {
    VectorDocument *d = document();
    TEST_ASSERT_TRUE(vector_objects_group(d));
    vector_document_select(d, 1);
    TEST_ASSERT_EQUAL_INT(2, d->selection.count);
    TEST_ASSERT_TRUE(vector_document_is_selected(d, 3));
    vector_document_toggle(d, 3);
    TEST_ASSERT_EQUAL_INT(0, d->selection.count);
    vector_document_toggle(d, 1);
    TEST_ASSERT_EQUAL_INT(2, d->selection.count);
    unsigned ids[] = {1, 3};
    TEST_ASSERT_TRUE(vector_rectangles_translate(d, ids, 2, 12, 23));
    TEST_ASSERT_EQUAL_FLOAT(62, vector_rectangle_find(d, 1)->rect.x);
    TEST_ASSERT_EQUAL_FLOAT(262, vector_rectangle_find(d, 3)->rect.x);
    TEST_ASSERT_EQUAL_FLOAT(150, vector_rectangle_find(d, 2)->rect.x);
    TEST_ASSERT_TRUE(vector_history_undo(&d->history, d));
    TEST_ASSERT_EQUAL_FLOAT(50, vector_rectangle_find(d, 1)->rect.x);
}

static void test_clipboard_paste_duplicate_and_editor_reload_keep_distinct_groups(void) {
    VectorDocument *d = document();
    TEST_ASSERT_TRUE(vector_objects_group(d));
    VectorClipboard clipboard;
    TEST_ASSERT_TRUE(vector_objects_copy(d, &clipboard));
    TEST_ASSERT_TRUE(vector_objects_duplicate(d));
    unsigned duplicate = vector_rectangle_find(d, d->active_rectangle)->group_id;
    TEST_ASSERT_TRUE(duplicate && duplicate != 1);
    TEST_ASSERT_EQUAL_INT(2, d->selection.count);
    TEST_ASSERT_TRUE(editor_host_action(&host, shell.view, "vector.add_document", ""));
    commit();
    d = document();
    TEST_ASSERT_TRUE(vector_artboard_add(d, (PUIRect){0, 0, 600, 400}));
    TEST_ASSERT_TRUE(vector_objects_paste(d, &clipboard));
    unsigned pasted = vector_rectangle_find(d, d->active_rectangle)->group_id;
    TEST_ASSERT_TRUE(pasted);
    TEST_ASSERT_EQUAL_INT(2, d->selection.count);
    activate("scripts");
    activate("vector");
    commit();
    d = document();
    TEST_ASSERT_EQUAL_UINT(pasted, vector_rectangle_find(d, d->active_rectangle)->group_id);
    TEST_ASSERT_EQUAL_INT(1, vector_group_tree_count(d));
    TEST_ASSERT_TRUE(vector_objects_ungroup(d));
    TEST_ASSERT_TRUE(vector_history_undo(&d->history, d));
    TEST_ASSERT_EQUAL_INT(1, vector_group_tree_count(d));
}

static void test_guards_reject_cross_artboard_partial_and_locked_selections(void) {
    VectorDocument *d = document();
    vector_document_select(d, 1);
    TEST_ASSERT_FALSE(vector_objects_group(d));
    TEST_ASSERT_FALSE(vector_objects_ungroup(d));
    vector_document_toggle(d, 3);
    TEST_ASSERT_TRUE(vector_rectangle_set_state(d, 3, VECTOR_OBJECT_LOCKED));
    TEST_ASSERT_FALSE(vector_objects_group(d));
    TEST_ASSERT_TRUE(vector_history_undo(&d->history, d));
    TEST_ASSERT_TRUE(vector_objects_group(d));
    vector_document_select(d, 1);
    d->selection.count = 1;
    d->selection.ids[0] = 1;
    TEST_ASSERT_FALSE(vector_objects_ungroup(d));
    vector_document_selection_sync(d);
    TEST_ASSERT_EQUAL_INT(2, d->selection.count);
    TEST_ASSERT_TRUE(vector_artboard_add(d, (PUIRect){700, 0, 100, 100}));
    TEST_ASSERT_TRUE(vector_rectangle_add(d, 2, (PUIRect){0, 0, 20, 20}));
    vector_document_select(d, 1);
    vector_document_toggle(d, 4);
    TEST_ASSERT_FALSE(vector_objects_group(d));
}

static void click(int button, float x, float y) {
    SDL_Event e = {.type = SDL_EVENT_MOUSE_BUTTON_DOWN};
    e.button.button = button;
    e.button.x = x;
    e.button.y = y;
    if (!pui_view_event(shell.view, &shell.input, window, &e)) {
        editor_host_event(&host, shell.view, &e);
    }
    commit();
    e.type = SDL_EVENT_MOUSE_BUTTON_UP;
    if (!pui_view_event(shell.view, &shell.input, window, &e)) {
        editor_host_event(&host, shell.view, &e);
    }
    commit();
}

static void menu(const char *id) {
    float x, y;
    TEST_ASSERT_TRUE(pui_view_radial_item_center(shell.view, id, &x, &y));
    click(SDL_BUTTON_LEFT, x, y);
}

static void test_native_radial_actions_and_group_tree_selection(void) {
    click(SDL_BUTTON_RIGHT, 630, 400);
    menu("vector_layers_menu");
    menu("vector_objects_group");
    commit();
    VectorDocument *d = document();
    TEST_ASSERT_EQUAL_INT(1, vector_group_tree_count(d));
    TEST_ASSERT_EQUAL_INT(5, pui_view_tree_item_count(shell.view, "vector_document_tree"));
    vector_document_select(d, 2);
    TEST_ASSERT_TRUE(
        editor_host_action(&host, shell.view, "vector.group.select", "vector_document_1.group_1"));
    TEST_ASSERT_EQUAL_INT(2, d->selection.count);
    click(SDL_BUTTON_RIGHT, 630, 400);
    menu("vector_layers_menu");
    menu("vector_objects_ungroup");
    commit();
    TEST_ASSERT_EQUAL_INT(0, vector_group_tree_count(d));
    TEST_ASSERT_EQUAL_INT(4, pui_view_tree_item_count(shell.view, "vector_document_tree"));
}

static void test_combining_groups_and_delete_are_atomic(void) {
    VectorDocument *d = document();
    TEST_ASSERT_TRUE(vector_objects_group(d));
    TEST_ASSERT_TRUE(vector_rectangle_add(d, 1, (PUIRect){350, 80, 30, 30}));
    TEST_ASSERT_TRUE(vector_rectangle_add(d, 1, (PUIRect){400, 80, 30, 30}));
    vector_document_select(d, 4);
    vector_document_toggle(d, 5);
    TEST_ASSERT_TRUE(vector_objects_group(d));
    TEST_ASSERT_EQUAL_INT(2, vector_group_tree_count(d));
    vector_document_toggle(d, 1);
    TEST_ASSERT_EQUAL_INT(4, d->selection.count);
    TEST_ASSERT_TRUE(vector_objects_group(d));
    TEST_ASSERT_EQUAL_INT(1, vector_group_tree_count(d));
    TEST_ASSERT_TRUE(vector_history_undo(&d->history, d));
    TEST_ASSERT_EQUAL_INT(2, vector_group_tree_count(d));
    TEST_ASSERT_TRUE(vector_history_redo(&d->history, d));
    TEST_ASSERT_TRUE(vector_objects_delete(d));
    TEST_ASSERT_EQUAL_INT(1, d->rectangle_count);
    TEST_ASSERT_EQUAL_INT(0, vector_group_tree_count(d));
    TEST_ASSERT_TRUE(vector_history_undo(&d->history, d));
    TEST_ASSERT_EQUAL_INT(5, d->rectangle_count);
    TEST_ASSERT_EQUAL_INT(1, vector_group_tree_count(d));
}

static void test_locked_member_keeps_group_complete_and_blocks_partial_edits(void) {
    VectorDocument *d = document();
    TEST_ASSERT_TRUE(vector_objects_group(d));
    TEST_ASSERT_TRUE(vector_rectangle_set_state(d, 3, VECTOR_OBJECT_LOCKED));
    vector_document_select(d, 1);
    vector_document_selection_sync(d);
    TEST_ASSERT_EQUAL_INT(2, d->selection.count);
    TEST_ASSERT_FALSE(vector_objects_can_delete(d));
    TEST_ASSERT_FALSE(vector_objects_can_ungroup(d));
    VectorShape first = *vector_rectangle_find(d, 1);
    unsigned ids[] = {1, 3};
    TEST_ASSERT_FALSE(vector_rectangles_translate(d, ids, 2, 100, 0));
    TEST_ASSERT_EQUAL_MEMORY(&first, vector_rectangle_find(d, 1), sizeof first);
}

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_combining_groups_and_delete_are_atomic);
    RUN_TEST(test_locked_member_keeps_group_complete_and_blocks_partial_edits);
    RUN_TEST(test_group_ungroup_preserves_values_order_and_undo_redo);
    RUN_TEST(test_click_toggle_and_group_translation_keep_members_together);
    RUN_TEST(test_clipboard_paste_duplicate_and_editor_reload_keep_distinct_groups);
    RUN_TEST(test_guards_reject_cross_artboard_partial_and_locked_selections);
    RUN_TEST(test_native_radial_actions_and_group_tree_selection);
    return UNITY_END();
}
