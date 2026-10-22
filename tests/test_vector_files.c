#include "documents/controller.h"
#include "editor_host/host.h"
#include "editor_host/registry.h"
#include "editors/vector/curve/curve.h"
#include "editors/vector/document/selection.h"
#include "editors/vector/ellipse/ellipse.h"
#include "editors/vector/ellipse/render.h"
#include "editors/vector/internal.h"
#include "editors/vector/object/clipboard.h"
#include "editors/vector/path/operations.h"
#include "editors/vector/persistence/document_file.h"
#include "editors/vector/polygon/polygon.h"
#include "editors/vector/text/text.h"
#include "editors/vector/text/tool.h"
#include "editors/vector/workspace/box_tool.h"
#include "editors/vector/workspace/files.h"
#include "shell/shell.h"
#include "unity.h"
#include <math.h>
#include <stdlib.h>
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

static char path[4096];

static void filename(void) {
    char *cwd = SDL_GetCurrentDirectory();
    snprintf(path, sizeof path, "%sroundtrip-%llu.pengvec", cwd,
             (unsigned long long)SDL_GetPerformanceCounter());
    SDL_free(cwd);
}

static void test_format_roundtrip_all_shapes_and_saved_baseline(void) {
    filename();
    VectorDocument *d = document();
    char error[256];
    TEST_ASSERT_TRUE(vector_rectangle_add(d, 1, (PUIRect){10, 20, 50, 60}));
    TEST_ASSERT_TRUE(vector_rectangle_set_radius(d, 1, 13));
    TEST_ASSERT_TRUE(vector_ellipse_add(d, 1, (PUIRect){100, 20, 50, 60}));
    TEST_ASSERT_TRUE(vector_polygon_add(d, 1, (PUIRect){200, 20, 50, 60}, 7));
    TEST_ASSERT_TRUE(vector_text_add(d, 1, (PUIRect){10, 100, 300, 200}));
    VectorText text = d->rectangles[3].text;
    text.bold = 1;
    text.italic = 1;
    text.columns = 2;
    text.letter_spacing = 1.5f;
    snprintf(text.content, sizeof text.content, "Hallo äöü\nMehrere Spalten");
    TEST_ASSERT_TRUE(vector_text_update(d, 4, &text));
    TEST_ASSERT_TRUE(
        vector_rectangle_set_stroke(d, 2, (VectorStroke){0xff7700ff, 3, VECTOR_STROKE_INSIDE}));
    TEST_ASSERT_TRUE(vector_document_file_save(d, path, error, sizeof error));
    TEST_ASSERT_EQUAL_STRING(path, d->filename);
    VectorDocument *loaded = calloc(1, sizeof *loaded);
    TEST_ASSERT_NOT_NULL(loaded);
    vector_document_init(loaded, 2);
    TEST_ASSERT_TRUE_MESSAGE(vector_document_file_load(loaded, path, error, sizeof error), error);
    TEST_ASSERT_EQUAL_INT(d->artboard_count, loaded->artboard_count);
    TEST_ASSERT_EQUAL_INT(4, loaded->rectangle_count);
    TEST_ASSERT_EQUAL_MEMORY(d->artboards, loaded->artboards, sizeof d->artboards[0]);
    for (int i = 0; i < 4; i++) {
        VectorShape expected = d->rectangles[i], actual = loaded->rectangles[i];
        // Derived path buffers may retain unused samples; the format stores active values only.
        memset(expected.path.points + expected.path.count, 0,
               (VECTOR_PATH_POINTS - expected.path.count) * sizeof(VectorPoint));
        memset(actual.path.points + actual.path.count, 0,
               (VECTOR_PATH_POINTS - actual.path.count) * sizeof(VectorPoint));
        memset(expected.path.ends + expected.path.contours, 0,
               (VECTOR_PATH_CONTOURS - expected.path.contours) * sizeof(unsigned));
        memset(actual.path.ends + actual.path.contours, 0,
               (VECTOR_PATH_CONTOURS - actual.path.contours) * sizeof(unsigned));
        TEST_ASSERT_EQUAL_MEMORY(&expected, &actual, sizeof(VectorShape));
    }
    TEST_ASSERT_FALSE(vector_history_undo(&loaded->history, loaded));
    TEST_ASSERT_TRUE(vector_rectangle_set_fill(loaded, 1, 0xff0000ff));
    TEST_ASSERT_TRUE(vector_history_undo(&loaded->history, loaded));
    TEST_ASSERT_EQUAL_UINT(d->rectangles[0].fill, loaded->rectangles[0].fill);
    vector_history_destroy(&loaded->history);
    free(loaded);
    SDL_RemovePath(path);
}

static void test_saved_tabs_open_and_survive_editor_switch_with_undo(void) {
    filename();
    char error[256];
    VectorDocument *d = document();
    TEST_ASSERT_TRUE(vector_ellipse_add(d, 1, (PUIRect){30, 40, 80, 90}));
    TEST_ASSERT_TRUE(
        vector_workspace_save_file(host.instance, shell.view, d->id, path, error, sizeof error));
    int count = ((VectorEditor *)host.instance)->documents.count;
    TEST_ASSERT_TRUE(vector_workspace_open_file(host.instance, shell.view, &shell.input, path,
                                                error, sizeof error));
    commit();
    d = document();
    TEST_ASSERT_EQUAL_INT(count + 1, ((VectorEditor *)host.instance)->documents.count);
    TEST_ASSERT_EQUAL_STRING(path, d->filename);
    TEST_ASSERT_FALSE(vector_history_undo(&d->history, d));
    TEST_ASSERT_TRUE(vector_rectangle_set_fill(d, 1, 0xff0000ff));
    activate("scripts");
    activate("vector");
    d = document();
    TEST_ASSERT_EQUAL_STRING(path, d->filename);
    TEST_ASSERT_TRUE(vector_history_undo(&d->history, d));
    TEST_ASSERT_EQUAL_UINT(VECTOR_RECTANGLE_DEFAULT_FILL, d->rectangles[0].fill);
    TEST_ASSERT_FALSE(vector_history_undo(&d->history, d));
    TEST_ASSERT_TRUE(
        vector_workspace_save_file(host.instance, shell.view, d->id, path, error, sizeof error));
    SDL_RemovePath(path);
}

static void test_invalid_files_and_failed_save_preserve_state(void) {
    filename();
    VectorDocument *d = document();
    char error[256];
    TEST_ASSERT_TRUE(vector_document_file_save(d, path, error, sizeof error));
    SDL_IOStream *io = SDL_IOFromFile(path, "r+b");
    TEST_ASSERT_NOT_NULL(io);
    SDL_SeekIO(io, 8, SDL_IO_SEEK_SET);
    const unsigned char badversion[4] = {99, 0, 0, 0};
    SDL_WriteIO(io, badversion, 4);
    SDL_CloseIO(io);
    int count = ((VectorEditor *)host.instance)->documents.count;
    TEST_ASSERT_FALSE(vector_workspace_open_file(host.instance, shell.view, &shell.input, path,
                                                 error, sizeof error));
    TEST_ASSERT_EQUAL_INT(count, ((VectorEditor *)host.instance)->documents.count);
    TEST_ASSERT_EQUAL_PTR(d, document());
    TEST_ASSERT_FALSE(
        vector_document_file_save(d, "/missing-dir/doc.pengvec", error, sizeof error));
    TEST_ASSERT_EQUAL_STRING(path, d->filename);
    SDL_RemovePath(path);
}

static void test_app_save_routing_and_default_project_directory(void) {
    filename();
    char default_path[4096];
    EditorFileInfo info = {.document = 1, .extension = "pengvec"};
    snprintf(info.title, sizeof info.title, "Dokument 1");
    TEST_ASSERT_TRUE(
        document_save_default_path(default_path, sizeof default_path, &info, "/tmp/Mein Projekt"));
    TEST_ASSERT_EQUAL_STRING("/tmp/Mein Projekt/Dokument 1.pengvec", default_path);
    TEST_ASSERT_EQUAL_STRING("vector", editor_registry_for_file(path)->id);
    TEST_ASSERT_NULL(editor_registry_for_file("picture.svg"));
    VectorDocument *d = document();
    char error[256];
    TEST_ASSERT_TRUE(vector_document_file_save(d, path, error, sizeof error));
    DocumentController controller;
    document_controller_init(&controller);
    ProjectController projects = {0};
    TEST_ASSERT_TRUE(document_controller_action(&controller, &host, &projects, window, shell.view,
                                                "editor.save", ""));
    TEST_ASSERT_FALSE(controller.dialog_pending);
    TEST_ASSERT_EQUAL_STRING(path, controller.pending_save);
    document_controller_commit(&controller, &host, &projects, shell.view, &shell.input);
    TEST_ASSERT_EQUAL_STRING("", controller.pending_save);
    document_controller_destroy(&controller);
    SDL_RemovePath(path);
}

/* SDL directory enumeration may use native separators on Windows, while the
 * fixture joins paths with '/'. Both spellings identify the same Windows file. */
static bool same_file_path(const char *a, const char *b) {
#ifdef _WIN32
    while (*a && *b) {
        char x = *a == '\\' ? '/' : *a;
        char y = *b == '\\' ? '/' : *b;
        if (SDL_tolower((unsigned char)x) != SDL_tolower((unsigned char)y)) {
            return false;
        }
        a++;
        b++;
    }
    return !*a && !*b;
#else
    return !strcmp(a, b);
#endif
}

static void test_explorer_routes_file_from_other_editor_to_own_tab(void) {
    char *cwd = SDL_GetCurrentDirectory(), folder[4096], file[4096], metadata[4096], error[256];
    snprintf(folder, sizeof folder, "%sfile-project-%llu", cwd,
             (unsigned long long)SDL_GetPerformanceCounter());
    SDL_free(cwd);
    TEST_ASSERT_TRUE(SDL_CreateDirectory(folder));
    snprintf(file, sizeof file, "%s/Artwork.pengvec", folder);
    snprintf(metadata, sizeof metadata, "%s/project.pengui.yaml", folder);
    ProjectController project;
    TEST_ASSERT_TRUE(
        project_controller_init(&project, PENGUI_EDITOR_RESOURCES, false, error, sizeof error));
    TEST_ASSERT_TRUE(project_controller_open(&project, folder, true, shell.view));
    TEST_ASSERT_TRUE(vector_document_file_save(document(), file, error, sizeof error));
    TEST_ASSERT_TRUE(
        project_explorer_commit(&project.explorer, shell.view, &shell.input, error, sizeof error));
    char row[64] = {0};
    bool found = false;
    for (unsigned id = 1; id < 10; id++) {
        snprintf(row, sizeof row, "project_entry_%u", id);
        const char *candidate = project_explorer_file(&project.explorer, row);
        if (candidate && same_file_path(candidate, file)) {
            found = true;
            break;
        }
    }
    TEST_ASSERT_TRUE(found);
    activate("scripts");
    DocumentController controller;
    document_controller_init(&controller);
    TEST_ASSERT_TRUE(document_controller_action(&controller, &host, &project, window, shell.view,
                                                "project.entry", row));
    TEST_ASSERT_TRUE(editor_host_commit(&host, shell.view, &shell.input, error, sizeof error));
    document_controller_commit(&controller, &host, &project, shell.view, &shell.input);
    commit();
    TEST_ASSERT_EQUAL_STRING("vector", host.active->id);
    TEST_ASSERT_TRUE(same_file_path(file, document()->filename));
    TEST_ASSERT_EQUAL_INT(2, ((VectorEditor *)host.instance)->documents.count);
    document_controller_destroy(&controller);
    project_controller_destroy(&project);
    SDL_RemovePath(file);
    SDL_RemovePath(metadata);
    SDL_RemovePath(folder);
}

static void test_legacy_version_six_defaults_to_square_corners(void) {
    filename();
    VectorDocument *d = document();
    char error[256];
    TEST_ASSERT_TRUE(vector_rectangle_add(d, 1, (PUIRect){10, 20, 50, 60}));
    TEST_ASSERT_TRUE(vector_rectangle_set_radius(d, 1, 13));
    TEST_ASSERT_TRUE(vector_document_file_save(d, path, error, sizeof error));
    size_t length;
    unsigned char *bytes = SDL_LoadFile(path, &length);
    TEST_ASSERT_NOT_NULL(bytes);
    TEST_ASSERT_EQUAL_UINT8(9, bytes[8]);
    bytes[8] = 6;
    TEST_ASSERT_TRUE(SDL_SaveFile(path, bytes, length - 28));
    SDL_free(bytes);
    VectorDocument *loaded = calloc(1, sizeof *loaded);
    TEST_ASSERT_NOT_NULL(loaded);
    vector_document_init(loaded, 2);
    TEST_ASSERT_TRUE_MESSAGE(vector_document_file_load(loaded, path, error, sizeof error), error);
    TEST_ASSERT_EQUAL_FLOAT(0, loaded->rectangles[0].corner_radius);
    vector_history_destroy(&loaded->history);
    free(loaded);
    SDL_RemovePath(path);
}

static void test_tab_marks_unsaved_changes_and_tracks_saved_content(void) {
    filename();
    VectorDocument *d = document();
    char error[256], tab[64], marked[256];
    snprintf(tab, sizeof tab, "vector_document_%u", d->id);
    snprintf(marked, sizeof marked, "%s *", d->title);
    commit();
    TEST_ASSERT_EQUAL_STRING(marked, pui_view_text(shell.view, tab));
    TEST_ASSERT_TRUE(
        vector_workspace_save_file(host.instance, shell.view, d->id, path, error, sizeof error));
    commit();
    TEST_ASSERT_EQUAL_STRING(d->title, pui_view_text(shell.view, tab));
    snprintf(marked, sizeof marked, "%s *", d->title);
    TEST_ASSERT_TRUE(vector_artboard_add(d, (PUIRect){700, 0, 100, 100}));
    commit();
    TEST_ASSERT_EQUAL_STRING(marked, pui_view_text(shell.view, tab));
    TEST_ASSERT_TRUE(vector_history_undo(&d->history, d));
    commit();
    TEST_ASSERT_EQUAL_STRING(d->title, pui_view_text(shell.view, tab));
    TEST_ASSERT_TRUE(vector_history_redo(&d->history, d));
    commit();
    TEST_ASSERT_EQUAL_STRING(marked, pui_view_text(shell.view, tab));
    activate("scripts");
    activate("vector");
    commit();
    TEST_ASSERT_EQUAL_STRING(marked, pui_view_text(shell.view, tab));
    SDL_RemovePath(path);
}

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_tab_marks_unsaved_changes_and_tracks_saved_content);
    RUN_TEST(test_legacy_version_six_defaults_to_square_corners);
    RUN_TEST(test_explorer_routes_file_from_other_editor_to_own_tab);
    RUN_TEST(test_app_save_routing_and_default_project_directory);
    RUN_TEST(test_format_roundtrip_all_shapes_and_saved_baseline);
    RUN_TEST(test_saved_tabs_open_and_survive_editor_switch_with_undo);
    RUN_TEST(test_invalid_files_and_failed_save_preserve_state);
    return UNITY_END();
}
