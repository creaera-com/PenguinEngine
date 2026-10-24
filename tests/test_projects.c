#include "editor/config.h"
#include "editor_host/host.h"
#include "editors/vector/artboard/tool.h"
#include "editors/vector/internal.h"
#include "editors/vector/workspace/ruler.h"
#include "project/controller.h"
#include "project/directories.h"
#include "project/state.h"
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

static char folder[4096];

static void temp_folder(void) {
    char *cwd = SDL_GetCurrentDirectory();
    snprintf(folder, sizeof folder, "%sproject-test-%llu", cwd,
             (unsigned long long)SDL_GetPerformanceCounter());
    SDL_free(cwd);
    TEST_ASSERT_TRUE(SDL_CreateDirectory(folder));
}

static void file(const char *path, const char *text) {
    SDL_IOStream *io = SDL_IOFromFile(path, "wb");
    TEST_ASSERT_NOT_NULL(io);
    TEST_ASSERT_EQUAL_UINT(strlen(text), SDL_WriteIO(io, text, strlen(text)));
    TEST_ASSERT_TRUE(SDL_CloseIO(io));
}

static SDL_EnumerationResult remove_child(void *context, const char *directory, const char *name) {
    (void)context;
    char path[4096];
    snprintf(path, sizeof path, "%s%s", directory, name);
    SDL_PathInfo info;
    TEST_ASSERT_TRUE(SDL_GetPathInfo(path, &info));
    if (info.type == SDL_PATHTYPE_DIRECTORY) {
        SDL_EnumerateDirectory(path, remove_child, NULL);
    }
    TEST_ASSERT_TRUE(SDL_RemovePath(path));
    return SDL_ENUM_CONTINUE;
}

static void remove_folder(void) {
    SDL_EnumerateDirectory(folder, remove_child, NULL);
    TEST_ASSERT_TRUE(SDL_RemovePath(folder));
}

static void test_create_validate_no_overwrite_and_state_roundtrip(void) {
    temp_folder();
    char error[256], state[4096], value[4096];
    AppProject p = {0};
    TEST_ASSERT_TRUE_MESSAGE(app_project_create(&p, folder, error, sizeof error), error);
    TEST_ASSERT_EQUAL_STRING(folder, p.root);
    AppProject before = p;
    TEST_ASSERT_FALSE(app_project_create(&p, folder, error, sizeof error));
    TEST_ASSERT_EQUAL_MEMORY(&before, &p, sizeof p);
    snprintf(state, sizeof state, "%s/last-project.txt", folder);
    TEST_ASSERT_TRUE(app_project_state_save(state, p.file));
    TEST_ASSERT_TRUE(app_project_state_read(state, value, sizeof value));
    TEST_ASSERT_EQUAL_STRING(p.file, value);
    TEST_ASSERT_TRUE(app_project_state_save(state, "/a different/project.yaml"));
    TEST_ASSERT_TRUE(app_project_state_read(state, value, sizeof value));
    TEST_ASSERT_EQUAL_STRING("/a different/project.yaml", value);
    file(p.file, "schema: invalid\nname: nope\n");
    TEST_ASSERT_FALSE(app_project_open(&p, p.file, error, sizeof error));
    TEST_ASSERT_EQUAL_MEMORY(&before, &p, sizeof p);
    remove_folder();
}

static void test_shared_explorer_lazy_folders_paging_and_editor_switch(void) {
    temp_folder();
    char error[256] = {0}, path[4096];
    ProjectController projects;
    TEST_ASSERT_TRUE(
        project_controller_init(&projects, PENGUI_EDITOR_RESOURCES, false, error, sizeof error));
    activate("scripts");
    TEST_ASSERT_TRUE(project_controller_open(&projects, folder, true, shell.view));
    snprintf(path, sizeof path, "%s/Assets", folder);
    TEST_ASSERT_TRUE(SDL_CreateDirectory(path));
    snprintf(path, sizeof path, "%s/Assets/image.png", folder);
    file(path, "example");
    for (int i = 0; i < 40; i++) {
        snprintf(path, sizeof path, "%s/file-%02d.txt", folder, i);
        file(path, "example");
    }
    TEST_ASSERT_TRUE(
        project_explorer_commit(&projects.explorer, shell.view, &shell.input, error, sizeof error));
    TEST_ASSERT_GREATER_THAN_INT(30, pui_view_tree_item_count(shell.view, "project_files"));
    // Choose the folder by its row hit position, not its enumeration-dependent identity.
    pui_view_draw(shell.view, &shell.input, (PUIRect){0, 0, 800, 600}, NULL, NULL);
    const char *folder_row = pui_view_tree_item_at(shell.view, "project_files", 70, 210);
    if (folder_row) {
        project_explorer_action(&projects.explorer, shell.view, "project.entry", folder_row);
    }
    TEST_ASSERT_TRUE(
        project_explorer_commit(&projects.explorer, shell.view, &shell.input, error, sizeof error));
    // Root ID 1, Assets was discovered immediately after the project metadata (ID varies).
    project_explorer_action(&projects.explorer, shell.view, "project.page", "project_page_1");
    TEST_ASSERT_TRUE(
        project_explorer_commit(&projects.explorer, shell.view, &shell.input, error, sizeof error));
    TEST_ASSERT_LESS_THAN_INT(30, pui_view_tree_item_count(shell.view, "project_files"));
    activate("game");
    TEST_ASSERT_TRUE(
        project_explorer_commit(&projects.explorer, shell.view, &shell.input, error, sizeof error));
    TEST_ASSERT_GREATER_THAN_INT(0, pui_view_tree_item_count(shell.view, "project_files"));
    activate("vector");
    TEST_ASSERT_TRUE(
        project_explorer_commit(&projects.explorer, shell.view, &shell.input, error, sizeof error));
    TEST_ASSERT_EQUAL_STRING(folder, projects.project.root);
    project_controller_destroy(&projects);
    remove_folder();
}

static void test_linux_uses_penguengine_state_and_restores_project(void) {
#ifdef __linux__
    temp_folder();
    char error[256], state[4096];
    AppProject p = {0};
    TEST_ASSERT_TRUE(app_project_create(&p, folder, error, sizeof error));
    const char *old = SDL_getenv("XDG_STATE_HOME");
    char *saved = old ? SDL_strdup(old) : NULL;
    SDL_SetEnvironmentVariable(SDL_GetEnvironment(), "XDG_STATE_HOME", folder, true);
    TEST_ASSERT_TRUE(app_project_state_path(state, sizeof state));
    TEST_ASSERT_NOT_NULL(strstr(state, "/penguengine/last-project.txt"));
    TEST_ASSERT_TRUE(app_project_state_save(state, p.file));
    ProjectController c;
    error[0] = 0;
    TEST_ASSERT_TRUE(
        project_controller_init(&c, PENGUI_EDITOR_RESOURCES, true, error, sizeof error));
    TEST_ASSERT_EQUAL_STRING(p.file, c.project.file);
    project_controller_destroy(&c);
    SDL_RemovePath(p.file);
    error[0] = 0;
    TEST_ASSERT_TRUE(
        project_controller_init(&c, PENGUI_EDITOR_RESOURCES, true, error, sizeof error));
    TEST_ASSERT_EQUAL_STRING("", c.project.file);
    TEST_ASSERT_NOT_EQUAL(0, error[0]);
    project_controller_destroy(&c);
    if (saved) {
        SDL_SetEnvironmentVariable(SDL_GetEnvironment(), "XDG_STATE_HOME", saved, true);
    } else {
        SDL_UnsetEnvironmentVariable(SDL_GetEnvironment(), "XDG_STATE_HOME");
    }
    SDL_free(saved);
    remove_folder();
#endif
}

static void test_create_directories_from_explorer_selection(void) {
    temp_folder();
    char error[256], path[4096];
    ProjectController c;
    TEST_ASSERT_TRUE(
        project_controller_init(&c, PENGUI_EDITOR_RESOURCES, false, error, sizeof error));
    activate("scripts");
    TEST_ASSERT_TRUE(project_controller_open(&c, folder, true, shell.view));
    TEST_ASSERT_TRUE(
        project_explorer_commit(&c.explorer, shell.view, &shell.input, error, sizeof error));
    pui_view_set_text(shell.view, "project_folder_name", "Assets");
    TEST_ASSERT_TRUE(project_explorer_action(&c.explorer, shell.view, "project.folder.create", ""));
    snprintf(path, sizeof path, "%s/Assets", folder);
    SDL_PathInfo info;
    TEST_ASSERT_FALSE(SDL_GetPathInfo(path, &info)); // UI action only queues I/O.
    TEST_ASSERT_TRUE(
        project_explorer_commit(&c.explorer, shell.view, &shell.input, error, sizeof error));
    TEST_ASSERT_TRUE(SDL_GetPathInfo(path, &info));
    TEST_ASSERT_EQUAL_INT(SDL_PATHTYPE_DIRECTORY, info.type);
    TEST_ASSERT_NOT_NULL(pui_view_selected_tree_item(shell.view, "project_files"));
    pui_view_set_text(shell.view, "project_folder_name", "Textures");
    project_explorer_action(&c.explorer, shell.view, "project.folder.create", "");
    TEST_ASSERT_TRUE(
        project_explorer_commit(&c.explorer, shell.view, &shell.input, error, sizeof error));
    snprintf(path, sizeof path, "%s/Assets/Textures", folder);
    TEST_ASSERT_TRUE(SDL_GetPathInfo(path, &info));
    const char *bad[] = {"",     ".",   "..",      "../escape", "a/b",
                         "a\\b", "CON", "NUL.txt", "trailing.", " foo"};
    for (size_t i = 0; i < sizeof bad / sizeof *bad; i++) {
        TEST_ASSERT_FALSE(
            project_directory_create(folder, bad[i], path, sizeof path, error, sizeof error));
    }
    TEST_ASSERT_FALSE(
        project_directory_create(folder, "Assets", path, sizeof path, error, sizeof error));
    project_controller_destroy(&c);
    remove_folder();
}

static void test_file_search_finds_files_in_closed_folders_and_clears(void) {
    temp_folder();
    char error[256], path[4096];
    snprintf(path, sizeof path, "%s/Assets/Closed", folder);
    TEST_ASSERT_TRUE(SDL_CreateDirectory(path));
    snprintf(path, sizeof path, "%s/Assets/Closed/needle.txt", folder);
    file(path, "example");
    ProjectController c;
    TEST_ASSERT_TRUE(
        project_controller_init(&c, PENGUI_EDITOR_RESOURCES, false, error, sizeof error));
    activate("vector");
    TEST_ASSERT_TRUE(project_controller_open(&c, folder, true, shell.view));
    TEST_ASSERT_TRUE(
        project_explorer_commit(&c.explorer, shell.view, &shell.input, error, sizeof error));
    TEST_ASSERT_TRUE(pui_view_set_text(shell.view, "project_search", "NEEDLE"));
    for (int i = 0; i < 5; i++) {
        TEST_ASSERT_TRUE(
            project_explorer_commit(&c.explorer, shell.view, &shell.input, error, sizeof error));
    }
    TEST_ASSERT_EQUAL_INT(4, pui_view_tree_item_count(shell.view, "project_files"));
    TEST_ASSERT_TRUE(pui_view_set_text(shell.view, "project_search", "not-present"));
    TEST_ASSERT_TRUE(
        project_explorer_commit(&c.explorer, shell.view, &shell.input, error, sizeof error));
    TEST_ASSERT_EQUAL_INT(0, pui_view_tree_item_count(shell.view, "project_files"));
    TEST_ASSERT_TRUE(pui_view_set_text(shell.view, "project_search", ""));
    TEST_ASSERT_TRUE(
        project_explorer_commit(&c.explorer, shell.view, &shell.input, error, sizeof error));
    TEST_ASSERT_GREATER_THAN_INT(0, pui_view_tree_item_count(shell.view, "project_files"));
    project_controller_destroy(&c);
    remove_folder();
}

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_file_search_finds_files_in_closed_folders_and_clears);
    RUN_TEST(test_create_directories_from_explorer_selection);
    RUN_TEST(test_create_validate_no_overwrite_and_state_roundtrip);
    RUN_TEST(test_shared_explorer_lazy_folders_paging_and_editor_switch);
    RUN_TEST(test_linux_uses_penguengine_state_and_restores_project);
    return UNITY_END();
}
