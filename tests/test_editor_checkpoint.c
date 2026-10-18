#include "editor_host/checkpoint.h"
#include "editor_host/host.h"
#include "editors/vector/adapter.h"
#include "editors/vector/artboard/tool.h"
#include "editors/vector/internal.h"
#include "editors/vector/persistence/checkpoint.h"
#include "platform/temp_file.h"
#include "shell/shell.h"
#include "unity.h"
#include <string.h>

static SDL_Window *window;
static SDL_Renderer *renderer;
static EditorShell shell;
static EditorHost host;

static void frame(void) {
    pui_view_draw(shell.view, &shell.input, (PUIRect){0, 0, 1280, 800}, NULL, NULL);
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

static VectorDocument *add_document(void) {
    TEST_ASSERT_TRUE(editor_host_action(&host, shell.view, "vector.add_document", ""));
    commit();
    VectorEditor *editor = host.instance;
    return vector_document_tabs_active(&editor->documents, shell.view);
}

void setUp(void) {
    TEST_ASSERT_TRUE(SDL_Init(SDL_INIT_VIDEO));
    window = SDL_CreateWindow("checkpoint", 1280, 800, SDL_WINDOW_HIDDEN);
    renderer = SDL_CreateRenderer(window, NULL);
    TEST_ASSERT_NOT_NULL(renderer);
    char path[1600], error[256];
    snprintf(path, sizeof path, "%s/shell.yaml", PENGUI_EDITOR_RESOURCES);
    TEST_ASSERT_TRUE_MESSAGE(editor_shell_load(&shell, renderer, path, error, sizeof error), error);
    editor_host_init(&host, (EditorServices){PENGUI_EDITOR_RESOURCES});
    activate("vector");
}

void tearDown(void) {
    editor_shell_destroy(&shell);
    editor_host_destroy(&host);
    TEST_ASSERT_NULL(host.checkpoints);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
}

static void test_switch_restores_documents_histories_viewports_and_latest_edits(void) {
    VectorDocument *first = add_document();
    TEST_ASSERT_TRUE(vector_artboard_add(first, (PUIRect){-10, 32, 250, 70}));
    TEST_ASSERT_TRUE(vector_artboard_add(first, (PUIRect){90, -40, 33, 125}));
    TEST_ASSERT_TRUE(vector_history_undo(&first->history, first));
    first->viewport.x = -123;
    first->viewport.y = 78;
    first->viewport.zoom = 2.5f;
    VectorDocument *second = add_document();
    TEST_ASSERT_TRUE(vector_artboard_add(second, (PUIRect){1, 2, 3, 4}));
    vector_artboard_tool_arm(second);
    second->artboard_tool.dragging = true;
    second->artboard_tool.start_x = 500;
    unsigned next = first->next_artboard_id;
    activate("scripts");
    TEST_ASSERT_EQUAL_STRING("scripts", host.active->id);
    TEST_ASSERT_NOT_NULL(editor_checkpoint_find(host.checkpoints, vector_editor_definition()));
    activate("game");
    activate("vector");
    VectorEditor *editor = host.instance;
    TEST_ASSERT_EQUAL_INT(2, editor->documents.count);
    first = &editor->documents.items[0];
    second = &editor->documents.items[1];
    TEST_ASSERT_EQUAL_STRING("vector_document_2",
                             pui_view_selected_tab(shell.view, "vector_document_tabs"));
    TEST_ASSERT_EQUAL_INT(1, first->artboard_count);
    TEST_ASSERT_EQUAL_FLOAT(-10, first->artboards[0].rect.x);
    TEST_ASSERT_EQUAL_FLOAT(250, first->artboards[0].rect.w);
    TEST_ASSERT_EQUAL_UINT(next, first->next_artboard_id);
    TEST_ASSERT_EQUAL_FLOAT(-123, first->viewport.x);
    TEST_ASSERT_EQUAL_FLOAT(78, first->viewport.y);
    TEST_ASSERT_EQUAL_FLOAT(2.5f, first->viewport.zoom);
    TEST_ASSERT_EQUAL_INT(2, first->history.count);
    TEST_ASSERT_EQUAL_INT(1, first->history.cursor);
    TEST_ASSERT_FALSE(second->artboard_tool.enabled);
    TEST_ASSERT_FALSE(second->artboard_tool.dragging);
    TEST_ASSERT_TRUE(vector_history_redo(&first->history, first));
    TEST_ASSERT_EQUAL_INT(2, first->artboard_count);
    TEST_ASSERT_EQUAL_FLOAT(-40, first->artboards[1].rect.y);
    TEST_ASSERT_EQUAL_FLOAT(125, first->artboards[1].rect.h);
    TEST_ASSERT_TRUE(editor_host_action(&host, shell.view, "editor.undo", ""));
    TEST_ASSERT_EQUAL_INT(0, second->artboard_count);
    TEST_ASSERT_EQUAL_INT(2, first->artboard_count);
    for (int i = 0; i < 3; i++) {
        activate("scripts");
        activate("vector");
        editor = host.instance;
        second = &editor->documents.items[1];
        TEST_ASSERT_EQUAL_INT(0, second->artboard_count);
        TEST_ASSERT_EQUAL_INT(0, second->history.cursor);
        TEST_ASSERT_EQUAL_INT(1, second->history.count);
    }
    TEST_ASSERT_TRUE(editor_host_action(&host, shell.view, "editor.redo", ""));
    TEST_ASSERT_EQUAL_INT(1, second->artboard_count);
    TEST_ASSERT_TRUE(vector_history_undo(&second->history, second));
    TEST_ASSERT_TRUE(vector_artboard_add(second, (PUIRect){5, 6, 7, 8}));
    activate("game");
    activate("vector");
    editor = host.instance;
    second = &editor->documents.items[1];
    TEST_ASSERT_FALSE(vector_history_redo(&second->history, second));
    TEST_ASSERT_EQUAL_UINT(2, second->artboards[0].id);
    TEST_ASSERT_EQUAL_FLOAT(7, second->artboards[0].rect.w);
}

static void test_checkpoint_retains_dragged_tab_order_and_overview_selection(void) {
    add_document();
    add_document();
    pui_pointer(&shell.input, 370, 108, true);
    frame();
    pui_pointer(&shell.input, 265, 108, true);
    frame();
    pui_pointer(&shell.input, 265, 108, false);
    frame();
    TEST_ASSERT_EQUAL_STRING("vector_document_1",
                             pui_view_tab_at(shell.view, "vector_document_tabs", 0));
    TEST_ASSERT_EQUAL_STRING("vector_workspace_tab",
                             pui_view_tab_at(shell.view, "vector_document_tabs", 1));
    activate("scripts");
    activate("vector");
    TEST_ASSERT_EQUAL_STRING("vector_document_1",
                             pui_view_selected_tab(shell.view, "vector_document_tabs"));
    TEST_ASSERT_EQUAL_STRING("vector_document_1",
                             pui_view_tab_at(shell.view, "vector_document_tabs", 0));
    TEST_ASSERT_EQUAL_STRING("vector_workspace_tab",
                             pui_view_tab_at(shell.view, "vector_document_tabs", 1));
    pui_pointer(&shell.input, 385, 108, true);
    pui_pointer(&shell.input, 385, 108, false);
    frame();
    TEST_ASSERT_EQUAL_STRING("vector_workspace_tab",
                             pui_view_selected_tab(shell.view, "vector_document_tabs"));
    activate("game");
    activate("vector");
    TEST_ASSERT_EQUAL_STRING("vector_workspace_tab",
                             pui_view_selected_tab(shell.view, "vector_document_tabs"));
}

static void test_reload_preserves_document_and_pending_creation_is_flushed(void) {
    VectorDocument *document = add_document();
    TEST_ASSERT_TRUE(vector_artboard_add(document, (PUIRect){8, 9, 10, 11}));
    TEST_ASSERT_TRUE(editor_host_action(&host, shell.view, "vector.add_document", ""));
    TEST_ASSERT_TRUE(editor_host_request(&host, "vector", true));
    commit();
    VectorEditor *editor = host.instance;
    TEST_ASSERT_EQUAL_INT(2, editor->documents.count);
    TEST_ASSERT_EQUAL_INT(1, editor->documents.items[0].artboard_count);
    TEST_ASSERT_EQUAL_INT(1, editor->documents.items[0].history.cursor);
    TEST_ASSERT_EQUAL_INT(3, pui_view_tab_count(shell.view, "vector_document_tabs"));
    activate("scripts");
    activate("vector");
    TEST_ASSERT_TRUE(editor_host_action(&host, shell.view, "vector.add_document", ""));
    commit();
    TEST_ASSERT_EQUAL_STRING("vector_document_3",
                             pui_view_selected_tab(shell.view, "vector_document_tabs"));
}

static void test_corrupt_checkpoint_keeps_current_editor_and_previous_snapshot(void) {
    add_document();
    activate("scripts");
    FILE *file = editor_checkpoint_find(host.checkpoints, vector_editor_definition());
    TEST_ASSERT_NOT_NULL(file);
    rewind(file);
    TEST_ASSERT_EQUAL_INT('P', fgetc(file));
    rewind(file);
    TEST_ASSERT_NOT_EQUAL(EOF, fputc('X', file));
    TEST_ASSERT_EQUAL_INT(0, fflush(file));
    void *current = host.instance;
    TEST_ASSERT_TRUE(editor_host_request(&host, "vector", false));
    char error[256];
    TEST_ASSERT_FALSE(editor_host_commit(&host, shell.view, &shell.input, error, sizeof error));
    TEST_ASSERT_EQUAL_PTR(current, host.instance);
    TEST_ASSERT_EQUAL_STRING("scripts", host.active->id);
    TEST_ASSERT_NOT_EQUAL(0, strlen(error));
    rewind(file);
    TEST_ASSERT_NOT_EQUAL(EOF, fputc('P', file));
    TEST_ASSERT_EQUAL_INT(0, fflush(file));
    activate("vector");
    TEST_ASSERT_EQUAL_INT(2, pui_view_tab_count(shell.view, "vector_document_tabs"));
}

static bool reject_snapshot(const void *instance, const PUIView *view, FILE *file, char *error,
                            size_t capacity) {
    (void)instance;
    (void)view;
    (void)file;
    snprintf(error, capacity, "simulated write failure");
    return false;
}

static void test_failed_checkpoint_and_failed_mount_do_not_discard_active_document(void) {
    VectorDocument *document = add_document();
    TEST_ASSERT_TRUE(vector_artboard_add(document, (PUIRect){0, 0, 12, 34}));
    const EditorDefinition *original = host.active;
    EditorDefinition failure = *original;
    failure.checkpoint = reject_snapshot;
    host.active = &failure;
    TEST_ASSERT_TRUE(editor_host_request(&host, "scripts", false));
    char error[256];
    TEST_ASSERT_FALSE(editor_host_commit(&host, shell.view, &shell.input, error, sizeof error));
    TEST_ASSERT_EQUAL_PTR(&failure, host.active);
    TEST_ASSERT_EQUAL_INT(1, document->artboard_count);
    TEST_ASSERT_NULL(host.checkpoints);
    host.active = original;
    const char *yaml = "schema: pengui/v1\nroot: {type: label, text: no_slot}";
    PUILayout *base = pui_layout_parse(yaml, strlen(yaml), error, sizeof error);
    PUIView *without_slot = pui_view_new(base, renderer);
    TEST_ASSERT_TRUE(editor_host_request(&host, "scripts", false));
    /* Snapshot cannot find its document tabs in an unrelated view: leave the active instance
     * intact. */
    TEST_ASSERT_FALSE(editor_host_commit(&host, without_slot, &shell.input, error, sizeof error));
    TEST_ASSERT_EQUAL_PTR(original, host.active);
    TEST_ASSERT_EQUAL_INT(1, document->artboard_count);
    pui_view_free(without_slot);
    pui_layout_free(base);
    activate("scripts");
    activate("vector");
    VectorEditor *editor = host.instance;
    TEST_ASSERT_EQUAL_INT(1, editor->documents.items[0].artboard_count);
}

static void test_session_is_not_carried_into_a_new_host(void) {
    add_document();
    activate("scripts");
    TEST_ASSERT_NOT_NULL(host.checkpoints);
    editor_host_destroy(&host);
    TEST_ASSERT_NULL(host.checkpoints);
    editor_host_init(&host, (EditorServices){PENGUI_EDITOR_RESOURCES});
    activate("vector");
    TEST_ASSERT_EQUAL_INT(1, pui_view_tab_count(shell.view, "vector_document_tabs"));
}

static void test_truncated_checkpoint_releases_partial_history_and_retains_model(void) {
    VectorDocument *document = add_document();
    TEST_ASSERT_TRUE(vector_artboard_add(document, (PUIRect){10, 20, 30, 40}));
    TEST_ASSERT_TRUE(vector_artboard_add(document, (PUIRect){40, 50, 60, 70}));
    FILE *file = editor_temporary_file();
    TEST_ASSERT_NOT_NULL(file);
    char error[256];
    TEST_ASSERT_TRUE(
        vector_editor_checkpoint(host.instance, shell.view, file, error, sizeof error));
    TEST_ASSERT_EQUAL_INT(0, fflush(file));
    long length = ftell(file);
    TEST_ASSERT_TRUE(length > 20);
    FILE *truncated = editor_temporary_file();
    TEST_ASSERT_NOT_NULL(truncated);
    rewind(file);
    for (long i = 0; i < length - 4; i++) {
        int value = fgetc(file);
        TEST_ASSERT_NOT_EQUAL(EOF, value);
        TEST_ASSERT_NOT_EQUAL(EOF, fputc(value, truncated));
    }
    TEST_ASSERT_EQUAL_INT(0, fflush(truncated));
    rewind(truncated);
    const EditorDefinition *definition = vector_editor_definition();
    void *candidate = definition->create(&host.services, error, sizeof error);
    TEST_ASSERT_NOT_NULL(candidate);
    TEST_ASSERT_FALSE(definition->restore(candidate, truncated, error, sizeof error));
    VectorEditor *empty = candidate;
    TEST_ASSERT_EQUAL_INT(0, empty->documents.count);
    definition->destroy(candidate);
    TEST_ASSERT_EQUAL_INT(2, document->artboard_count);
    TEST_ASSERT_EQUAL_INT(2, document->history.count);
    fclose(truncated);
    fclose(file);
}

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_truncated_checkpoint_releases_partial_history_and_retains_model);
    RUN_TEST(test_switch_restores_documents_histories_viewports_and_latest_edits);
    RUN_TEST(test_checkpoint_retains_dragged_tab_order_and_overview_selection);
    RUN_TEST(test_reload_preserves_document_and_pending_creation_is_flushed);
    RUN_TEST(test_corrupt_checkpoint_keeps_current_editor_and_previous_snapshot);
    RUN_TEST(test_failed_checkpoint_and_failed_mount_do_not_discard_active_document);
    RUN_TEST(test_session_is_not_carried_into_a_new_host);
    return UNITY_END();
}
