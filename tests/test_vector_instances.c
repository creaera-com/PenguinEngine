#include "editor_host/host.h"
#include "editors/vector/geometry/constraints.h"
#include "editors/vector/geometry/resize.h"
#include "editors/vector/geometry/rotation.h"
#include "editors/vector/grid/render.h"
#include "editors/vector/grid/snap.h"
#include "editors/vector/internal.h"
#include "shell/shell.h"
#include "unity.h"

static SDL_Window *window;
static SDL_Renderer *renderer;
static EditorShell shell;
static EditorHost host;

static void commit(void) {
    char error[256];
    TEST_ASSERT_TRUE_MESSAGE(
        editor_host_commit(&host, shell.view, &shell.input, error, sizeof error), error);
    pui_view_draw(shell.view, &shell.input, (PUIRect){0, 0, 1280, 800}, NULL, NULL);
}

static VectorDocument *document(void) {
    VectorEditor *editor = host.instance;
    return vector_document_tabs_active(&editor->documents, shell.view);
}

static void action(const char *name) {
    TEST_ASSERT_TRUE(editor_host_action(&host, shell.view, name, ""));
    commit();
}

static void activate(const char *id) {
    TEST_ASSERT_TRUE(editor_host_request(&host, id, false));
    commit();
}

void setUp(void) {
    TEST_ASSERT_TRUE(SDL_Init(SDL_INIT_VIDEO));
    window = SDL_CreateWindow("grid", 1280, 800, SDL_WINDOW_HIDDEN);
    renderer = SDL_CreateRenderer(window, NULL);
    TEST_ASSERT_NOT_NULL(renderer);
    char path[1600], error[256];
    snprintf(path, sizeof path, "%s/shell.yaml", PENGUI_EDITOR_RESOURCES);
    TEST_ASSERT_TRUE_MESSAGE(editor_shell_load(&shell, renderer, path, error, sizeof error), error);
    editor_host_init(&host, (EditorServices){PENGUI_EDITOR_RESOURCES});
    activate("vector");
    commit();
    /* These feature fixtures build their own scene independently of startup content. */
    vector_history_destroy(&document()->history);
    vector_document_init(document(), document()->id);
}

void tearDown(void) {
    editor_shell_destroy(&shell);
    editor_host_destroy(&host);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
}

#include "editors/vector/instance/instance.h"
#include "editors/vector/mask/mask.h"
#include "editors/vector/object/clipboard.h"
#include "editors/vector/persistence/document_file.h"
#include "editors/vector/rectangle/transform.h"
#include <stdlib.h>
#include <string.h>

static void source(void) {
    TEST_ASSERT_TRUE(vector_artboard_add(document(), (PUIRect){0, 0, 800, 600}));
    TEST_ASSERT_TRUE(vector_rectangle_add(document(), 1, (PUIRect){40, 50, 100, 60}));
    commit();
}

static void test_linked_instance_inherits_appearance_but_keeps_transform_and_history(void) {
    source();
    VectorDocument *d = document();
    action("vector.objects.clone_linked");
    TEST_ASSERT_EQUAL_INT(2, d->rectangle_count);
    TEST_ASSERT_EQUAL_UINT(1, d->rectangles[1].instance_source);
    unsigned id = d->active_rectangle;
    TEST_ASSERT_TRUE(vector_rectangles_translate(d, &id, 1, 120, 30));
    TEST_ASSERT_TRUE(vector_rectangle_set_angle(d, id, 35));
    TEST_ASSERT_TRUE(vector_rectangle_set_fill(d, 1, 0xff5533ff));
    TEST_ASSERT_TRUE(vector_rectangle_update(d, 1, "Original", (PUIRect){40, 50, 200, 90}));
    VectorShape resolved;
    vector_instance_resolve(d, &d->rectangles[1], &resolved);
    TEST_ASSERT_EQUAL_HEX32(0xff5533ff, resolved.fill);
    TEST_ASSERT_EQUAL_FLOAT(176, resolved.rect.x);
    TEST_ASSERT_EQUAL_FLOAT(96, resolved.rect.y);
    TEST_ASSERT_EQUAL_FLOAT(200, resolved.rect.w);
    TEST_ASSERT_EQUAL_FLOAT(35, resolved.angle);
    TEST_ASSERT_FALSE(vector_rectangle_set_fill(d, id, 0xffffffff));
    TEST_ASSERT_TRUE(vector_history_undo(&d->history, d));
    TEST_ASSERT_EQUAL_FLOAT(100, vector_instance_rect(d, &d->rectangles[1]).w);
    TEST_ASSERT_TRUE(vector_history_redo(&d->history, d));
    TEST_ASSERT_EQUAL_FLOAT(200, vector_instance_rect(d, &d->rectangles[1]).w);
    PUIRect resized = vector_instance_rect(d, &d->rectangles[1]);
    resized.w = 300;
    TEST_ASSERT_TRUE(vector_rectangles_set_rects(d, &id, &resized, 1));
    TEST_ASSERT_EQUAL_FLOAT(300, vector_instance_rect(d, &d->rectangles[1]).w);
    TEST_ASSERT_TRUE(vector_history_undo(&d->history, d));
    TEST_ASSERT_EQUAL_FLOAT(200, vector_instance_rect(d, &d->rectangles[1]).w);
}

static void test_multiple_selection_is_one_command_and_detach_is_reversible(void) {
    source();
    VectorDocument *d = document();
    TEST_ASSERT_TRUE(vector_rectangle_add(d, 1, (PUIRect){200, 50, 80, 80}));
    vector_document_select(d, 1);
    vector_document_toggle(d, 2);
    int cursor = d->history.cursor;
    TEST_ASSERT_TRUE(vector_instances_create(d));
    TEST_ASSERT_EQUAL_INT(cursor + 1, d->history.cursor);
    TEST_ASSERT_EQUAL_INT(4, d->rectangle_count);
    TEST_ASSERT_EQUAL_INT(2, d->selection.count);
    TEST_ASSERT_TRUE(vector_instances_detach(d));
    TEST_ASSERT_EQUAL_UINT(0, d->rectangles[2].instance_source);
    TEST_ASSERT_TRUE(vector_history_undo(&d->history, d));
    TEST_ASSERT_EQUAL_UINT(1, d->rectangles[2].instance_source);
    TEST_ASSERT_TRUE(vector_history_undo(&d->history, d));
    TEST_ASSERT_EQUAL_INT(2, d->rectangle_count);
    TEST_ASSERT_TRUE(vector_history_redo(&d->history, d));
    TEST_ASSERT_EQUAL_INT(4, d->rectangle_count);
}

static void test_save_load_and_editor_unload_preserve_links(void) {
    source();
    VectorDocument *d = document();
    TEST_ASSERT_TRUE(vector_instances_create(d));
    TEST_ASSERT_TRUE(vector_rectangle_set_fill(d, 1, 0x2288ffff));
    char error[256];
    TEST_ASSERT_TRUE_MESSAGE(
        vector_document_file_save(d, "linked-instance-test.pengvec", error, sizeof error), error);
    VectorDocument *loaded = calloc(1, sizeof *loaded);
    TEST_ASSERT_NOT_NULL(loaded);
    vector_document_init(loaded, 99);
    TEST_ASSERT_TRUE_MESSAGE(
        vector_document_file_load(loaded, "linked-instance-test.pengvec", error, sizeof error),
        error);
    TEST_ASSERT_EQUAL_UINT(1, loaded->rectangles[1].instance_source);
    TEST_ASSERT_TRUE(vector_rectangle_set_fill(loaded, 1, 0x44aa22ff));
    VectorShape resolved;
    vector_instance_resolve(loaded, &loaded->rectangles[1], &resolved);
    TEST_ASSERT_EQUAL_HEX32(0x44aa22ff, resolved.fill);
    vector_history_destroy(&loaded->history);
    free(loaded);
    SDL_RemovePath("linked-instance-test.pengvec");
    activate("scripts");
    activate("vector");
    d = document();
    TEST_ASSERT_NOT_NULL(d);
    TEST_ASSERT_EQUAL_INT(2, d->rectangle_count);
    TEST_ASSERT_EQUAL_UINT(1, d->rectangles[1].instance_source);
    TEST_ASSERT_TRUE(vector_history_undo(&d->history, d));
}

static void test_deleting_source_bakes_instances_and_undo_restores_link(void) {
    source();
    VectorDocument *d = document();
    TEST_ASSERT_TRUE(vector_instances_create(d));
    TEST_ASSERT_TRUE(vector_rectangle_set_fill(d, 1, 0xaabbccff));
    vector_document_select(d, 1);
    TEST_ASSERT_TRUE(vector_objects_delete(d));
    TEST_ASSERT_EQUAL_INT(1, d->rectangle_count);
    TEST_ASSERT_EQUAL_UINT(0, d->rectangles[0].instance_source);
    TEST_ASSERT_EQUAL_HEX32(0xaabbccff, d->rectangles[0].fill);
    TEST_ASSERT_TRUE(vector_history_undo(&d->history, d));
    TEST_ASSERT_EQUAL_UINT(1, d->rectangles[1].instance_source);
}

static void test_copy_to_another_document_bakes_external_links_and_rejects_cycles(void) {
    source();
    VectorDocument *d = document();
    TEST_ASSERT_TRUE(vector_instances_create(d));
    TEST_ASSERT_TRUE(vector_rectangle_set_fill(d, 1, 0x6611aaff));
    VectorClipboard *copy = calloc(1, sizeof *copy);
    TEST_ASSERT_NOT_NULL(copy);
    TEST_ASSERT_TRUE(vector_objects_copy(d, copy));
    VectorDocument *other = calloc(1, sizeof *other);
    TEST_ASSERT_NOT_NULL(other);
    vector_document_init(other, 99);
    TEST_ASSERT_TRUE(vector_objects_paste(other, copy));
    TEST_ASSERT_EQUAL_UINT(0, other->rectangles[0].instance_source);
    TEST_ASSERT_EQUAL_HEX32(0x6611aaff, other->rectangles[0].fill);
    VectorShape bad[2] = {d->rectangles[0], d->rectangles[1]};
    bad[0].instance_source = bad[1].id;
    TEST_ASSERT_FALSE(vector_instances_valid(bad, 2));
    vector_history_destroy(&other->history);
    free(other);
    free(copy);
}

static void test_render_and_hit_testing_use_current_source_geometry_and_fill(void) {
    source();
    VectorDocument *d = document();
    TEST_ASSERT_TRUE(vector_instances_create(d));
    unsigned id = d->active_rectangle;
    TEST_ASSERT_TRUE(vector_rectangles_translate(d, &id, 1, 200, 0));
    TEST_ASSERT_TRUE(vector_rectangle_set_fill(d, 1, 0xe04020ff));
    TEST_ASSERT_TRUE(vector_rectangle_update(d, 1, "Original", (PUIRect){40, 50, 180, 90}));
    VectorEditor *editor = host.instance;
    vector_artwork_draw(&editor->artwork, d, &d->artboards[0], renderer,
                        (SDL_FRect){0, 0, 800, 600}, editor->text_renderer, editor->image_renderer);
    SDL_Surface *pixels = SDL_RenderReadPixels(renderer, NULL);
    TEST_ASSERT_NOT_NULL(pixels);
    Uint8 r, g, b, a;
    TEST_ASSERT_TRUE(SDL_ReadSurfacePixel(pixels, 380, 90, &r, &g, &b, &a));
    TEST_ASSERT_EQUAL_UINT8(224, r);
    TEST_ASSERT_EQUAL_UINT8(64, g);
    TEST_ASSERT_EQUAL_UINT8(32, b);
    SDL_DestroySurface(pixels);
    TEST_ASSERT_TRUE(vector_mask_contains(d, &d->rectangles[1], 380, 90));
    TEST_ASSERT_FALSE(vector_mask_contains(d, &d->rectangles[1], 450, 90));
}

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_render_and_hit_testing_use_current_source_geometry_and_fill);
    RUN_TEST(test_copy_to_another_document_bakes_external_links_and_rejects_cycles);
    RUN_TEST(test_linked_instance_inherits_appearance_but_keeps_transform_and_history);
    RUN_TEST(test_multiple_selection_is_one_command_and_detach_is_reversible);
    RUN_TEST(test_save_load_and_editor_unload_preserve_links);
    RUN_TEST(test_deleting_source_bakes_instances_and_undo_restores_link);
    return UNITY_END();
}
