#include "editor_host/checkpoint.h"
#include "editor_host/host.h"
#include "editors/vector/adapter.h"
#include "editors/vector/artboard/selection.h"
#include "editors/vector/artboard/tool.h"
#include "editors/vector/geometry/resize.h"
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

static void test_checkpoint_retains_dragged_document_order_and_selection(void) {
    add_document();
    add_document();
    pui_pointer(&shell.input, 410, 108, true);
    frame();
    pui_pointer(&shell.input, 265, 108, true);
    frame();
    pui_pointer(&shell.input, 265, 108, false);
    frame();
    TEST_ASSERT_EQUAL_STRING("vector_document_2",
                             pui_view_tab_at(shell.view, "vector_document_tabs", 0));
    TEST_ASSERT_EQUAL_STRING("vector_document_1",
                             pui_view_tab_at(shell.view, "vector_document_tabs", 1));
    activate("scripts");
    activate("vector");
    TEST_ASSERT_EQUAL_STRING("vector_document_2",
                             pui_view_selected_tab(shell.view, "vector_document_tabs"));
    TEST_ASSERT_EQUAL_STRING("vector_document_2",
                             pui_view_tab_at(shell.view, "vector_document_tabs", 0));
    TEST_ASSERT_EQUAL_STRING("vector_document_1",
                             pui_view_tab_at(shell.view, "vector_document_tabs", 1));
    pui_pointer(&shell.input, 410, 108, true);
    pui_pointer(&shell.input, 410, 108, false);
    frame();
    TEST_ASSERT_EQUAL_STRING("vector_document_1",
                             pui_view_selected_tab(shell.view, "vector_document_tabs"));
    activate("game");
    activate("vector");
    TEST_ASSERT_EQUAL_STRING("vector_document_1",
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
    TEST_ASSERT_EQUAL_INT(2, pui_view_tab_count(shell.view, "vector_document_tabs"));
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
    TEST_ASSERT_EQUAL_INT(1, pui_view_tab_count(shell.view, "vector_document_tabs"));
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
    TEST_ASSERT_EQUAL_INT(0, pui_view_tab_count(shell.view, "vector_document_tabs"));
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

static void test_document_tree_tracks_changes_documents_and_checkpoint_restore(void) {
    VectorDocument *document = add_document();
    TEST_ASSERT_EQUAL_INT(0, pui_view_tree_item_count(shell.view, "vector_document_tree"));
    TEST_ASSERT_TRUE(vector_artboard_add(document, (PUIRect){1, 2, 3, 4}));
    TEST_ASSERT_TRUE(vector_artboard_add(document, (PUIRect){5, 6, 7, 8}));
    commit();
    TEST_ASSERT_EQUAL_INT(2, pui_view_tree_item_count(shell.view, "vector_document_tree"));
    TEST_ASSERT_TRUE(editor_host_action(&host, shell.view, "editor.undo", ""));
    commit();
    TEST_ASSERT_EQUAL_INT(1, pui_view_tree_item_count(shell.view, "vector_document_tree"));
    TEST_ASSERT_TRUE(editor_host_action(&host, shell.view, "editor.redo", ""));
    commit();
    TEST_ASSERT_EQUAL_INT(2, pui_view_tree_item_count(shell.view, "vector_document_tree"));
    activate("scripts");
    activate("vector");
    commit();
    TEST_ASSERT_EQUAL_INT(2, pui_view_tree_item_count(shell.view, "vector_document_tree"));
    for (int i = 1; i < VECTOR_MAX_DOCUMENTS; i++) {
        document = add_document();
        TEST_ASSERT_EQUAL_INT(0, pui_view_tree_item_count(shell.view, "vector_document_tree"));
    }
    for (int i = 0; i < VECTOR_MAX_ARTBOARDS; i++) {
        TEST_ASSERT_TRUE(vector_artboard_add(document, (PUIRect){i * 10.f, 0, 5, 5}));
    }
    commit();
    TEST_ASSERT_EQUAL_INT(VECTOR_MAX_ARTBOARDS,
                          pui_view_tree_item_count(shell.view, "vector_document_tree"));
    activate("game");
    activate("vector");
    commit();
    TEST_ASSERT_EQUAL_INT(VECTOR_MAX_ARTBOARDS,
                          pui_view_tree_item_count(shell.view, "vector_document_tree"));
}

static void test_navigation_tab_selection_survives_document_changes_editor_switch_and_reload(void) {
    TEST_ASSERT_EQUAL_STRING("vector_document_tree_tab",
                             pui_view_selected_tab(shell.view, "vector_navigation_tabs"));
    add_document();
    TEST_ASSERT_EQUAL_STRING("vector_document_tree_tab",
                             pui_view_selected_tab(shell.view, "vector_navigation_tabs"));
    add_document();
    TEST_ASSERT_EQUAL_STRING("vector_document_tree_tab",
                             pui_view_selected_tab(shell.view, "vector_navigation_tabs"));
    pui_pointer(&shell.input, 300, 108, true);
    pui_pointer(&shell.input, 300, 108, false);
    frame();
    commit();
    TEST_ASSERT_EQUAL_STRING("vector_document_1",
                             pui_view_selected_tab(shell.view, "vector_document_tabs"));
    TEST_ASSERT_EQUAL_STRING("vector_document_tree_tab",
                             pui_view_selected_tab(shell.view, "vector_navigation_tabs"));
    activate("scripts");
    activate("vector");
    TEST_ASSERT_EQUAL_STRING("vector_document_tree_tab",
                             pui_view_selected_tab(shell.view, "vector_navigation_tabs"));
    TEST_ASSERT_TRUE(editor_host_request(&host, "vector", true));
    commit();
    TEST_ASSERT_EQUAL_STRING("vector_document_tree_tab",
                             pui_view_selected_tab(shell.view, "vector_navigation_tabs"));
    pui_pointer(&shell.input, 50, 65, true);
    pui_pointer(&shell.input, 50, 65, false);
    frame();
    activate("game");
    activate("vector");
    TEST_ASSERT_EQUAL_STRING("vector_explorer_tab",
                             pui_view_selected_tab(shell.view, "vector_navigation_tabs"));
}

static void test_empty_workspace_gets_one_default_document_without_duplicates(void) {
    /* Initial mounting is followed by the first between-frame UI commit. */
    commit();
    TEST_ASSERT_EQUAL_INT(1, pui_view_tab_count(shell.view, "vector_document_tabs"));
    TEST_ASSERT_EQUAL_STRING("vector_document_1",
                             pui_view_selected_tab(shell.view, "vector_document_tabs"));
    TEST_ASSERT_EQUAL_STRING("vector_document_tree_tab",
                             pui_view_selected_tab(shell.view, "vector_navigation_tabs"));
    commit();
    TEST_ASSERT_EQUAL_INT(1, pui_view_tab_count(shell.view, "vector_document_tabs"));
    VectorEditor *editor = host.instance;
    TEST_ASSERT_EQUAL_INT(0, editor->documents.items[0].artboard_count);
    TEST_ASSERT_EQUAL_INT(0, editor->documents.items[0].history.count);
    activate("scripts");
    activate("vector");
    commit();
    TEST_ASSERT_EQUAL_INT(1, pui_view_tab_count(shell.view, "vector_document_tabs"));
    TEST_ASSERT_FALSE(pui_view_checked(shell.view, "vector_add_artboard"));
    TEST_ASSERT_EQUAL_INT(0, pui_view_tree_item_count(shell.view, "vector_document_tree"));
    TEST_ASSERT_TRUE(editor_host_request(&host, "vector", true));
    commit();
    TEST_ASSERT_EQUAL_INT(1, pui_view_tab_count(shell.view, "vector_document_tabs"));
    add_document();
    TEST_ASSERT_EQUAL_INT(2, pui_view_tab_count(shell.view, "vector_document_tabs"));
}

static void test_empty_restored_checkpoint_gets_a_default_document(void) {
    char error[256];
    EditorCheckpoint *empty = NULL;
    TEST_ASSERT_TRUE(editor_checkpoint_capture(host.active, host.instance, shell.view, &empty,
                                               error, sizeof error));
    TEST_ASSERT_NOT_NULL(empty);
    activate("scripts");
    editor_checkpoint_publish(&host.checkpoints, empty);
    activate("vector");
    commit();
    TEST_ASSERT_EQUAL_INT(1, pui_view_tab_count(shell.view, "vector_document_tabs"));
    TEST_ASSERT_EQUAL_STRING("vector_document_1",
                             pui_view_selected_tab(shell.view, "vector_document_tabs"));
    commit();
    TEST_ASSERT_EQUAL_INT(1, pui_view_tab_count(shell.view, "vector_document_tabs"));
}

static void property(const char *id, const char *value, const char *action) {
    TEST_ASSERT_TRUE(pui_view_set_text(shell.view, id, value));
    TEST_ASSERT_TRUE(editor_host_action(&host, shell.view, action, id));
    commit();
}

static void test_artboard_inspector_updates_history_and_survives_editor_switch(void) {
    VectorDocument *document = add_document();
    TEST_ASSERT_NULL(pui_view_text(shell.view, "artboard_name"));
    TEST_ASSERT_TRUE(vector_artboard_add(document, (PUIRect){10, 20, 200, 300}));
    commit();
    TEST_ASSERT_EQUAL_STRING("Artboard 1", pui_view_text(shell.view, "artboard_name"));
    property("artboard_name", "Menu", "vector.artboard.name");
    TEST_ASSERT_EQUAL_STRING("Dokument 1", pui_view_text(shell.view, "vector_document_1"));
    property("artboard_width", "640", "vector.artboard.width");
    property("artboard_height", "480", "vector.artboard.height");
    TEST_ASSERT_EQUAL_STRING("Menu", document->artboards[0].name);
    TEST_ASSERT_EQUAL_FLOAT(640, document->artboards[0].rect.w);
    TEST_ASSERT_EQUAL_FLOAT(480, document->artboards[0].rect.h);
    int count = document->history.count;
    property("artboard_width", "1e30", "vector.artboard.width");
    property("artboard_width", "nan", "vector.artboard.width");
    property("artboard_width", "0", "vector.artboard.width");
    property("artboard_height", "12junk", "vector.artboard.height");
    property("artboard_name", "", "vector.artboard.name");
    TEST_ASSERT_EQUAL_INT(count, document->history.count);
    TEST_ASSERT_EQUAL_STRING("640", pui_view_text(shell.view, "artboard_width"));
    TEST_ASSERT_EQUAL_STRING("Menu", pui_view_text(shell.view, "artboard_name"));
    TEST_ASSERT_TRUE(editor_host_action(&host, shell.view, "editor.undo", ""));
    commit();
    TEST_ASSERT_EQUAL_STRING("300", pui_view_text(shell.view, "artboard_height"));
    activate("scripts");
    activate("vector");
    commit();
    VectorEditor *editor = host.instance;
    document = &editor->documents.items[0];
    TEST_ASSERT_EQUAL_STRING("Menu", document->artboards[0].name);
    TEST_ASSERT_EQUAL_FLOAT(640, document->artboards[0].rect.w);
    TEST_ASSERT_EQUAL_FLOAT(300, document->artboards[0].rect.h);
    TEST_ASSERT_TRUE(editor_host_action(&host, shell.view, "editor.redo", ""));
    commit();
    TEST_ASSERT_EQUAL_STRING("480", pui_view_text(shell.view, "artboard_height"));
}

static void test_artboard_selection_and_tool_toggle(void) {
    VectorDocument *document = add_document();
    TEST_ASSERT_TRUE(editor_host_action(&host, shell.view, "vector.add_artboard", ""));
    TEST_ASSERT_TRUE(document->artboard_tool.enabled);
    TEST_ASSERT_TRUE(editor_host_action(&host, shell.view, "vector.add_artboard", ""));
    TEST_ASSERT_FALSE(document->artboard_tool.enabled);
    TEST_ASSERT_TRUE(vector_artboard_add(document, (PUIRect){10, 20, 200, 300}));
    TEST_ASSERT_TRUE(vector_artboard_add(document, (PUIRect){250, 20, 200, 300}));
    commit();
    TEST_ASSERT_TRUE(editor_host_action(&host, shell.view, "vector.artboard.select",
                                        "vector_document_1.artboard_1"));
    commit();
    TEST_ASSERT_EQUAL_UINT(1, document->active_artboard);
    TEST_ASSERT_EQUAL_STRING("Artboard 1", pui_view_text(shell.view, "artboard_name"));
    property("artboard_name", "First", "vector.artboard.name");
    TEST_ASSERT_TRUE(editor_host_action(&host, shell.view, "vector.add_artboard", ""));
    TEST_ASSERT_TRUE(editor_host_action(&host, shell.view, "vector.tool_selection", ""));
    TEST_ASSERT_FALSE(document->artboard_tool.enabled);
    activate("scripts");
    activate("vector");
    commit();
    VectorEditor *editor = host.instance;
    document = &editor->documents.items[0];
    TEST_ASSERT_EQUAL_UINT(1, document->active_artboard);
    TEST_ASSERT_EQUAL_STRING("First", pui_view_text(shell.view, "artboard_name"));
    TEST_ASSERT_TRUE(editor_host_action(&host, shell.view, "editor.undo", ""));
    commit();
    TEST_ASSERT_EQUAL_STRING("Artboard 1", pui_view_text(shell.view, "artboard_name"));
    document->active_artboard = 0;
    commit();
    TEST_ASSERT_NULL(pui_view_text(shell.view, "artboard_name"));
}

static void dispatch_property(void *context, const char *name, const char *widget) {
    (void)context;
    TEST_ASSERT_TRUE(editor_host_action(&host, shell.view, name, widget));
}

static void native_input(SDL_Event *event) {
    TEST_ASSERT_TRUE(pui_view_event(shell.view, &shell.input, window, event));
    pui_view_draw(shell.view, &shell.input, (PUIRect){0, 0, 1280, 800}, dispatch_property, NULL);
    commit();
}

static void test_tab_commits_properties_and_preserves_next_field_selection(void) {
    VectorDocument *document = add_document();
    TEST_ASSERT_TRUE(vector_artboard_add(document, (PUIRect){10, 20, 200, 300}));
    commit();
    SDL_Event key = {.type = SDL_EVENT_KEY_DOWN};
    key.key.key = SDLK_TAB;
    native_input(&key); // First visible input: name, selected for replacement.
    SDL_Event text = {.type = SDL_EVENT_TEXT_INPUT};
    text.text.text = "Renamed";
    native_input(&text);
    native_input(&key); // Name -> width commits name.
    TEST_ASSERT_EQUAL_STRING("Renamed", document->artboards[0].name);
    TEST_ASSERT_EQUAL_STRING("Dokument 1", pui_view_text(shell.view, "vector_document_1"));
    text.text.text = "640";
    native_input(&text);
    native_input(&key); // Width -> height commits width.
    TEST_ASSERT_EQUAL_FLOAT(640, document->artboards[0].rect.w);
    text.text.text = "480";
    native_input(&text);
    key.key.mod = SDL_KMOD_SHIFT;
    native_input(&key); // Shift+Tab commits height.
    TEST_ASSERT_EQUAL_FLOAT(480, document->artboards[0].rect.h);
    TEST_ASSERT_TRUE(editor_host_action(&host, shell.view, "editor.undo", ""));
    commit();
    TEST_ASSERT_EQUAL_FLOAT(300, document->artboards[0].rect.h);
}

static void selection_pointer(SDL_EventType type, float x, float y) {
    SDL_Event event = {.type = type};
    if (type == SDL_EVENT_MOUSE_MOTION) {
        event.motion.x = x;
        event.motion.y = y;
    } else {
        event.button.button = SDL_BUTTON_LEFT;
        event.button.x = x;
        event.button.y = y;
    }
    TEST_ASSERT_TRUE(editor_host_event(&host, shell.view, &event));
    frame();
}

static void test_label_drag_only_preview_undo_redo_and_checkpoint(void) {
    VectorDocument *document = add_document();
    TEST_ASSERT_TRUE(vector_artboard_add(document, (PUIRect){10, 20, 100, 80}));
    document->viewport.zoom = 2;
    commit();
    TEST_ASSERT_TRUE(pui_view_checked(shell.view, "vector_selection"));
    PUIRect label = vector_artboard_label_rect(document, &document->artboards[0]);
    selection_pointer(SDL_EVENT_MOUSE_BUTTON_DOWN, label.x + 4, label.y + 45);
    SDL_Event motion = {.type = SDL_EVENT_MOUSE_MOTION};
    motion.motion.x = label.x + 50;
    motion.motion.y = label.y + 70;
    editor_host_event(&host, shell.view, &motion);
    TEST_ASSERT_FALSE(document->selection_tool.dragging);
    TEST_ASSERT_EQUAL_FLOAT(10, document->artboards[0].rect.x);
    selection_pointer(SDL_EVENT_MOUSE_BUTTON_DOWN, label.x + 4, label.y + 4);
    selection_pointer(SDL_EVENT_MOUSE_MOTION, label.x + 64, label.y + 44);
    TEST_ASSERT_EQUAL_FLOAT(10, document->artboards[0].rect.x);
    TEST_ASSERT_EQUAL_FLOAT(40, vector_artboard_display_rect(document, &document->artboards[0]).x);
    TEST_ASSERT_EQUAL_INT(1, document->history.count);
    selection_pointer(SDL_EVENT_MOUSE_BUTTON_UP, label.x + 64, label.y + 44);
    TEST_ASSERT_FALSE(document->selection_tool.dragging);
    TEST_ASSERT_EQUAL_FLOAT(40, document->artboards[0].rect.x);
    TEST_ASSERT_EQUAL_FLOAT(40, document->artboards[0].rect.y);
    TEST_ASSERT_EQUAL_INT(2, document->history.count);
    TEST_ASSERT_TRUE(editor_host_action(&host, shell.view, "editor.undo", ""));
    TEST_ASSERT_EQUAL_FLOAT(10, document->artboards[0].rect.x);
    TEST_ASSERT_EQUAL_FLOAT(20, document->artboards[0].rect.y);
    activate("scripts");
    activate("vector");
    commit();
    VectorEditor *editor = host.instance;
    document = &editor->documents.items[0];
    TEST_ASSERT_TRUE(editor_host_action(&host, shell.view, "editor.redo", ""));
    TEST_ASSERT_EQUAL_FLOAT(40, document->artboards[0].rect.x);
    TEST_ASSERT_EQUAL_FLOAT(40, document->artboards[0].rect.y);
}

static void test_label_drag_cancellation_noop_and_mode_guards(void) {
    VectorDocument *document = add_document();
    TEST_ASSERT_TRUE(vector_artboard_add(document, (PUIRect){10, 20, 100, 80}));
    commit();
    PUIRect label = vector_artboard_label_rect(document, &document->artboards[0]);
    selection_pointer(SDL_EVENT_MOUSE_BUTTON_DOWN, label.x + 4, label.y + 4);
    selection_pointer(SDL_EVENT_MOUSE_BUTTON_UP, label.x + 4, label.y + 4);
    TEST_ASSERT_EQUAL_INT(1, document->history.count);
    selection_pointer(SDL_EVENT_MOUSE_BUTTON_DOWN, label.x + 4, label.y + 4);
    selection_pointer(SDL_EVENT_MOUSE_MOTION, label.x + 60, label.y + 30);
    SDL_Event key = {.type = SDL_EVENT_KEY_DOWN};
    key.key.key = SDLK_ESCAPE;
    TEST_ASSERT_TRUE(editor_host_event(&host, shell.view, &key));
    TEST_ASSERT_FALSE(document->selection_tool.dragging);
    TEST_ASSERT_EQUAL_FLOAT(10, document->artboards[0].rect.x);
    TEST_ASSERT_EQUAL_INT(1, document->history.count);
    selection_pointer(SDL_EVENT_MOUSE_BUTTON_DOWN, label.x + 4, label.y + 4);
    selection_pointer(SDL_EVENT_MOUSE_MOTION, label.x + 60, label.y + 30);
    TEST_ASSERT_TRUE(editor_host_action(&host, shell.view, "vector.add_artboard", ""));
    TEST_ASSERT_FALSE(document->selection_tool.dragging);
    TEST_ASSERT_FALSE(pui_view_checked(shell.view, "vector_selection"));
    document->active_artboard = 0;
    TEST_ASSERT_TRUE(editor_host_action(&host, shell.view, "vector.artboard.select",
                                        "vector_document_1.artboard_1"));
    TEST_ASSERT_EQUAL_UINT(0, document->active_artboard);
    selection_pointer(SDL_EVENT_MOUSE_BUTTON_DOWN, label.x + 4, label.y + 4);
    TEST_ASSERT_TRUE(document->artboard_tool.dragging);
    TEST_ASSERT_FALSE(document->selection_tool.dragging);
    TEST_ASSERT_TRUE(editor_host_action(&host, shell.view, "vector.tool_selection", ""));
    selection_pointer(SDL_EVENT_MOUSE_BUTTON_DOWN, label.x + 4, label.y + 4);
    SDL_Event lost = {.type = SDL_EVENT_WINDOW_FOCUS_LOST};
    TEST_ASSERT_TRUE(editor_host_event(&host, shell.view, &lost));
    TEST_ASSERT_FALSE(document->selection_tool.dragging);
    TEST_ASSERT_EQUAL_INT(1, document->history.count);
    TEST_ASSERT_EQUAL_FLOAT(10, document->artboards[0].rect.x);
}

static void assert_rect(PUIRect expected, PUIRect actual) {
    TEST_ASSERT_EQUAL_FLOAT(expected.x, actual.x);
    TEST_ASSERT_EQUAL_FLOAT(expected.y, actual.y);
    TEST_ASSERT_EQUAL_FLOAT(expected.w, actual.w);
    TEST_ASSERT_EQUAL_FLOAT(expected.h, actual.h);
}

static PUIRect board_screen(VectorDocument *document) {
    VectorViewport *v = &document->viewport;
    PUIRect r = document->artboards[0].rect;
    return (PUIRect){v->bounds.x + v->x + r.x * v->zoom, v->bounds.y + v->y + r.y * v->zoom,
                     r.w * v->zoom, r.h * v->zoom};
}

static void test_all_eight_resize_handles_use_zoom_and_single_history_steps(void) {
    VectorDocument *document = add_document();
    const PUIRect initial = {10, 20, 200, 100};
    TEST_ASSERT_TRUE(vector_artboard_add(document, initial));
    document->viewport.zoom = 2;
    commit();
    const PUIRect expected[8] = {{30, 30, 180, 90},  {10, 30, 200, 90},  {10, 30, 220, 90},
                                 {10, 20, 220, 100}, {10, 20, 220, 110}, {10, 20, 200, 110},
                                 {30, 20, 180, 110}, {30, 20, 180, 100}};
    for (int i = 0; i < 8; i++) {
        PUIRect handle = vector_resize_handle_rect(board_screen(document), i, 7);
        float x = handle.x + 3.5f, y = handle.y + 3.5f;
        selection_pointer(SDL_EVENT_MOUSE_BUTTON_DOWN, x, y);
        TEST_ASSERT_EQUAL_UINT(vector_resize_handle_edges(i),
                               document->selection_tool.resize_edges);
        selection_pointer(SDL_EVENT_MOUSE_MOTION, x + 40, y + 20);
        assert_rect(initial, document->artboards[0].rect);
        assert_rect(expected[i], vector_artboard_display_rect(document, &document->artboards[0]));
        TEST_ASSERT_EQUAL_INT(1, document->history.cursor);
        selection_pointer(SDL_EVENT_MOUSE_BUTTON_UP, x + 40, y + 20);
        commit();
        assert_rect(expected[i], document->artboards[0].rect);
        TEST_ASSERT_EQUAL_INT(2, document->history.cursor);
        TEST_ASSERT_EQUAL_INT(2, document->history.count);
        TEST_ASSERT_TRUE(vector_history_undo(&document->history, document));
        assert_rect(initial, document->artboards[0].rect);
        TEST_ASSERT_TRUE(vector_history_redo(&document->history, document));
        assert_rect(expected[i], document->artboards[0].rect);
        TEST_ASSERT_TRUE(vector_history_undo(&document->history, document));
        commit();
    }
}

static void test_resize_minimum_cancel_noop_and_checkpoint_redo(void) {
    VectorDocument *document = add_document();
    PUIRect initial = {10, 20, 200, 100};
    TEST_ASSERT_TRUE(vector_artboard_add(document, initial));
    commit();
    PUIRect h = vector_resize_handle_rect(board_screen(document), 0, 7);
    float x = h.x + 3.5f, y = h.y + 3.5f;
    selection_pointer(SDL_EVENT_MOUSE_BUTTON_DOWN, x, y);
    selection_pointer(SDL_EVENT_MOUSE_BUTTON_UP, x, y);
    TEST_ASSERT_EQUAL_INT(1, document->history.count);
    selection_pointer(SDL_EVENT_MOUSE_BUTTON_DOWN, x, y);
    selection_pointer(SDL_EVENT_MOUSE_MOTION, x + 500, y + 500);
    assert_rect((PUIRect){209, 119, 1, 1},
                vector_artboard_display_rect(document, &document->artboards[0]));
    SDL_Event escape = {.type = SDL_EVENT_KEY_DOWN};
    escape.key.key = SDLK_ESCAPE;
    TEST_ASSERT_TRUE(editor_host_event(&host, shell.view, &escape));
    assert_rect(initial, document->artboards[0].rect);
    TEST_ASSERT_EQUAL_INT(1, document->history.count);
    selection_pointer(SDL_EVENT_MOUSE_BUTTON_DOWN, x, y);
    selection_pointer(SDL_EVENT_MOUSE_BUTTON_UP, x + 20, y + 10);
    commit();
    TEST_ASSERT_EQUAL_STRING("180", pui_view_text(shell.view, "artboard_width"));
    TEST_ASSERT_EQUAL_STRING("90", pui_view_text(shell.view, "artboard_height"));
    TEST_ASSERT_TRUE(vector_history_undo(&document->history, document));
    activate("scripts");
    activate("vector");
    commit();
    VectorEditor *editor = host.instance;
    document = &editor->documents.items[0];
    assert_rect(initial, document->artboards[0].rect);
    TEST_ASSERT_TRUE(vector_history_redo(&document->history, document));
    assert_rect((PUIRect){30, 30, 180, 90}, document->artboards[0].rect);
}

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_all_eight_resize_handles_use_zoom_and_single_history_steps);
    RUN_TEST(test_resize_minimum_cancel_noop_and_checkpoint_redo);
    RUN_TEST(test_label_drag_only_preview_undo_redo_and_checkpoint);
    RUN_TEST(test_label_drag_cancellation_noop_and_mode_guards);
    RUN_TEST(test_tab_commits_properties_and_preserves_next_field_selection);
    RUN_TEST(test_artboard_inspector_updates_history_and_survives_editor_switch);
    RUN_TEST(test_artboard_selection_and_tool_toggle);
    RUN_TEST(test_empty_restored_checkpoint_gets_a_default_document);
    RUN_TEST(test_empty_workspace_gets_one_default_document_without_duplicates);
    RUN_TEST(test_navigation_tab_selection_survives_document_changes_editor_switch_and_reload);
    RUN_TEST(test_document_tree_tracks_changes_documents_and_checkpoint_restore);
    RUN_TEST(test_truncated_checkpoint_releases_partial_history_and_retains_model);
    RUN_TEST(test_switch_restores_documents_histories_viewports_and_latest_edits);
    RUN_TEST(test_checkpoint_retains_dragged_document_order_and_selection);
    RUN_TEST(test_reload_preserves_document_and_pending_creation_is_flushed);
    RUN_TEST(test_corrupt_checkpoint_keeps_current_editor_and_previous_snapshot);
    RUN_TEST(test_failed_checkpoint_and_failed_mount_do_not_discard_active_document);
    RUN_TEST(test_session_is_not_carried_into_a_new_host);
    return UNITY_END();
}
