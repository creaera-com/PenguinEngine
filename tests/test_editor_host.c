#include "editor/config.h"
#include "editor_host/host.h"
#include "shell/shell.h"
#include "unity.h"
#include <stdio.h>
#include <string.h>

static SDL_Window *window;
static SDL_Renderer *renderer;
static EditorShell shell;
static EditorHost host;

void setUp(void) {
    TEST_ASSERT_TRUE(SDL_Init(SDL_INIT_VIDEO));
    window = SDL_CreateWindow("editor host", 800, 600, SDL_WINDOW_HIDDEN);
    renderer = SDL_CreateRenderer(window, NULL);
    TEST_ASSERT_NOT_NULL(renderer);
    char path[1600], error[256];
    snprintf(path, sizeof path, "%s/shell.yaml", PENGUI_EDITOR_RESOURCES);
    TEST_ASSERT_TRUE_MESSAGE(editor_shell_load(&shell, renderer, path, error, sizeof error), error);
    editor_host_init(&host, (EditorServices){PENGUI_EDITOR_RESOURCES});
}

void tearDown(void) {
    editor_shell_destroy(&shell);
    editor_host_destroy(&host);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
}

static void activate(const char *id) {
    char error[256];
    TEST_ASSERT_TRUE(editor_host_request(&host, id, false));
    TEST_ASSERT_TRUE_MESSAGE(
        editor_host_commit(&host, shell.view, &shell.input, error, sizeof error), error);
    pui_view_draw(shell.view, &shell.input, (PUIRect){0, 0, 800, 600}, NULL, NULL);
    TEST_ASSERT_EQUAL_STRING(id, host.active->id);
    TEST_ASSERT_TRUE(pui_view_checked(shell.view, id));
}

static void test_all_editors_switch_and_same_editor_is_not_recreated(void) {
    TEST_ASSERT_NULL(host.instance);
    activate("vector");
    void *instance = host.instance;
    activate("vector");
    TEST_ASSERT_EQUAL_PTR(instance, host.instance);
    for (int i = 0; i < 12; i++) {
        activate("scripts");
        activate("game");
        activate("vector");
    }
    TEST_ASSERT_FALSE(pui_view_checked(shell.view, "game"));
    TEST_ASSERT_FALSE(editor_host_request(&host, "unknown", false));
    TEST_ASSERT_EQUAL_STRING("vector", host.active->id);
}

static void test_failure_retains_old_editor_and_explicit_reload_recreates_it(void) {
    activate("vector");
    void *instance = host.instance;
    host.services.resource_root = "/missing/editor/resources";
    TEST_ASSERT_TRUE(editor_host_request(&host, "game", false));
    char error[256];
    TEST_ASSERT_FALSE(editor_host_commit(&host, shell.view, &shell.input, error, sizeof error));
    TEST_ASSERT_EQUAL_PTR(instance, host.instance);
    TEST_ASSERT_EQUAL_STRING("vector", host.active->id);
    TEST_ASSERT_TRUE(pui_view_checked(shell.view, "vector"));
    host.services.resource_root = PENGUI_EDITOR_RESOURCES;
    TEST_ASSERT_TRUE(editor_host_request(&host, "vector", true));
    TEST_ASSERT_TRUE_MESSAGE(
        editor_host_commit(&host, shell.view, &shell.input, error, sizeof error), error);
    TEST_ASSERT_NOT_EQUAL(instance, host.instance);
}

static void test_config_reads_editor_and_rejects_invalid_value_atomically(void) {
    EditorConfig config = {0};
    char path[1600], error[256];
    snprintf(path, sizeof path, "%s/test-editor-config.yaml", PENGUI_EDITOR_RESOURCES);
    TEST_ASSERT_TRUE_MESSAGE(editor_config_load(&config, path, error, sizeof error), error);
    TEST_ASSERT_EQUAL_STRING("scripts", config.startup_editor);
    snprintf(path, sizeof path, "%s/test-invalid-editor-config.yaml", PENGUI_EDITOR_RESOURCES);
    TEST_ASSERT_FALSE(editor_config_load(&config, path, error, sizeof error));
    TEST_ASSERT_EQUAL_STRING("scripts", config.startup_editor);
}

static void test_mount_preserves_outer_values_resets_child_and_clears_focus(void) {
    const char *yaml = "schema: pengui/v1\nroot: {type: column, padding: 0, children: ["
                       "{type: input, id: outer, text: Original}, {type: slot, id: body}, "
                       "{type: dropdown, id: mode, children: [{type: option, value: a}, {type: "
                       "option, value: b}]}, {type: button_group, children: ["
                       "{type: button, id: a}, {type: button, id: b}]}]}";
    char error[256];
    PUILayout *base = pui_layout_parse(yaml, strlen(yaml), error, sizeof error);
    TEST_ASSERT_NOT_NULL_MESSAGE(base, error);
    const char *child_yaml = "schema: pengui/v1\nroot: {type: input, id: inner, text: Initial}";
    PUILayout *child = pui_layout_parse(child_yaml, strlen(child_yaml), error, sizeof error);
    TEST_ASSERT_NOT_NULL_MESSAGE(child, error);
    PUIView *view = pui_view_new(base, renderer);
    PUIContext input;
    pui_init(&input);
    TEST_ASSERT_TRUE(pui_view_set_text(view, "outer", "Retained"));
    TEST_ASSERT_TRUE(pui_view_set_value(view, "mode", "b"));
    TEST_ASSERT_TRUE(pui_view_set_checked(view, "b", true));
    TEST_ASSERT_TRUE_MESSAGE(
        pui_view_replace_slot(view, &input, "body", child, error, sizeof error), error);
    TEST_ASSERT_EQUAL_STRING("Retained", pui_view_text(view, "outer"));
    TEST_ASSERT_EQUAL_STRING("b", pui_view_value(view, "mode"));
    TEST_ASSERT_TRUE(pui_view_checked(view, "b"));
    TEST_ASSERT_TRUE(pui_view_set_text(view, "inner", "Discarded"));
    pui_view_draw(view, &input, (PUIRect){0, 0, 400, 300}, NULL, NULL);
    SDL_Event click = {0};
    click.type = SDL_EVENT_MOUSE_BUTTON_DOWN;
    click.button.button = SDL_BUTTON_LEFT;
    click.button.x = 20;
    click.button.y = 15;
    pui_view_event(view, &input, window, &click);
    TEST_ASSERT_EQUAL_UINT(pui_id("outer"), input.focus);
    TEST_ASSERT_TRUE(pui_view_replace_slot(view, &input, "body", child, error, sizeof error));
    TEST_ASSERT_EQUAL_UINT(0, input.focus);
    TEST_ASSERT_EQUAL_UINT(0, input.active);
    TEST_ASSERT_EQUAL_STRING("Initial", pui_view_text(view, "inner"));
    TEST_ASSERT_EQUAL_STRING("Retained", pui_view_text(view, "outer"));
    TEST_ASSERT_EQUAL_STRING("b", pui_view_value(view, "mode"));
    TEST_ASSERT_TRUE(pui_view_checked(view, "b"));
    pui_focus(&input, pui_id("outer"));
    TEST_ASSERT_FALSE(pui_view_replace_slot(view, &input, "missing", child, error, sizeof error));
    TEST_ASSERT_EQUAL_UINT(pui_id("outer"), input.focus);
    TEST_ASSERT_EQUAL_STRING("Retained", pui_view_text(view, "outer"));
    TEST_ASSERT_TRUE(pui_view_replace_slot(view, &input, "body", NULL, error, sizeof error));
    TEST_ASSERT_NULL(pui_view_text(view, "inner"));
    pui_view_free(view);
    pui_layout_free(child);
    pui_layout_free(base);
}

static int released_candidates;
static bool duplicate_candidate;
static int ui_commits;

static void *candidate_create(const EditorServices *services, char *error, size_t capacity) {
    (void)services;
    const char *yaml = duplicate_candidate
                           ? "schema: pengui/v1\nroot: {type: label, id: status}"
                           : "schema: pengui/v1\nroot: {type: label, id: candidate, text: Test}";
    return pui_layout_parse(yaml, strlen(yaml), error, capacity);
}

static const PUILayout *candidate_layout(const void *instance) {
    return instance;
}

static void candidate_destroy(void *instance) {
    released_candidates++;
    pui_layout_free(instance);
}

static bool candidate_commit(void *instance, PUIView *view, PUIContext *input, char *error,
                             size_t capacity) {
    (void)instance;
    (void)error;
    (void)capacity;
    TEST_ASSERT_EQUAL_PTR(shell.view, view);
    TEST_ASSERT_EQUAL_PTR(&shell.input, input);
    ui_commits++;
    return true;
}

static void test_successful_switch_and_failed_mount_release_editor_instances(void) {
    const EditorDefinition candidate = {.id = "candidate",
                                        .title = "Candidate",
                                        .create = candidate_create,
                                        .layout = candidate_layout,
                                        .destroy = candidate_destroy,
                                        .commit_ui = candidate_commit};
    released_candidates = 0;
    ui_commits = 0;
    duplicate_candidate = false;
    host.pending = &candidate;
    char error[256];
    TEST_ASSERT_TRUE(editor_host_commit(&host, shell.view, &shell.input, error, sizeof error));
    TEST_ASSERT_EQUAL_INT(0, released_candidates);
    TEST_ASSERT_EQUAL_INT(0, ui_commits);
    TEST_ASSERT_TRUE(editor_host_commit(&host, shell.view, &shell.input, error, sizeof error));
    TEST_ASSERT_EQUAL_INT(1, ui_commits);
    activate("vector");
    TEST_ASSERT_EQUAL_INT(1, released_candidates);
    void *active = host.instance;
    duplicate_candidate = true;
    host.pending = &candidate;
    TEST_ASSERT_FALSE(editor_host_commit(&host, shell.view, &shell.input, error, sizeof error));
    TEST_ASSERT_EQUAL_INT(2, released_candidates);
    TEST_ASSERT_EQUAL_PTR(active, host.instance);
    TEST_ASSERT_EQUAL_STRING("vector", host.active->id);
}

static void request_action(void *context, const char *name, const char *widget) {
    (void)widget;
    EditorHost *target = context;
    TEST_ASSERT_EQUAL_STRING("editor.scripts", name);
    TEST_ASSERT_TRUE(editor_host_request(target, "scripts", false));
}

static void test_toolbar_action_queues_switch_until_commit(void) {
    activate("vector");
    pui_pointer(&shell.input, 210, 20, true);
    pui_pointer(&shell.input, 210, 20, false);
    pui_view_draw(shell.view, &shell.input, (PUIRect){0, 0, 800, 600}, request_action, &host);
    TEST_ASSERT_EQUAL_STRING("vector", host.active->id);
    TEST_ASSERT_NOT_NULL(host.pending);
    char error[256];
    TEST_ASSERT_TRUE(editor_host_commit(&host, shell.view, &shell.input, error, sizeof error));
    TEST_ASSERT_EQUAL_STRING("scripts", host.active->id);
}

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_toolbar_action_queues_switch_until_commit);
    RUN_TEST(test_successful_switch_and_failed_mount_release_editor_instances);
    RUN_TEST(test_all_editors_switch_and_same_editor_is_not_recreated);
    RUN_TEST(test_failure_retains_old_editor_and_explicit_reload_recreates_it);
    RUN_TEST(test_config_reads_editor_and_rejects_invalid_value_atomically);
    RUN_TEST(test_mount_preserves_outer_values_resets_child_and_clears_focus);
    return UNITY_END();
}
