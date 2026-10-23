/** @file @brief Project-scoped logging, readable metrics, bounded event frequency and adapter
 * wiring. */
#include "editor_host/host.h"
#include "editors/vector/document/selection.h"
#include "editors/vector/internal.h"
#include "editors/vector/repeat/repeat.h"
#include "monitor/log.h"
#include "shell/shell.h"
#include "unity.h"
#include <stdio.h>
#include <string.h>
#include <yaml.h>

static MonitorLog log_state;
static ProcessMonitor monitor;
static char folder[4096];

static SDL_EnumerationResult SDLCALL remove_child(void *context, const char *path,
                                                  const char *name) {
    (void)context;
    char file[4096];
    snprintf(file, sizeof file, "%s%s", path, name);
    SDL_RemovePath(file);
    return SDL_ENUM_CONTINUE;
}

void setUp(void) {
    TEST_ASSERT_TRUE(SDL_Init(SDL_INIT_VIDEO));
    char *cwd = SDL_GetCurrentDirectory();
    snprintf(folder, sizeof folder, "%smonitor-test-%llu", cwd,
             (unsigned long long)SDL_GetPerformanceCounter());
    SDL_free(cwd);
    TEST_ASSERT_TRUE(SDL_CreateDirectory(folder));
    log_state = (MonitorLog){0};
    monitor = (ProcessMonitor){0};
}

void tearDown(void) {
    monitor_log_stop(&log_state, &monitor, "test.cleanup", 99);
    SDL_EnumerateDirectory(folder, remove_child, NULL);
    SDL_RemovePath(folder);
    SDL_Quit();
}

static EditorDiagnostics context(void) {
    EditorDiagnostics d = {.document = 1, .objects = 1, .instances = 1024, .selected = 1};
    snprintf(d.operation, sizeof d.operation, "idle");
    return d;
}

static void start(void) {
    monitor_log_project(&log_state, folder, &monitor, 0);
    TEST_ASSERT_TRUE_MESSAGE(monitor_log_start(&log_state, &monitor, "vector", context(), 1),
                             log_state.error);
}

static char *read_log(void) {
    char *text = SDL_LoadFile(log_state.path, NULL);
    TEST_ASSERT_NOT_NULL(text);
    return text;
}

static int occurrences(const char *text, const char *part) {
    int count = 0;
    for (const char *p = text; (p = strstr(p, part)); p += strlen(part)) {
        count++;
    }
    return count;
}

static void test_requires_project_and_does_not_create_files_while_disabled(void) {
    TEST_ASSERT_FALSE(monitor_log_start(&log_state, &monitor, "vector", context(), 1));
    TEST_ASSERT_NULL(log_state.stream);
    TEST_ASSERT_NOT_NULL(strstr(log_state.error, "Projekt"));
    monitor_log_observe(&log_state, &monitor, "vector", context(), 2);
    TEST_ASSERT_EQUAL_STRING("", log_state.path);
}

static void test_json_rows_escape_actions_and_report_unknown_metrics_as_null(void) {
    start();
    monitor_log_action(&log_state, &monitor, "vector", context(), "quoted\"\\\n\001", "widget",
                       1.1);
    monitor_log_stop(&log_state, &monitor, "user.stop", 2);
    char *text = read_log();
    TEST_ASSERT_EQUAL_INT(3, occurrences(text, "\n"));
    TEST_ASSERT_NOT_NULL(strstr(text, "quoted\\\"\\\\\\u000a\\u0001"));
    TEST_ASSERT_NOT_NULL(strstr(text, "\"cpu_percent\":null"));
    TEST_ASSERT_NOT_NULL(strstr(text, "\"gpu_effect_mib\":null"));
    TEST_ASSERT_NOT_NULL(strstr(text, "\"metrics_age_s\":null"));
    TEST_ASSERT_NOT_NULL(strstr(text, "\"instances\":1024.000000"));
    TEST_ASSERT_NOT_NULL(strstr(text, "\"session.stop\""));
    /* Each JSON line must independently parse as a flow mapping. */
    for (char *line = text; *line;) {
        char *end = strchr(line, '\n');
        TEST_ASSERT_NOT_NULL(end);
        yaml_parser_t parser;
        yaml_document_t document;
        TEST_ASSERT_TRUE(yaml_parser_initialize(&parser));
        yaml_parser_set_input_string(&parser, (unsigned char *)line, (size_t)(end - line));
        TEST_ASSERT_TRUE(yaml_parser_load(&parser, &document));
        TEST_ASSERT_EQUAL_INT(YAML_MAPPING_NODE, yaml_document_get_root_node(&document)->type);
        yaml_document_delete(&document);
        yaml_parser_delete(&parser);
        line = end + 1;
    }
    SDL_free(text);
}

static void test_samples_and_gesture_boundaries_do_not_log_every_motion(void) {
    start();
    EditorDiagnostics d = context();
    d.interacting = true;
    snprintf(d.operation, sizeof d.operation, "selection.move");
    monitor_log_observe(&log_state, &monitor, "vector", d, 1.1);
    for (int i = 0; i < 1000; i++) {
        monitor_log_observe(&log_state, &monitor, "vector", d, 1.1 + i * .0001);
    }
    ProcessCounters c = {.cpu_available = true, .cpu_seconds = 1};
    TEST_ASSERT_TRUE(process_monitor_record(&monitor, c, 1.2));
    monitor_log_observe(&log_state, &monitor, "vector", d, 1.2);
    c.cpu_seconds = 1.25;
    TEST_ASSERT_TRUE(process_monitor_record(&monitor, c, 1.7));
    monitor_log_observe(&log_state, &monitor, "vector", d, 1.7);
    d.interacting = false;
    d.history_cursor++;
    snprintf(d.operation, sizeof d.operation, "idle");
    monitor_log_observe(&log_state, &monitor, "vector", d, 1.8);
    char *text = read_log();
    TEST_ASSERT_EQUAL_INT(5, occurrences(text, "\n"));
    TEST_ASSERT_EQUAL_INT(1, occurrences(text, "\"interaction.begin\""));
    TEST_ASSERT_EQUAL_INT(1, occurrences(text, "\"interaction.end\""));
    TEST_ASSERT_EQUAL_INT(2, occurrences(text, "\"sample\""));
    TEST_ASSERT_NOT_NULL(strstr(text, "\"cpu_percent\":50.000000"));
    SDL_free(text);
}

static void test_switching_project_stops_and_restart_preserves_old_file(void) {
    start();
    char previous[4096];
    snprintf(previous, sizeof previous, "%s", log_state.path);
    monitor_log_project(&log_state, "", &monitor, 2);
    TEST_ASSERT_NULL(log_state.stream);
    char *text = SDL_LoadFile(previous, NULL);
    TEST_ASSERT_NOT_NULL(text);
    TEST_ASSERT_NOT_NULL(strstr(text, "project.changed"));
    SDL_free(text);
    start();
    TEST_ASSERT_NOT_EQUAL(0, strcmp(previous, log_state.path));
    monitor_log_observe(&log_state, &monitor, "scripts", (EditorDiagnostics){0}, 3);
    text = read_log();
    TEST_ASSERT_NOT_NULL(strstr(text, "context.changed"));
    SDL_free(text);
}

static void test_write_failure_closes_recording_instead_of_growing_memory(void) {
    start();
    TEST_ASSERT_TRUE(SDL_CloseIO(log_state.stream));
    char bytes[8];
    log_state.stream = SDL_IOFromMem(bytes, sizeof bytes);
    TEST_ASSERT_NOT_NULL(log_state.stream);
    monitor_log_action(&log_state, &monitor, "vector", context(), "action", "", 2);
    TEST_ASSERT_NULL(log_state.stream);
    TEST_ASSERT_NOT_EQUAL(0, log_state.error[0]);
}

static void test_host_toggle_and_real_clone_move_are_logged_through_adapter(void) {
    SDL_Window *window = SDL_CreateWindow("monitor", 1280, 800, SDL_WINDOW_HIDDEN);
    SDL_Renderer *renderer = SDL_CreateRenderer(window, NULL);
    TEST_ASSERT_NOT_NULL(renderer);
    EditorShell shell = {0};
    EditorHost host = {0};
    char path[1600], error[256];
    snprintf(path, sizeof path, "%s/shell.yaml", PENGUI_EDITOR_RESOURCES);
    TEST_ASSERT_TRUE(editor_shell_load(&shell, renderer, path, error, sizeof error));
    editor_host_init(&host, (EditorServices){PENGUI_EDITOR_RESOURCES});
    TEST_ASSERT_TRUE(editor_host_request(&host, "vector", false));
    TEST_ASSERT_TRUE(editor_host_commit(&host, shell.view, &shell.input, error, sizeof error));
    pui_view_draw(shell.view, &shell.input, (PUIRect){0, 0, 1280, 800}, NULL, NULL);
    TEST_ASSERT_TRUE(editor_host_commit(&host, shell.view, &shell.input, error, sizeof error));
    pui_view_set_checked(shell.view, "process_monitor_log", true);
    TEST_ASSERT_TRUE(editor_host_action(&host, shell.view, "monitor.log", "process_monitor_log"));
    TEST_ASSERT_FALSE(pui_view_checked(shell.view, "process_monitor_log"));
    editor_host_project(&host, folder);
    pui_view_set_checked(shell.view, "process_monitor_log", true);
    TEST_ASSERT_TRUE(editor_host_action(&host, shell.view, "monitor.log", "process_monitor_log"));
    TEST_ASSERT_NOT_NULL_MESSAGE(host.log.stream, host.log.error);
    VectorEditor *editor = host.instance;
    VectorDocument *d = vector_document_tabs_active(&editor->documents, shell.view);
    TEST_ASSERT_NOT_NULL(d);
    TEST_ASSERT_TRUE(vector_rectangle_add(d, 1, (PUIRect){40, 40, 60, 40}));
    vector_document_select(d, d->rectangles[0].id);
    VectorRepeat repeat = vector_repeat_default();
    TEST_ASSERT_TRUE(vector_repeat_apply(d, &repeat, true));
    editor_host_log_action(&host, shell.view, "vector.repeat.apply", "repeat");
    TEST_ASSERT_TRUE(editor_host_commit(&host, shell.view, &shell.input, error, sizeof error));
    pui_view_draw(shell.view, &shell.input, (PUIRect){0, 0, 1280, 800}, NULL, NULL);
    SDL_Event event = {.type = SDL_EVENT_MOUSE_BUTTON_DOWN};
    event.button.button = SDL_BUTTON_LEFT;
    event.button.x = d->viewport.bounds.x + d->viewport.x + 70;
    event.button.y = d->viewport.bounds.y + d->viewport.y + 60;
    TEST_ASSERT_TRUE(editor_host_event(&host, shell.view, &event));
    TEST_ASSERT_TRUE(d->selection_tool.dragging);
    TEST_ASSERT_FALSE(d->selection_tool.rotating);
    event.type = SDL_EVENT_MOUSE_MOTION;
    event.motion.x = d->viewport.bounds.x + d->viewport.x + 100;
    event.motion.y = d->viewport.bounds.y + d->viewport.y + 80;
    TEST_ASSERT_TRUE(editor_host_event(&host, shell.view, &event));
    event.type = SDL_EVENT_MOUSE_BUTTON_UP;
    event.button.button = SDL_BUTTON_LEFT;
    event.button.x = d->viewport.bounds.x + d->viewport.x + 100;
    event.button.y = d->viewport.bounds.y + d->viewport.y + 80;
    TEST_ASSERT_TRUE(editor_host_event(&host, shell.view, &event));
    TEST_ASSERT_TRUE(editor_host_request(&host, "scripts", false));
    TEST_ASSERT_TRUE(editor_host_commit(&host, shell.view, &shell.input, error, sizeof error));
    TEST_ASSERT_NOT_NULL(host.log.stream);
    TEST_ASSERT_TRUE(editor_host_request(&host, "vector", false));
    TEST_ASSERT_TRUE(editor_host_commit(&host, shell.view, &shell.input, error, sizeof error));
    TEST_ASSERT_NOT_NULL(host.log.stream);
    TEST_ASSERT_TRUE(pui_view_checked(shell.view, "process_monitor_log"));
    pui_view_set_checked(shell.view, "process_monitor_log", false);
    TEST_ASSERT_TRUE(editor_host_action(&host, shell.view, "monitor.log", "process_monitor_log"));
    TEST_ASSERT_NULL(host.log.stream);
    char *text = SDL_LoadFile(host.log.path, NULL);
    TEST_ASSERT_NOT_NULL(text);
    TEST_ASSERT_NOT_NULL(strstr(text, "selection.move"));
    TEST_ASSERT_NOT_NULL(strstr(text, "interaction.begin"));
    TEST_ASSERT_NOT_NULL(strstr(text, "interaction.end"));
    TEST_ASSERT_NOT_NULL(strstr(text, "\"instances\":20.000000"));
    SDL_free(text);
    editor_shell_destroy(&shell);
    editor_host_destroy(&host);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
}

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_requires_project_and_does_not_create_files_while_disabled);
    RUN_TEST(test_json_rows_escape_actions_and_report_unknown_metrics_as_null);
    RUN_TEST(test_samples_and_gesture_boundaries_do_not_log_every_motion);
    RUN_TEST(test_switching_project_stops_and_restart_preserves_old_file);
    RUN_TEST(test_write_failure_closes_recording_instead_of_growing_memory);
    RUN_TEST(test_host_toggle_and_real_clone_move_are_logged_through_adapter);
    return UNITY_END();
}
