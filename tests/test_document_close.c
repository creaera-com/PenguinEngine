/** @file @brief Close routing, native confirmation outcomes and document ownership regressions. */
#include "documents/controller.h"
#include "editors/vector/internal.h"
#include "editors/vector/persistence/document_file.h"
#include "editors/vector/workspace/files.h"
#include "shell/shell.h"
#include "unity.h"
#include <stdlib.h>
#include <string.h>

static SDL_Window *window;
static SDL_Renderer *renderer;
static EditorShell shell;
static EditorHost host;
static DocumentController controller;
static ProjectController projects;
static int choice, prompts;
static bool prompt_ok;
static SDL_DialogFileCallback dialog_callback;
static void *dialog_context;
static char saved_path[4096];

/* This executable replaces native dialogs only; production always uses SDL's implementation. */
static bool SDLCALL confirm(const SDL_MessageBoxData *data, int *result) {
    TEST_ASSERT_EQUAL_INT(3, data->numbuttons);
    prompts++;
    *result = choice;
    return prompt_ok;
}

static void SDLCALL save_dialog(SDL_DialogFileCallback callback, void *context, SDL_Window *parent,
                                const SDL_DialogFileFilter *filters, int count,
                                const char *location) {
    (void)parent;
    (void)filters;
    (void)count;
    TEST_ASSERT_NOT_NULL(location);
    dialog_callback = callback;
    dialog_context = context;
}

static void action(void *context, const char *name, const char *widget) {
    (void)context;
    if (!document_controller_action(&controller, &host, &projects, window, shell.view, name,
                                    widget)) {
        TEST_ASSERT_TRUE(editor_host_action(&host, shell.view, name, widget));
    }
}

static void frame(void) {
    char error[256];
    TEST_ASSERT_TRUE_MESSAGE(
        editor_host_commit(&host, shell.view, &shell.input, error, sizeof error), error);
    pui_view_draw(shell.view, &shell.input, (PUIRect){0, 0, 1280, 800}, action, NULL);
}

static VectorDocument *document(void) {
    return vector_document_tabs_active(&((VectorEditor *)host.instance)->documents, shell.view);
}

static void finish_close(void) {
    document_controller_commit(&controller, &host, &projects, shell.view, &shell.input);
    frame();
}

static void close_tab(unsigned id) {
    char name[64];
    snprintf(name, sizeof name, "vector_document_%u", id);
    action(NULL, "editor.close_document", name);
    finish_close();
}

void setUp(void) {
    TEST_ASSERT_TRUE(SDL_Init(SDL_INIT_VIDEO));
    window = SDL_CreateWindow("close tests", 1280, 800, SDL_WINDOW_HIDDEN);
    renderer = SDL_CreateRenderer(window, NULL);
    TEST_ASSERT_NOT_NULL(renderer);
    char path[1600], error[256];
    snprintf(path, sizeof path, "%s/shell.yaml", PENGUI_EDITOR_RESOURCES);
    TEST_ASSERT_TRUE_MESSAGE(editor_shell_load(&shell, renderer, path, error, sizeof error), error);
    editor_host_init(&host, (EditorServices){PENGUI_EDITOR_RESOURCES});
    document_controller_init(&controller);
    controller.confirm = confirm;
    controller.save_dialog = save_dialog;
    projects = (ProjectController){0};
    choice = 0;
    prompts = 0;
    prompt_ok = true;
    dialog_callback = NULL;
    dialog_context = NULL;
    saved_path[0] = 0;
    TEST_ASSERT_TRUE(editor_host_request(&host, "vector", false));
    frame();
    frame();
}

void tearDown(void) {
    if (dialog_callback) {
        const char *cancel[] = {NULL};
        dialog_callback(dialog_context, cancel, 0);
        dialog_callback = NULL;
    }
    document_controller_destroy(&controller);
    editor_shell_destroy(&shell);
    editor_host_destroy(&host);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
    if (*saved_path) {
        remove(saved_path);
    }
}

static void test_cancel_discard_and_empty_workspace_restore(void) {
    TEST_ASSERT_TRUE(vector_document_modified(document()));
    close_tab(1);
    TEST_ASSERT_EQUAL_INT(1, prompts);
    TEST_ASSERT_NOT_NULL(document());
    prompt_ok = false;
    choice = 1;
    close_tab(1);
    TEST_ASSERT_NOT_NULL(document());
    prompt_ok = true;
    close_tab(1);
    TEST_ASSERT_NULL(document());
    frame();
    TEST_ASSERT_EQUAL_INT(0, ((VectorEditor *)host.instance)->documents.count);
    TEST_ASSERT_EQUAL_INT(0, pui_view_tab_count(shell.view, "vector_document_tabs"));
    TEST_ASSERT_TRUE(editor_host_request(&host, "scripts", false));
    frame();
    TEST_ASSERT_TRUE(editor_host_request(&host, "vector", false));
    frame();
    frame();
    TEST_ASSERT_NULL(document());
    action(NULL, "vector.add_document", "");
    frame();
    TEST_ASSERT_EQUAL_UINT(2, document()->id);
}

static void test_close_inactive_tab_keeps_identity_and_checkpoint(void) {
    vector_document_mark_saved(document());
    action(NULL, "vector.add_document", "");
    frame();
    TEST_ASSERT_EQUAL_UINT(2, document()->id);
    close_tab(1);
    TEST_ASSERT_EQUAL_INT(0, prompts);
    TEST_ASSERT_EQUAL_UINT(2, document()->id);
    action(NULL, "vector.add_document", "");
    frame();
    TEST_ASSERT_EQUAL_UINT(3, document()->id);
    TEST_ASSERT_TRUE(editor_host_request(&host, "scripts", false));
    frame();
    TEST_ASSERT_TRUE(editor_host_request(&host, "vector", false));
    frame();
    VectorDocuments *docs = &((VectorEditor *)host.instance)->documents;
    TEST_ASSERT_EQUAL_INT(2, docs->count);
    TEST_ASSERT_NOT_NULL(vector_documents_find(docs, 2));
    TEST_ASSERT_NOT_NULL(vector_documents_find(docs, 3));
    close_tab(3); // Empty unsaved tab needs no prompt; selects its neighbor.
    TEST_ASSERT_EQUAL_UINT(2, document()->id);
}

static void test_save_before_close_and_failed_save_keeps_document(void) {
    char *cwd = SDL_GetCurrentDirectory();
    snprintf(saved_path, sizeof saved_path, "%sclose-%llu.pengvec", cwd,
             (unsigned long long)SDL_GetPerformanceCounter());
    SDL_free(cwd);
    char error[256];
    TEST_ASSERT_TRUE(
        vector_workspace_save_file(host.instance, shell.view, 1, saved_path, error, sizeof error));
    TEST_ASSERT_FALSE(vector_document_modified(document()));
    TEST_ASSERT_TRUE(vector_rectangle_add(document(), 1, (PUIRect){10, 10, 100, 100}));
    choice = 2;
    close_tab(1);
    TEST_ASSERT_NULL(document());
    VectorDocument *loaded = calloc(1, sizeof *loaded);
    TEST_ASSERT_NOT_NULL(loaded);
    vector_document_init(loaded, 1);
    TEST_ASSERT_TRUE_MESSAGE(vector_document_file_load(loaded, saved_path, error, sizeof error),
                             error);
    TEST_ASSERT_EQUAL_INT(1, loaded->rectangle_count);
    vector_history_destroy(&loaded->history);
    free(loaded);
    action(NULL, "vector.add_document", "");
    frame();
    VectorDocument *d = document();
    TEST_ASSERT_TRUE(vector_artboard_add(d, (PUIRect){0, 0, 100, 100}));
    snprintf(d->filename, sizeof d->filename, "/missing-directory/document.pengvec");
    close_tab(d->id);
    TEST_ASSERT_NOT_NULL(document());
    TEST_ASSERT_TRUE(vector_document_modified(document()));
}

static void test_save_as_cancel_keeps_document_open(void) {
    choice = 2;
    close_tab(1);
    TEST_ASSERT_TRUE(controller.dialog_pending);
    TEST_ASSERT_NOT_NULL(dialog_callback);
    TEST_ASSERT_NOT_NULL(document());
    const char *cancel[] = {NULL};
    dialog_callback(dialog_context, cancel, 0);
    dialog_callback = NULL;
    SDL_Event event;
    while (SDL_PollEvent(&event)) {
        document_controller_event(&controller, shell.view, &event);
    }
    finish_close();
    TEST_ASSERT_FALSE(controller.dialog_pending);
    TEST_ASSERT_FALSE(controller.close_after_save);
    TEST_ASSERT_NOT_NULL(document());
}

static void test_cross_button_requests_close_without_selecting_another_tab(void) {
    action(NULL, "vector.add_document", "");
    frame();
    TEST_ASSERT_EQUAL_UINT(2, document()->id);
    // Workspace starts at x=260; "Dokument 1" has 90px label + 24px close space.
    pui_pointer(&shell.input, 260 + 100, 107, true);
    pui_pointer(&shell.input, 260 + 100, 107, false);
    frame();
    TEST_ASSERT_EQUAL_STRING("vector_document_1", controller.close_request);
    TEST_ASSERT_EQUAL_UINT(2, document()->id);
    choice = 1;
    finish_close();
    TEST_ASSERT_EQUAL_UINT(2, document()->id);
    TEST_ASSERT_EQUAL_INT(1, pui_view_tab_count(shell.view, "vector_document_tabs"));
}

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_cancel_discard_and_empty_workspace_restore);
    RUN_TEST(test_close_inactive_tab_keeps_identity_and_checkpoint);
    RUN_TEST(test_save_before_close_and_failed_save_keeps_document);
    RUN_TEST(test_save_as_cancel_keeps_document_open);
    RUN_TEST(test_cross_button_requests_close_without_selecting_another_tab);
    return UNITY_END();
}
