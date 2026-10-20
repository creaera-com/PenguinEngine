#include "documents/controller.h"
#include "editor_host/host.h"
#include "editors/vector/document/selection.h"
#include "editors/vector/ellipse/ellipse.h"
#include "editors/vector/image/assets.h"
#include "editors/vector/image/import.h"
#include "editors/vector/image/render.h"
#include "editors/vector/internal.h"
#include "editors/vector/mask/mask.h"
#include "editors/vector/mask/render.h"
#include "editors/vector/object/commands.h"
#include "editors/vector/object/group.h"
#include "editors/vector/path/operations.h"
#include "editors/vector/persistence/document_file.h"
#include "editors/vector/preview/render.h"
#include "editors/vector/preview/tool.h"
#include "editors/vector/rectangle/render.h"
#include "editors/vector/rectangle/transform.h"
#include "editors/vector/text/text.h"
#include "editors/vector/ui/document_tree/groups.h"
#include "editors/vector/workspace/files.h"
#include "editors/vector/workspace/selection.h"
#include "shell/shell.h"
#include "unity.h"
#include <math.h>
#include <stdlib.h>
#include <string.h>

static SDL_Window *window;
static SDL_Renderer *renderer;
static EditorShell shell;
static EditorHost host;
static char folder[4096], root[4096], png[4096], jpg[4096];
static void remove_tree(const char *path);

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
    char *cwd = SDL_GetCurrentDirectory();
    snprintf(folder, sizeof folder, "%simage-test-%llu", cwd,
             (unsigned long long)SDL_GetPerformanceCounter());
    SDL_free(cwd);
    snprintf(root, sizeof root, "%s/project", folder);
    TEST_ASSERT_TRUE(SDL_CreateDirectory(root));
    char metadata[4096];
    snprintf(metadata, sizeof metadata, "%s/project.pengui.yaml", root);
    TEST_ASSERT_TRUE(SDL_SaveFile(metadata, "schema: pengui/project/v1\nname: Images\n", 39));
    snprintf(png, sizeof png, "%s/source.png", folder);
    snprintf(jpg, sizeof jpg, "%s/source.jpg", folder);
}

void tearDown(void) {
    editor_shell_destroy(&shell);
    editor_host_destroy(&host);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
    remove_tree(folder);
}

static void remove_tree(const char *path) {
    SDL_PathInfo info;
    if (!SDL_GetPathInfo(path, &info)) {
        return;
    }
    if (info.type == SDL_PATHTYPE_DIRECTORY) {
        int count;
        char **entries = SDL_GlobDirectory(path, "*", 0, &count);
        for (int i = 0; entries && i < count; i++) {
            char child[4096];
            snprintf(child, sizeof child, "%s/%s", path, entries[i]);
            remove_tree(child);
        }
        SDL_free(entries);
    }
    SDL_RemovePath(path);
}

static const unsigned char png_bytes[] = {
    137, 80,  78,  71,  13,  10,  26,  10,  0,   0,   0,   13,  73,  72,  68,  82,  0,   0,   0,
    64,  0,   0,   0,   48,  8,   6,   0,   0,   0,   161, 75,  124, 31,  0,   0,   0,   148, 73,
    68,  65,  84,  120, 156, 237, 210, 177, 13,  131, 80,  0,   196, 80,  126, 68,  207, 18,  41,
    178, 29,  13,  19,  48,  19,  21,  99,  100, 33,  24,  227, 33,  97,  47,  112, 150, 117, 227,
    183, 255, 175, 9,   114, 44,  155, 156, 159, 62,  116, 253, 1,   20,  64,  11,  104, 10,  160,
    5,   52,  5,   208, 2,   154, 2,   104, 1,   77,  1,   180, 128, 166, 0,   90,  64,  83,  0,
    45,  160, 41,  128, 22,  208, 20,  64,  11,  104, 10,  160, 5,   52,  5,   208, 2,   154, 89,
    11,  124, 215, 115, 200, 253, 215, 63,  160, 0,   90,  64,  83,  0,   45,  160, 41,  128, 22,
    208, 20,  64,  11,  104, 10,  160, 5,   52,  5,   208, 2,   154, 2,   104, 1,   77,  1,   180,
    128, 166, 0,   90,  64,  83,  0,   45,  160, 41,  128, 22,  208, 220, 167, 71,  5,   98,  76,
    151, 97,  182, 0,   0,   0,   0,   73,  69,  78,  68,  174, 66,  96,  130,
};

static const unsigned char jpeg_bytes[] = {
    255, 216, 255, 224, 0,   16,  74,  70,  73,  70,  0,   1,   1,   0,   0,   1,   0,   1,   0,
    0,   255, 219, 0,   67,  0,   8,   6,   6,   7,   6,   5,   8,   7,   7,   7,   9,   9,   8,
    10,  12,  20,  13,  12,  11,  11,  12,  25,  18,  19,  15,  20,  29,  26,  31,  30,  29,  26,
    28,  28,  32,  36,  46,  39,  32,  34,  44,  35,  28,  28,  40,  55,  41,  44,  48,  49,  52,
    52,  52,  31,  39,  57,  61,  56,  50,  60,  46,  51,  52,  50,  255, 219, 0,   67,  1,   9,
    9,   9,   12,  11,  12,  24,  13,  13,  24,  50,  33,  28,  33,  50,  50,  50,  50,  50,  50,
    50,  50,  50,  50,  50,  50,  50,  50,  50,  50,  50,  50,  50,  50,  50,  50,  50,  50,  50,
    50,  50,  50,  50,  50,  50,  50,  50,  50,  50,  50,  50,  50,  50,  50,  50,  50,  50,  50,
    50,  50,  50,  50,  50,  50,  255, 192, 0,   17,  8,   0,   48,  0,   64,  3,   1,   34,  0,
    2,   17,  1,   3,   17,  1,   255, 196, 0,   31,  0,   0,   1,   5,   1,   1,   1,   1,   1,
    1,   0,   0,   0,   0,   0,   0,   0,   0,   1,   2,   3,   4,   5,   6,   7,   8,   9,   10,
    11,  255, 196, 0,   181, 16,  0,   2,   1,   3,   3,   2,   4,   3,   5,   5,   4,   4,   0,
    0,   1,   125, 1,   2,   3,   0,   4,   17,  5,   18,  33,  49,  65,  6,   19,  81,  97,  7,
    34,  113, 20,  50,  129, 145, 161, 8,   35,  66,  177, 193, 21,  82,  209, 240, 36,  51,  98,
    114, 130, 9,   10,  22,  23,  24,  25,  26,  37,  38,  39,  40,  41,  42,  52,  53,  54,  55,
    56,  57,  58,  67,  68,  69,  70,  71,  72,  73,  74,  83,  84,  85,  86,  87,  88,  89,  90,
    99,  100, 101, 102, 103, 104, 105, 106, 115, 116, 117, 118, 119, 120, 121, 122, 131, 132, 133,
    134, 135, 136, 137, 138, 146, 147, 148, 149, 150, 151, 152, 153, 154, 162, 163, 164, 165, 166,
    167, 168, 169, 170, 178, 179, 180, 181, 182, 183, 184, 185, 186, 194, 195, 196, 197, 198, 199,
    200, 201, 202, 210, 211, 212, 213, 214, 215, 216, 217, 218, 225, 226, 227, 228, 229, 230, 231,
    232, 233, 234, 241, 242, 243, 244, 245, 246, 247, 248, 249, 250, 255, 196, 0,   31,  1,   0,
    3,   1,   1,   1,   1,   1,   1,   1,   1,   1,   0,   0,   0,   0,   0,   0,   1,   2,   3,
    4,   5,   6,   7,   8,   9,   10,  11,  255, 196, 0,   181, 17,  0,   2,   1,   2,   4,   4,
    3,   4,   7,   5,   4,   4,   0,   1,   2,   119, 0,   1,   2,   3,   17,  4,   5,   33,  49,
    6,   18,  65,  81,  7,   97,  113, 19,  34,  50,  129, 8,   20,  66,  145, 161, 177, 193, 9,
    35,  51,  82,  240, 21,  98,  114, 209, 10,  22,  36,  52,  225, 37,  241, 23,  24,  25,  26,
    38,  39,  40,  41,  42,  53,  54,  55,  56,  57,  58,  67,  68,  69,  70,  71,  72,  73,  74,
    83,  84,  85,  86,  87,  88,  89,  90,  99,  100, 101, 102, 103, 104, 105, 106, 115, 116, 117,
    118, 119, 120, 121, 122, 130, 131, 132, 133, 134, 135, 136, 137, 138, 146, 147, 148, 149, 150,
    151, 152, 153, 154, 162, 163, 164, 165, 166, 167, 168, 169, 170, 178, 179, 180, 181, 182, 183,
    184, 185, 186, 194, 195, 196, 197, 198, 199, 200, 201, 202, 210, 211, 212, 213, 214, 215, 216,
    217, 218, 226, 227, 228, 229, 230, 231, 232, 233, 234, 242, 243, 244, 245, 246, 247, 248, 249,
    250, 255, 218, 0,   12,  3,   1,   0,   2,   17,  3,   17,  0,   63,  0,   207, 162, 138, 43,
    236, 79,  145, 10,  40,  162, 128, 58,  250, 40,  162, 191, 34,  63,  85,  10,  40,  162, 128,
    57,  10,  40,  162, 191, 93,  63,  42,  10,  40,  162, 128, 58,  250, 43,  230, 10,  43,  243,
    143, 236, 191, 239, 254, 31,  240, 79,  208, 191, 180, 191, 187, 248, 255, 0,   192, 62,  159,
    162, 190, 96,  162, 143, 236, 191, 239, 254, 31,  240, 67,  251, 75,  251, 191, 143, 252, 3,
    220, 40,  162, 138, 253, 28,  252, 244, 40,  162, 138, 0,   240, 250, 40,  162, 190, 56,  250,
    224, 162, 138, 40,  3,   255, 217,
};

static void sources(void) {
    TEST_ASSERT_TRUE(SDL_SaveFile(png, png_bytes, sizeof png_bytes));
    TEST_ASSERT_TRUE(SDL_SaveFile(jpg, jpeg_bytes, sizeof jpeg_bytes));
}

static bool place(const char *path) {
    char error[256];
    return vector_image_import(host.instance, shell.view, 1, 1, path, root, error, sizeof error);
}

static void test_import_copies_png_and_jpeg_and_undo_preserves_assets(void) {
    sources();
    VectorDocument *d = document();
    int cursor = d->history.cursor;
    TEST_ASSERT_TRUE(place(png));
    TEST_ASSERT_EQUAL_INT(cursor + 1, d->history.cursor);
    TEST_ASSERT_EQUAL_UINT(64, d->rectangles[0].image.width);
    TEST_ASSERT_EQUAL_FLOAT(64, d->rectangles[0].rect.w);
    TEST_ASSERT_EQUAL_FLOAT(268, d->rectangles[0].rect.x);
    TEST_ASSERT_NOT_EQUAL(0, strcmp(png, d->rectangles[0].image.source));
    char asset[1024];
    snprintf(asset, sizeof asset, "%s", d->rectangles[0].image.source);
    TEST_ASSERT_TRUE(vector_history_undo(&d->history, d));
    TEST_ASSERT_EQUAL_INT(0, d->rectangle_count);
    SDL_PathInfo info;
    TEST_ASSERT_TRUE(SDL_GetPathInfo(asset, &info));
    TEST_ASSERT_TRUE(vector_history_redo(&d->history, d));
    TEST_ASSERT_TRUE(place(jpg));
    TEST_ASSERT_EQUAL_INT(2, d->rectangle_count);
    TEST_ASSERT_TRUE(vector_objects_duplicate(d));
    TEST_ASSERT_EQUAL_STRING(d->rectangles[1].image.source, d->rectangles[2].image.source);
    commit();
    TEST_ASSERT_EQUAL_STRING("Bild", pui_view_text(shell.view, "rectangle_kind"));
}

static void test_invalid_images_and_locked_artboard_do_not_change_document(void) {
    sources();
    VectorDocument *d = document();
    int cursor = d->history.cursor;
    TEST_ASSERT_TRUE(SDL_SaveFile(png, "invalid", 7));
    TEST_ASSERT_FALSE(place(png));
    TEST_ASSERT_EQUAL_INT(cursor, d->history.cursor);
    TEST_ASSERT_TRUE(vector_artboard_set_state(d, 1, VECTOR_OBJECT_LOCKED));
    TEST_ASSERT_FALSE(place(jpg));
    TEST_ASSERT_EQUAL_INT(0, d->rectangle_count);
    TEST_ASSERT_FALSE(vector_image_relative_valid("assets/images/../outside.png"));
    TEST_ASSERT_FALSE(vector_image_relative_valid("/tmp/image.png"));
    TEST_ASSERT_TRUE(vector_image_relative_valid("assets/images/image.png"));
}

static void test_project_relative_save_load_and_editor_switch_keep_image(void) {
    sources();
    TEST_ASSERT_TRUE(place(png));
    VectorDocument *d = document();
    char directory[4096], file[4096], error[256];
    snprintf(directory, sizeof directory, "%s/documents", root);
    TEST_ASSERT_TRUE(SDL_CreateDirectory(directory));
    snprintf(file, sizeof file, "%s/art.pengvec", directory);
    TEST_ASSERT_TRUE(vector_document_file_save(d, file, error, sizeof error));
    size_t length;
    char *bytes = SDL_LoadFile(file, &length);
    TEST_ASSERT_NOT_NULL(bytes);
    // An absolute machine-specific path must not leak into the persistent file.
    bool absolute = false;
    for (size_t i = 0; i + strlen(root) <= length; i++) {
        absolute |= !memcmp(bytes + i, root, strlen(root));
    }
    SDL_free(bytes);
    TEST_ASSERT_FALSE(absolute);
    TEST_ASSERT_TRUE(SDL_RemovePath(png)); // Project copy remains available.
    VectorDocument *loaded = calloc(1, sizeof *loaded);
    vector_document_init(loaded, 1);
    TEST_ASSERT_TRUE_MESSAGE(vector_document_file_load(loaded, file, error, sizeof error), error);
    SDL_Surface *image = editor_image_decode(loaded->rectangles[0].image.source);
    TEST_ASSERT_NOT_NULL(image);
    SDL_DestroySurface(image);
    vector_history_destroy(&loaded->history);
    vector_document_init(loaded, 1);
    char moved[4096], moved_file[4096];
    snprintf(moved, sizeof moved, "%s/moved", folder);
    TEST_ASSERT_TRUE(SDL_RenamePath(root, moved));
    snprintf(moved_file, sizeof moved_file, "%s/documents/art.pengvec", moved);
    TEST_ASSERT_TRUE_MESSAGE(vector_document_file_load(loaded, moved_file, error, sizeof error),
                             error);
    image = editor_image_decode(loaded->rectangles[0].image.source);
    TEST_ASSERT_NOT_NULL(image);
    SDL_DestroySurface(image);
    vector_history_destroy(&loaded->history);
    free(loaded);
    TEST_ASSERT_TRUE(SDL_RenamePath(moved, root));
    activate("scripts");
    activate("vector");
    d = document();
    TEST_ASSERT_EQUAL_INT(1, d->rectangle_count);
    TEST_ASSERT_TRUE(vector_history_undo(&d->history, d));
    TEST_ASSERT_TRUE(vector_history_redo(&d->history, d));
}

static void test_image_content_is_clipped_and_rendered_in_preview(void) {
    sources();
    TEST_ASSERT_TRUE(place(png));
    VectorDocument *d = document();
    TEST_ASSERT_TRUE(vector_rectangle_update(d, 1, "Picture", (PUIRect){80, 70, 160, 160}));
    TEST_ASSERT_TRUE(vector_ellipse_add(d, 1, (PUIRect){80, 70, 160, 160}));
    vector_document_select(d, 1);
    vector_document_toggle(d, 2);
    TEST_ASSERT_TRUE(vector_mask_create(d));
    d->preview.board = 1;
    d->preview.region = (PUIRect){60, 50, 200, 200};
    SDL_SetRenderClipRect(renderer, NULL);
    VectorEditor *editor = host.instance;
    vector_preview_draw(d, renderer, (PUIRect){0, 0, 208, 228}, editor->text_renderer,
                        editor->image_renderer);
    SDL_Surface *surface = SDL_RenderReadPixels(renderer, NULL);
    TEST_ASSERT_NOT_NULL(surface);
    Uint8 r, g, b, a;
    TEST_ASSERT_TRUE(SDL_ReadSurfacePixel(surface, 64, 124, &r, &g, &b, &a));
    TEST_ASSERT_EQUAL_UINT8(40, r);
    TEST_ASSERT_EQUAL_UINT8(130, g);
    TEST_ASSERT_EQUAL_UINT8(210, b);
    TEST_ASSERT_TRUE(SDL_ReadSurfacePixel(surface, 25, 45, &r, &g, &b, &a));
    TEST_ASSERT_EQUAL_UINT8(242, r);
    TEST_ASSERT_TRUE(SDL_ReadSurfacePixel(surface, 144, 164, &r, &g, &b, &a));
    TEST_ASSERT_EQUAL_UINT8(242, r); // PNG alpha.
    SDL_DestroySurface(surface);
}

static void test_queued_import_targets_original_document_and_rejects_project_changes(void) {
    sources();
    ProjectController *projects = calloc(1, sizeof *projects);
    TEST_ASSERT_NOT_NULL(projects);
    snprintf(projects->project.root, sizeof projects->project.root, "%s", root);
    DocumentController c;
    document_controller_init(&c);
    c.owner = host.active;
    c.document = 1;
    c.artboard = 1;
    snprintf(c.import_root, sizeof c.import_root, "%s", root);
    snprintf(c.pending_import, sizeof c.pending_import, "%s", png);
    TEST_ASSERT_TRUE(editor_host_action(&host, shell.view, "vector.add_document", ""));
    commit();
    TEST_ASSERT_EQUAL_UINT(2, document()->id);
    TEST_ASSERT_TRUE(
        document_controller_action(&c, &host, projects, window, shell.view, "editor.save", ""));
    TEST_ASSERT_FALSE(c.dialog_pending); // Pending import cannot be overwritten by another dialog.
    document_controller_commit(&c, &host, projects, shell.view, &shell.input);
    TEST_ASSERT_EQUAL_INT(0, document()->rectangle_count);
    VectorDocument *first = vector_documents_find(&((VectorEditor *)host.instance)->documents, 1);
    TEST_ASSERT_EQUAL_INT(1, first->rectangle_count);
    snprintf(c.pending_import, sizeof c.pending_import, "%s", jpg);
    snprintf(projects->project.root, sizeof projects->project.root, "%s", folder);
    document_controller_commit(&c, &host, projects, shell.view, &shell.input);
    TEST_ASSERT_EQUAL_INT(1, first->rectangle_count);
    document_controller_destroy(&c);
    free(projects);
}

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_queued_import_targets_original_document_and_rejects_project_changes);
    RUN_TEST(test_import_copies_png_and_jpeg_and_undo_preserves_assets);
    RUN_TEST(test_invalid_images_and_locked_artboard_do_not_change_document);
    RUN_TEST(test_project_relative_save_load_and_editor_switch_keep_image);
    RUN_TEST(test_image_content_is_clipped_and_rendered_in_preview);
    return UNITY_END();
}
