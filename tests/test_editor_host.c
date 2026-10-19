#include "editor/config.h"
#include "editor_host/host.h"
#include "editors/vector/artboard/tool.h"
#include "editors/vector/internal.h"
#include "editors/vector/workspace/ruler.h"
#include "shell/shell.h"
#include "unity.h"
#include <math.h>
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
    pui_pointer(&shell.input, 306, 20, true);
    pui_pointer(&shell.input, 306, 20, false);
    pui_view_draw(shell.view, &shell.input, (PUIRect){0, 0, 800, 600}, request_action, &host);
    TEST_ASSERT_EQUAL_STRING("vector", host.active->id);
    TEST_ASSERT_NOT_NULL(host.pending);
    char error[256];
    TEST_ASSERT_TRUE(editor_host_commit(&host, shell.view, &shell.input, error, sizeof error));
    TEST_ASSERT_EQUAL_STRING("scripts", host.active->id);
}

static void document_action(void *context, const char *name, const char *widget) {
    TEST_ASSERT_EQUAL_STRING("vector.add_document", name);
    TEST_ASSERT_TRUE(editor_host_action(context, shell.view, name, widget));
}

static void test_add_document_button_appends_and_selects_tabs_between_frames(void) {
    activate("vector");
    TEST_ASSERT_EQUAL_INT(0, pui_view_tab_count(shell.view, "vector_document_tabs"));
    pui_pointer(&shell.input, 279, 70, true);
    pui_pointer(&shell.input, 279, 70, false);
    pui_view_draw(shell.view, &shell.input, (PUIRect){0, 0, 800, 600}, document_action, &host);
    TEST_ASSERT_EQUAL_INT(0, pui_view_tab_count(shell.view, "vector_document_tabs"));
    char error[256];
    TEST_ASSERT_TRUE_MESSAGE(
        editor_host_commit(&host, shell.view, &shell.input, error, sizeof error), error);
    TEST_ASSERT_EQUAL_INT(1, pui_view_tab_count(shell.view, "vector_document_tabs"));
    TEST_ASSERT_EQUAL_STRING("vector_document_1",
                             pui_view_selected_tab(shell.view, "vector_document_tabs"));
    for (int i = 1; i < VECTOR_MAX_DOCUMENTS; i++) {
        TEST_ASSERT_TRUE(
            editor_host_action(&host, shell.view, "vector.add_document", "vector_add_document"));
        TEST_ASSERT_TRUE_MESSAGE(
            editor_host_commit(&host, shell.view, &shell.input, error, sizeof error), error);
    }
    TEST_ASSERT_EQUAL_INT(VECTOR_MAX_DOCUMENTS,
                          pui_view_tab_count(shell.view, "vector_document_tabs"));
    TEST_ASSERT_TRUE(
        editor_host_action(&host, shell.view, "vector.add_document", "vector_add_document"));
    TEST_ASSERT_TRUE(editor_host_commit(&host, shell.view, &shell.input, error, sizeof error));
    TEST_ASSERT_EQUAL_INT(VECTOR_MAX_DOCUMENTS,
                          pui_view_tab_count(shell.view, "vector_document_tabs"));
    activate("scripts");
    activate("vector");
    TEST_ASSERT_EQUAL_INT(VECTOR_MAX_DOCUMENTS,
                          pui_view_tab_count(shell.view, "vector_document_tabs"));
}

static void test_append_tab_retains_existing_input_values_and_dragged_order(void) {
    char error[256];
    const char *base_yaml = "schema: pengui/v1\nroot: {type: tabs, id: documents, children: "
                            "[{type: tab, id: start, text: Start}]}";
    const char *content_yaml = "schema: pengui/v1\nroot: {type: input, id: name, text: Initial}";
    PUILayout *base = pui_layout_parse(base_yaml, strlen(base_yaml), error, sizeof error);
    PUILayout *content = pui_layout_parse(content_yaml, strlen(content_yaml), error, sizeof error);
    TEST_ASSERT_NOT_NULL(base);
    TEST_ASSERT_NOT_NULL(content);
    PUIView *view = pui_view_new(base, renderer);
    PUIContext input;
    pui_init(&input);
    TEST_ASSERT_TRUE(
        pui_view_add_tab(view, &input, "documents", "one", "One", content, error, sizeof error));
    TEST_ASSERT_TRUE(pui_view_set_text(view, "one.name", "Changed"));
    TEST_ASSERT_TRUE(
        pui_view_add_tab(view, &input, "documents", "two", "Two", content, error, sizeof error));
    TEST_ASSERT_EQUAL_STRING("Changed", pui_view_text(view, "one.name"));
    TEST_ASSERT_EQUAL_STRING("Initial", pui_view_text(view, "two.name"));
    pui_pointer(&input, 80, 15, true);
    pui_view_draw(view, &input, (PUIRect){0, 0, 400, 300}, NULL, NULL);
    pui_pointer(&input, 10, 15, true);
    pui_view_draw(view, &input, (PUIRect){0, 0, 400, 300}, NULL, NULL);
    pui_pointer(&input, 10, 15, false);
    pui_view_draw(view, &input, (PUIRect){0, 0, 400, 300}, NULL, NULL);
    TEST_ASSERT_TRUE(pui_view_add_tab(view, &input, "documents", "three", "Three", content, error,
                                      sizeof error));
    TEST_ASSERT_EQUAL_STRING("three", pui_view_selected_tab(view, "documents"));
    pui_pointer(&input, 10, 15, true);
    pui_pointer(&input, 10, 15, false);
    pui_view_draw(view, &input, (PUIRect){0, 0, 400, 300}, NULL, NULL);
    TEST_ASSERT_EQUAL_STRING("one", pui_view_selected_tab(view, "documents"));
    TEST_ASSERT_EQUAL_STRING("Changed", pui_view_text(view, "one.name"));
    TEST_ASSERT_FALSE(pui_view_add_tab(view, &input, "documents", "one", "Duplicate", content,
                                       error, sizeof error));
    TEST_ASSERT_EQUAL_INT(4, pui_view_tab_count(view, "documents"));
    TEST_ASSERT_EQUAL_STRING("one", pui_view_selected_tab(view, "documents"));
    TEST_ASSERT_EQUAL_STRING("Changed", pui_view_text(view, "one.name"));
    pui_view_free(view);
    pui_layout_free(content);
    pui_layout_free(base);
}

static void test_failed_document_tab_does_not_consume_document_number(void) {
    activate("vector");
    const char *yaml = "schema: pengui/v1\nroot: {type: label, text: Temporary}";
    char error[256];
    PUILayout *replacement = pui_layout_parse(yaml, strlen(yaml), error, sizeof error);
    TEST_ASSERT_NOT_NULL(replacement);
    TEST_ASSERT_TRUE(pui_view_replace_slot(shell.view, &shell.input, "editor_area", replacement,
                                           error, sizeof error));
    TEST_ASSERT_TRUE(
        editor_host_action(&host, shell.view, "vector.add_document", "vector_add_document"));
    TEST_ASSERT_FALSE(editor_host_commit(&host, shell.view, &shell.input, error, sizeof error));
    TEST_ASSERT_EQUAL_INT(0, pui_view_tab_count(shell.view, "vector_document_tabs"));
    TEST_ASSERT_TRUE(pui_view_replace_slot(shell.view, &shell.input, "editor_area",
                                           host.active->layout(host.instance), error,
                                           sizeof error));
    TEST_ASSERT_TRUE(
        editor_host_action(&host, shell.view, "vector.add_document", "vector_add_document"));
    TEST_ASSERT_TRUE(editor_host_commit(&host, shell.view, &shell.input, error, sizeof error));
    TEST_ASSERT_EQUAL_STRING("vector_document_1",
                             pui_view_selected_tab(shell.view, "vector_document_tabs"));
    pui_layout_free(replacement);
}

static VectorDocument *add_vector_document(void) {
    char error[256];
    TEST_ASSERT_TRUE(editor_host_action(&host, shell.view, "vector.add_document", ""));
    TEST_ASSERT_TRUE(editor_host_commit(&host, shell.view, &shell.input, error, sizeof error));
    pui_view_draw(shell.view, &shell.input, (PUIRect){0, 0, 800, 600}, NULL, NULL);
    VectorEditor *editor = host.instance;
    return vector_document_tabs_active(&editor->documents, shell.view);
}

static void canvas_event(SDL_Event *event) {
    bool consumed = pui_view_event(shell.view, &shell.input, window, event);
    TEST_ASSERT_FALSE(consumed);
    TEST_ASSERT_TRUE(editor_host_event(&host, shell.view, event));
    pui_view_draw(shell.view, &shell.input, (PUIRect){0, 0, 800, 600}, NULL, NULL);
}

static void draw_artboard(void) {
    TEST_ASSERT_TRUE(editor_host_action(&host, shell.view, "vector.add_artboard", ""));
    SDL_Event event = {.type = SDL_EVENT_MOUSE_BUTTON_DOWN};
    event.button.button = SDL_BUTTON_LEFT;
    event.button.x = 330;
    event.button.y = 220;
    canvas_event(&event);
    event.type = SDL_EVENT_MOUSE_MOTION;
    event.motion.x = 430;
    event.motion.y = 380;
    canvas_event(&event);
    event.type = SDL_EVENT_MOUSE_BUTTON_UP;
    event.button.button = SDL_BUTTON_LEFT;
    event.button.x = 430;
    event.button.y = 380;
    canvas_event(&event);
}

static void test_artboard_history_is_per_document_and_discards_redo_branch(void) {
    activate("vector");
    TEST_ASSERT_TRUE(editor_host_action(&host, shell.view, "vector.add_artboard", ""));
    VectorDocument *first = add_vector_document();
    TEST_ASSERT_NOT_NULL(first);
    draw_artboard();
    TEST_ASSERT_EQUAL_INT(1, first->artboard_count);
    unsigned original = first->active_artboard;
    TEST_ASSERT_EQUAL_FLOAT(100, first->artboards[0].rect.w);
    TEST_ASSERT_TRUE(editor_host_action(&host, shell.view, "editor.undo", ""));
    TEST_ASSERT_EQUAL_INT(0, first->artboard_count);
    TEST_ASSERT_TRUE(editor_host_action(&host, shell.view, "editor.redo", ""));
    TEST_ASSERT_EQUAL_UINT(original, first->active_artboard);
    VectorDocument *second = add_vector_document();
    TEST_ASSERT_TRUE(editor_host_action(&host, shell.view, "editor.undo", ""));
    TEST_ASSERT_EQUAL_INT(1, first->artboard_count);
    TEST_ASSERT_EQUAL_INT(0, second->artboard_count);
    draw_artboard();
    TEST_ASSERT_TRUE(editor_host_action(&host, shell.view, "editor.undo", ""));
    draw_artboard();
    TEST_ASSERT_EQUAL_INT(1, second->artboard_count);
    TEST_ASSERT_EQUAL_UINT(2, second->active_artboard);
    TEST_ASSERT_FALSE(vector_history_redo(&second->history, second));
    TEST_ASSERT_EQUAL_INT(1, first->history.cursor);
    for (int i = 1; i < VECTOR_MAX_ARTBOARDS; i++) {
        TEST_ASSERT_TRUE(vector_artboard_add(second, (PUIRect){0, 0, 100, 160}));
    }
    TEST_ASSERT_FALSE(vector_artboard_add(second, (PUIRect){0, 0, 100, 160}));
    TEST_ASSERT_EQUAL_INT(VECTOR_MAX_ARTBOARDS, second->history.count);
    for (int i = 0; i < VECTOR_MAX_ARTBOARDS; i++) {
        TEST_ASSERT_TRUE(vector_history_undo(&second->history, second));
    }
    TEST_ASSERT_EQUAL_INT(0, second->artboard_count);
    TEST_ASSERT_FALSE(vector_history_undo(&second->history, second));
    for (int i = 0; i < VECTOR_MAX_ARTBOARDS; i++) {
        TEST_ASSERT_TRUE(vector_history_redo(&second->history, second));
    }
    TEST_ASSERT_EQUAL_INT(VECTOR_MAX_ARTBOARDS, second->artboard_count);
}

static void test_canvas_is_clipped_and_zoom_keeps_point_under_pointer(void) {
    activate("vector");
    VectorDocument *document = add_vector_document();
    TEST_ASSERT_TRUE(pui_view_hit(shell.view, "vector_document_1.canvas", 400, 300));
    TEST_ASSERT_FALSE(pui_view_hit(shell.view, "vector_document_1.canvas", 10, 10));
    SDL_Surface *surface = SDL_RenderReadPixels(renderer, NULL);
    TEST_ASSERT_NOT_NULL(surface);
    Uint8 r, g, b, a;
    TEST_ASSERT_TRUE(SDL_ReadSurfacePixel(surface, 400, 300, &r, &g, &b, &a));
    TEST_ASSERT_EQUAL_UINT8(104, r);
    TEST_ASSERT_EQUAL_UINT8(104, g);
    SDL_DestroySurface(surface);
    VectorViewport *viewport = &document->viewport;
    float point = (400 - viewport->bounds.x - viewport->x) / viewport->zoom;
    SDL_Event event = {.type = SDL_EVENT_MOUSE_WHEEL};
    event.wheel.mouse_x = 400;
    event.wheel.mouse_y = 300;
    event.wheel.y = 1;
    TEST_ASSERT_TRUE(editor_host_event(&host, shell.view, &event));
    TEST_ASSERT_EQUAL_FLOAT(1.1f, viewport->zoom);
    TEST_ASSERT_FLOAT_WITHIN(.001f, point,
                             (400 - viewport->bounds.x - viewport->x) / viewport->zoom);
    event.type = SDL_EVENT_MOUSE_BUTTON_DOWN;
    event.button.button = SDL_BUTTON_MIDDLE;
    event.button.x = 400;
    event.button.y = 300;
    TEST_ASSERT_TRUE(editor_host_event(&host, shell.view, &event));
    float previous_x = viewport->x;
    event.type = SDL_EVENT_MOUSE_MOTION;
    event.motion.x = 425;
    event.motion.y = 310;
    TEST_ASSERT_TRUE(editor_host_event(&host, shell.view, &event));
    TEST_ASSERT_EQUAL_FLOAT(previous_x + 25, viewport->x);
    event.type = SDL_EVENT_WINDOW_FOCUS_LOST;
    editor_host_event(&host, shell.view, &event);
    TEST_ASSERT_FALSE(viewport->panning);
    event.type = SDL_EVENT_MOUSE_WHEEL;
    event.wheel.mouse_y = 300;
    event.wheel.mouse_x = 10;
    TEST_ASSERT_FALSE(editor_host_event(&host, shell.view, &event));
    draw_artboard();
    pui_view_draw(shell.view, &shell.input, (PUIRect){0, 0, 800, 600}, NULL, NULL);
    surface = SDL_RenderReadPixels(renderer, NULL);
    TEST_ASSERT_TRUE(SDL_ReadSurfacePixel(surface, 400, 300, &r, &g, &b, &a));
    TEST_ASSERT_EQUAL_UINT8(242, r);
    TEST_ASSERT_TRUE(SDL_ReadSurfacePixel(surface, 100, 300, &r, &g, &b, &a));
    TEST_ASSERT_NOT_EQUAL(242, r);
    SDL_DestroySurface(surface);
}

static void test_drag_normalizes_zoomed_coordinates_and_commits_once(void) {
    activate("vector");
    VectorDocument *document = add_vector_document();
    document->viewport.zoom = 2;
    document->viewport.x = 31;
    document->viewport.y = -17;
    TEST_ASSERT_TRUE(editor_host_action(&host, shell.view, "vector.add_artboard", ""));
    TEST_ASSERT_EQUAL_INT(0, document->history.count);
    SDL_Event event = {.type = SDL_EVENT_MOUSE_BUTTON_DOWN};
    event.button.button = SDL_BUTTON_LEFT;
    event.button.x = 450;
    event.button.y = 400;
    canvas_event(&event);
    event.type = SDL_EVENT_MOUSE_MOTION;
    event.motion.x = 350;
    event.motion.y = 250;
    canvas_event(&event);
    TEST_ASSERT_EQUAL_INT(0, document->artboard_count);
    TEST_ASSERT_EQUAL_INT(0, document->history.count);
    event.type = SDL_EVENT_MOUSE_BUTTON_UP;
    event.button.button = SDL_BUTTON_LEFT;
    event.button.x = 350;
    event.button.y = 250;
    canvas_event(&event);
    TEST_ASSERT_EQUAL_INT(1, document->history.count);
    TEST_ASSERT_EQUAL_INT(1, document->artboard_count);
    PUIRect rect = document->artboards[0].rect;
    TEST_ASSERT_EQUAL_FLOAT(50, rect.w);
    TEST_ASSERT_EQUAL_FLOAT(75, rect.h);
    TEST_ASSERT_EQUAL_FLOAT(roundf((350 - document->viewport.bounds.x - 31) / 2), rect.x);
    TEST_ASSERT_EQUAL_FLOAT(roundf((250 - document->viewport.bounds.y + 17) / 2), rect.y);
    TEST_ASSERT_FALSE(document->artboard_tool.enabled);
    TEST_ASSERT_TRUE(vector_history_undo(&document->history, document));
    TEST_ASSERT_EQUAL_INT(0, document->artboard_count);
    TEST_ASSERT_TRUE(vector_history_redo(&document->history, document));
    TEST_ASSERT_EQUAL_MEMORY(&rect, &document->artboards[0].rect, sizeof rect);
}

static void test_artboard_cancel_empty_drag_and_release_outside_canvas(void) {
    activate("vector");
    VectorDocument *document = add_vector_document();
    TEST_ASSERT_TRUE(editor_host_action(&host, shell.view, "vector.add_artboard", ""));
    SDL_Event event = {.type = SDL_EVENT_MOUSE_BUTTON_DOWN};
    event.button.button = SDL_BUTTON_LEFT;
    event.button.x = 350;
    event.button.y = 250;
    canvas_event(&event);
    event.type = SDL_EVENT_MOUSE_BUTTON_UP;
    canvas_event(&event);
    TEST_ASSERT_EQUAL_INT(0, document->history.count);
    TEST_ASSERT_TRUE(document->artboard_tool.enabled);
    event.type = SDL_EVENT_MOUSE_BUTTON_DOWN;
    canvas_event(&event);
    event.type = SDL_EVENT_KEY_DOWN;
    event.key.key = SDLK_ESCAPE;
    canvas_event(&event);
    TEST_ASSERT_FALSE(document->artboard_tool.dragging);
    TEST_ASSERT_FALSE(document->artboard_tool.enabled);
    TEST_ASSERT_EQUAL_INT(0, document->history.count);
    TEST_ASSERT_TRUE(editor_host_action(&host, shell.view, "vector.add_artboard", ""));
    event = (SDL_Event){.type = SDL_EVENT_MOUSE_BUTTON_DOWN};
    event.button.button = SDL_BUTTON_LEFT;
    event.button.x = 350;
    event.button.y = 250;
    canvas_event(&event);
    event.type = SDL_EVENT_WINDOW_FOCUS_LOST;
    canvas_event(&event);
    TEST_ASSERT_EQUAL_INT(0, document->artboard_count);
    TEST_ASSERT_FALSE(document->artboard_tool.enabled);
    TEST_ASSERT_TRUE(editor_host_action(&host, shell.view, "vector.add_artboard", ""));
    event = (SDL_Event){.type = SDL_EVENT_MOUSE_BUTTON_DOWN};
    event.button.button = SDL_BUTTON_LEFT;
    event.button.x = 350;
    event.button.y = 250;
    canvas_event(&event);
    event.type = SDL_EVENT_MOUSE_BUTTON_UP;
    event.button.x = 700;
    event.button.y = 500;
    canvas_event(&event);
    TEST_ASSERT_EQUAL_INT(1, document->artboard_count);
    TEST_ASSERT_EQUAL_FLOAT(350, document->artboards[0].rect.w);
    TEST_ASSERT_EQUAL_FLOAT(250, document->artboards[0].rect.h);
    TEST_ASSERT_FALSE(vector_artboard_add(document, (PUIRect){0, 0, 0, 10}));
    TEST_ASSERT_FALSE(vector_artboard_add(document, (PUIRect){0, 0, INFINITY, 10}));
    TEST_ASSERT_EQUAL_INT(1, document->history.count);
}

static void test_ruler_ticks_follow_document_scale_and_preserve_canvas_clip(void) {
    activate("vector");
    VectorDocument *document = add_vector_document();
    document->viewport.x = 40;
    document->viewport.y = 50;
    TEST_ASSERT_TRUE(vector_artboard_add(document, (PUIRect){0, 0, 80, 100}));
    for (int zoom = 1; zoom <= 2; zoom++) {
        document->viewport.zoom = (float)zoom;
        pui_view_draw(shell.view, &shell.input, (PUIRect){0, 0, 800, 600}, NULL, NULL);
        SDL_Surface *surface = SDL_RenderReadPixels(renderer, NULL);
        TEST_ASSERT_NOT_NULL(surface);
        int x = (int)document->viewport.bounds.x + 40;
        int y = (int)document->viewport.bounds.y + 50;
        Uint8 r, g, b, a;
        TEST_ASSERT_TRUE(SDL_ReadSurfacePixel(surface, x + 10, y - 2, &r, &g, &b, &a));
        TEST_ASSERT_EQUAL_UINT8(zoom == 1 ? 151 : 83, r);
        TEST_ASSERT_TRUE(SDL_ReadSurfacePixel(surface, x + 30, y + 30, &r, &g, &b, &a));
        TEST_ASSERT_EQUAL_UINT8(242, r);
        SDL_DestroySurface(surface);
    }
}

static void test_ruler_pointer_marks_follow_zoomed_board_and_preserve_clip(void) {
    SDL_Rect clip = {20, 20, 700, 500};
    SDL_SetRenderClipRect(renderer, &clip);
    for (int zoom = 1; zoom <= 2; zoom++) {
        SDL_SetRenderDrawColor(renderer, 104, 104, 104, 255);
        SDL_RenderClear(renderer);
        SDL_FRect board = {150, 120, 100 * zoom, 80 * zoom};
        float x = board.x + 23 * zoom, y = board.y + 31 * zoom;
        vector_ruler_draw(renderer, board, (float)zoom);
        vector_ruler_draw_pointer(renderer, board, x, y);
        SDL_Rect actual;
        TEST_ASSERT_TRUE(SDL_RenderClipEnabled(renderer));
        SDL_GetRenderClipRect(renderer, &actual);
        TEST_ASSERT_EQUAL_MEMORY(&clip, &actual, sizeof clip);
        SDL_Surface *surface = SDL_RenderReadPixels(renderer, NULL);
        TEST_ASSERT_NOT_NULL(surface);
        Uint8 r, g, b, a;
        TEST_ASSERT_TRUE(SDL_ReadSurfacePixel(surface, (int)x, 112, &r, &g, &b, &a));
        TEST_ASSERT_EQUAL_UINT8(0, r);
        TEST_ASSERT_EQUAL_UINT8(0, g);
        TEST_ASSERT_EQUAL_UINT8(0, b);
        TEST_ASSERT_TRUE(SDL_ReadSurfacePixel(surface, 142, (int)y, &r, &g, &b, &a));
        TEST_ASSERT_EQUAL_UINT8(0, r);
        /* Neighbouring pixels stay gray, so the indicator remains one screen pixel wide. */
        TEST_ASSERT_TRUE(SDL_ReadSurfacePixel(surface, (int)x + 1, 112, &r, &g, &b, &a));
        TEST_ASSERT_NOT_EQUAL(0, r);
        SDL_DestroySurface(surface);
        SDL_SetRenderDrawColor(renderer, 104, 104, 104, 255);
        SDL_RenderClear(renderer);
        vector_ruler_draw(renderer, board, (float)zoom);
        vector_ruler_draw_pointer(renderer, board, board.x - 1, y);
        vector_ruler_draw_pointer(renderer, board, x, board.y + board.h);
        surface = SDL_RenderReadPixels(renderer, NULL);
        TEST_ASSERT_NOT_NULL(surface);
        TEST_ASSERT_TRUE(SDL_ReadSurfacePixel(surface, (int)x, 112, &r, &g, &b, &a));
        TEST_ASSERT_NOT_EQUAL(0, r);
        TEST_ASSERT_TRUE(SDL_ReadSurfacePixel(surface, 142, (int)y, &r, &g, &b, &a));
        TEST_ASSERT_NOT_EQUAL(0, r);
        SDL_DestroySurface(surface);
    }
    SDL_SetRenderClipRect(renderer, NULL);
}

static void artboard_action(void *context, const char *name, const char *widget) {
    TEST_ASSERT_EQUAL_STRING("vector.add_artboard", name);
    TEST_ASSERT_TRUE(editor_host_action(context, shell.view, name, widget));
}

static void test_artboard_tool_button_stays_active_until_cancel_or_creation(void) {
    activate("vector");
    VectorDocument *document = add_vector_document();
    pui_pointer(&shell.input, 338, 70, true);
    pui_pointer(&shell.input, 338, 70, false);
    pui_view_draw(shell.view, &shell.input, (PUIRect){0, 0, 800, 600}, artboard_action, &host);
    TEST_ASSERT_TRUE(document->artboard_tool.enabled);
    TEST_ASSERT_TRUE(pui_view_checked(shell.view, "vector_add_artboard"));
    SDL_Event event = {.type = SDL_EVENT_MOUSE_BUTTON_DOWN};
    event.button.button = SDL_BUTTON_LEFT;
    event.button.x = 350;
    event.button.y = 250;
    canvas_event(&event);
    TEST_ASSERT_TRUE(pui_view_checked(shell.view, "vector_add_artboard"));
    event.type = SDL_EVENT_MOUSE_BUTTON_UP;
    canvas_event(&event);
    TEST_ASSERT_EQUAL_INT(0, document->artboard_count);
    TEST_ASSERT_TRUE(pui_view_checked(shell.view, "vector_add_artboard"));
    event.type = SDL_EVENT_KEY_DOWN;
    event.key.key = SDLK_ESCAPE;
    canvas_event(&event);
    TEST_ASSERT_FALSE(pui_view_checked(shell.view, "vector_add_artboard"));
    draw_artboard();
    TEST_ASSERT_EQUAL_INT(1, document->artboard_count);
    TEST_ASSERT_FALSE(pui_view_checked(shell.view, "vector_add_artboard"));
    TEST_ASSERT_TRUE(editor_host_action(&host, shell.view, "vector.add_artboard", ""));
    TEST_ASSERT_TRUE(pui_view_checked(shell.view, "vector_add_artboard"));
    event.type = SDL_EVENT_WINDOW_FOCUS_LOST;
    canvas_event(&event);
    TEST_ASSERT_FALSE(pui_view_checked(shell.view, "vector_add_artboard"));
    TEST_ASSERT_TRUE(editor_host_action(&host, shell.view, "vector.add_artboard", ""));
    add_vector_document();
    TEST_ASSERT_FALSE(pui_view_checked(shell.view, "vector_add_artboard"));
}

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_ruler_pointer_marks_follow_zoomed_board_and_preserve_clip);
    RUN_TEST(test_artboard_tool_button_stays_active_until_cancel_or_creation);
    RUN_TEST(test_ruler_ticks_follow_document_scale_and_preserve_canvas_clip);
    RUN_TEST(test_drag_normalizes_zoomed_coordinates_and_commits_once);
    RUN_TEST(test_artboard_cancel_empty_drag_and_release_outside_canvas);
    RUN_TEST(test_artboard_history_is_per_document_and_discards_redo_branch);
    RUN_TEST(test_canvas_is_clipped_and_zoom_keeps_point_under_pointer);
    RUN_TEST(test_failed_document_tab_does_not_consume_document_number);
    RUN_TEST(test_add_document_button_appends_and_selects_tabs_between_frames);
    RUN_TEST(test_append_tab_retains_existing_input_values_and_dragged_order);
    RUN_TEST(test_toolbar_action_queues_switch_until_commit);
    RUN_TEST(test_successful_switch_and_failed_mount_release_editor_instances);
    RUN_TEST(test_all_editors_switch_and_same_editor_is_not_recreated);
    RUN_TEST(test_failure_retains_old_editor_and_explicit_reload_recreates_it);
    RUN_TEST(test_config_reads_editor_and_rejects_invalid_value_atomically);
    RUN_TEST(test_mount_preserves_outer_values_resets_child_and_clears_focus);
    return UNITY_END();
}
