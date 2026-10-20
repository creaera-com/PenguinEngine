#include "editor_host/host.h"
#include "editors/vector/document/selection.h"
#include "editors/vector/ellipse/ellipse.h"
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
#include "editors/vector/repeat/render.h"
#include "editors/vector/repeat/repeat.h"
#include "editors/vector/text/text.h"
#include "editors/vector/ui/document_tree/groups.h"
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
    TEST_ASSERT_TRUE(vector_rectangle_add(d, 1, (PUIRect){20, 30, 40, 30}));
    vector_document_select(d, 1);
    commit();
}

void tearDown(void) {
    editor_shell_destroy(&shell);
    editor_host_destroy(&host);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
}

static void test_linked_twenty_cells_track_source_and_each_axis_steps(void) {
    VectorDocument *d = document();
    VectorRepeat r = vector_repeat_default();
    r.gap_x = 10;
    r.gap_y = 20;
    r.column.value[0] = 2;
    r.row.value[1] = 3;
    r.column.value[2] = 1;
    r.row.value[2] = 5;
    r.column.value[3] = .5f;
    r.column.value[4] = 20;
    r.row.value[7] = -5;
    int cursor = d->history.cursor;
    TEST_ASSERT_TRUE(vector_repeat_apply(d, &r, true));
    TEST_ASSERT_EQUAL_INT(1, d->rectangle_count);
    TEST_ASSERT_EQUAL_INT(cursor + 1, d->history.cursor);
    VectorShape out;
    TEST_ASSERT_TRUE(
        vector_repeat_instance(&d->rectangles[0], &d->rectangles[0].repeat, 4, 3, &out));
    TEST_ASSERT_EQUAL_FLOAT(220, out.rect.x);
    TEST_ASSERT_EQUAL_FLOAT(180, out.rect.y);
    TEST_ASSERT_EQUAL_FLOAT(48, out.rect.w);
    TEST_ASSERT_EQUAL_FLOAT(39, out.rect.h);
    TEST_ASSERT_EQUAL_FLOAT(19, out.angle);
    TEST_ASSERT_EQUAL_FLOAT(2, out.stroke.width);
    TEST_ASSERT_FALSE(out.repeat.enabled);
    TEST_ASSERT_TRUE(vector_rectangle_set_fill(d, 1, 0xff0000ff));
    TEST_ASSERT_TRUE(
        vector_repeat_instance(&d->rectangles[0], &d->rectangles[0].repeat, 1, 0, &out));
    TEST_ASSERT_UINT_WITHIN(1, 20, (unsigned)pui_color_to_hsl(out.fill).h);
    TEST_ASSERT_TRUE(vector_history_undo(&d->history, d));
    TEST_ASSERT_TRUE(vector_history_undo(&d->history, d));
    TEST_ASSERT_FALSE(d->rectangles[0].repeat.enabled);
    TEST_ASSERT_TRUE(vector_history_redo(&d->history, d));
    TEST_ASSERT_TRUE(d->rectangles[0].repeat.enabled);
}

static void test_independent_copies_and_detach_are_single_atomic_edits(void) {
    VectorDocument *d = document();
    VectorRepeat r = vector_repeat_default();
    TEST_ASSERT_TRUE(vector_repeat_apply(d, &r, true));
    int cursor = d->history.cursor;
    TEST_ASSERT_TRUE(vector_repeat_detach(d));
    TEST_ASSERT_EQUAL_INT(20, d->rectangle_count);
    TEST_ASSERT_EQUAL_INT(cursor + 1, d->history.cursor);
    unsigned ids[VECTOR_MAX_RECTANGLES];
    for (int i = 0; i < 20; i++) {
        ids[i] = d->rectangles[i].id;
        TEST_ASSERT_FALSE(d->rectangles[i].repeat.enabled);
    }
    uint32_t other = d->rectangles[1].fill;
    TEST_ASSERT_TRUE(vector_rectangle_set_fill(d, ids[0], 0x123456ff));
    TEST_ASSERT_EQUAL_UINT(other, d->rectangles[1].fill);
    TEST_ASSERT_TRUE(vector_history_undo(&d->history, d));
    TEST_ASSERT_TRUE(vector_history_undo(&d->history, d));
    TEST_ASSERT_EQUAL_INT(1, d->rectangle_count);
    TEST_ASSERT_TRUE(d->rectangles[0].repeat.enabled);
    TEST_ASSERT_TRUE(vector_history_redo(&d->history, d));
    TEST_ASSERT_EQUAL_INT(20, d->rectangle_count);
    for (int i = 0; i < 20; i++) {
        TEST_ASSERT_EQUAL_UINT(ids[i], d->rectangles[i].id);
    }
}

static void test_guards_limits_and_remove_leave_source_unchanged_on_failure(void) {
    VectorDocument *d = document();
    VectorRepeat r = vector_repeat_default();
    r.columns = 64;
    r.rows = 16;
    TEST_ASSERT_TRUE(vector_repeat_apply(d, &r, true));
    int cursor = d->history.cursor;
    TEST_ASSERT_FALSE(vector_repeat_detach(d));
    TEST_ASSERT_EQUAL_INT(cursor, d->history.cursor);
    r.rows = 17;
    TEST_ASSERT_FALSE(vector_repeat_apply(d, &r, true));
    r.rows = 4;
    r.column.value[2] = NAN;
    TEST_ASSERT_FALSE(vector_repeat_apply(d, &r, true));
    TEST_ASSERT_TRUE(vector_rectangle_set_state(d, 1, VECTOR_OBJECT_LOCKED));
    TEST_ASSERT_FALSE(vector_repeat_remove(d));
    TEST_ASSERT_TRUE(vector_history_undo(&d->history, d));
    TEST_ASSERT_TRUE(vector_repeat_remove(d));
    TEST_ASSERT_FALSE(d->rectangles[0].repeat.enabled);
    TEST_ASSERT_TRUE(vector_history_undo(&d->history, d));
    TEST_ASSERT_TRUE(d->rectangles[0].repeat.enabled);
}

static void test_virtual_picking_and_rendering_select_source_and_track_fill(void) {
    VectorDocument *d = document();
    VectorRepeat r = vector_repeat_default();
    r.columns = 3;
    r.rows = 2;
    TEST_ASSERT_TRUE(vector_repeat_apply(d, &r, true));
    TEST_ASSERT_TRUE(vector_shape_contains(&d->rectangles[0], 120, 80));
    TEST_ASSERT_FALSE(vector_shape_contains(&d->rectangles[0], 64, 40));
    TEST_ASSERT_TRUE(vector_shape_intersects(&d->rectangles[0], (PUIRect){119, 79, 2, 2}));
    TEST_ASSERT_TRUE(vector_rectangle_set_fill(d, 1, 0xe05030ff));
    d->preview.board = 1;
    d->preview.region = (PUIRect){0, 0, 200, 140};
    VectorEditor *editor = host.instance;
    SDL_SetRenderClipRect(renderer, NULL);
    vector_preview_draw(d, renderer, (PUIRect){0, 0, 208, 168}, editor->text_renderer,
                        editor->image_renderer);
    SDL_Surface *pixels = SDL_RenderReadPixels(renderer, NULL);
    TEST_ASSERT_NOT_NULL(pixels);
    Uint8 red, green, blue, alpha;
    TEST_ASSERT_TRUE(SDL_ReadSurfacePixel(pixels, 124, 104, &red, &green, &blue, &alpha));
    TEST_ASSERT_EQUAL_UINT8(0xe0, red);
    TEST_ASSERT_EQUAL_UINT8(0x50, green);
    SDL_DestroySurface(pixels);
}

static void test_file_roundtrip_checkpoints_and_clipboard_keep_repetition(void) {
    VectorDocument *d = document();
    VectorRepeat r = vector_repeat_default();
    r.row.value[2] = 2;
    TEST_ASSERT_TRUE(vector_repeat_apply(d, &r, true));
    VectorClipboard clipboard;
    TEST_ASSERT_TRUE(vector_objects_copy(d, &clipboard));
    TEST_ASSERT_TRUE(vector_objects_duplicate(d));
    TEST_ASSERT_EQUAL_INT(2, d->rectangle_count);
    TEST_ASSERT_TRUE(d->rectangles[1].repeat.enabled);
    TEST_ASSERT_TRUE(vector_history_undo(&d->history, d));
    char file[4096], error[256];
    char *cwd = SDL_GetCurrentDirectory();
    snprintf(file, sizeof file, "%srepeat-%llu.pengvec", cwd,
             (unsigned long long)SDL_GetPerformanceCounter());
    SDL_free(cwd);
    TEST_ASSERT_TRUE(vector_document_file_save(d, file, error, sizeof error));
    VectorDocument *loaded = calloc(1, sizeof *loaded);
    vector_document_init(loaded, 1);
    TEST_ASSERT_TRUE_MESSAGE(vector_document_file_load(loaded, file, error, sizeof error), error);
    TEST_ASSERT_EQUAL_INT(1, loaded->rectangle_count);
    TEST_ASSERT_EQUAL_MEMORY(&r, &loaded->rectangles[0].repeat, sizeof r);
    vector_history_destroy(&loaded->history);
    free(loaded);
    SDL_RemovePath(file);
    activate("scripts");
    activate("vector");
    d = document();
    TEST_ASSERT_EQUAL_MEMORY(&r, &d->rectangles[0].repeat, sizeof r);
    TEST_ASSERT_TRUE(vector_history_undo(&d->history, d));
    TEST_ASSERT_FALSE(d->rectangles[0].repeat.enabled);
}

static void field(const char *id, const char *value, const char *action) {
    TEST_ASSERT_TRUE(pui_view_set_text(shell.view, id, value));
    TEST_ASSERT_TRUE(editor_host_action(&host, shell.view, action, id));
    commit();
}

static void test_yaml_drafts_commit_only_on_apply_and_ui_reports_twenty(void) {
    VectorDocument *d = document();
    int cursor = d->history.cursor;
    field("repeat_columns", "5", "vector.repeat.columns");
    field("repeat_rows", "4", "vector.repeat.rows");
    TEST_ASSERT_EQUAL_STRING("20 Exemplare inkl. Original",
                             pui_view_text(shell.view, "repeat_count"));
    TEST_ASSERT_TRUE(pui_view_set_value(shell.view, "repeat_property", "angle"));
    TEST_ASSERT_TRUE(editor_host_action(&host, shell.view, "vector.repeat.property", ""));
    field("repeat_column_step", "1", "vector.repeat.column_step");
    field("repeat_row_step", "5", "vector.repeat.row_step");
    TEST_ASSERT_EQUAL_INT(cursor, d->history.cursor);
    TEST_ASSERT_TRUE(editor_host_action(&host, shell.view, "vector.repeat.apply", ""));
    commit();
    TEST_ASSERT_EQUAL_INT(cursor + 1, d->history.cursor);
    TEST_ASSERT_TRUE(d->rectangles[0].repeat.enabled);
    TEST_ASSERT_EQUAL_FLOAT(1, d->rectangles[0].repeat.column.value[2]);
    TEST_ASSERT_EQUAL_FLOAT(5, d->rectangles[0].repeat.row.value[2]);
}

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_linked_twenty_cells_track_source_and_each_axis_steps);
    RUN_TEST(test_independent_copies_and_detach_are_single_atomic_edits);
    RUN_TEST(test_guards_limits_and_remove_leave_source_unchanged_on_failure);
    RUN_TEST(test_virtual_picking_and_rendering_select_source_and_track_fill);
    RUN_TEST(test_file_roundtrip_checkpoints_and_clipboard_keep_repetition);
    RUN_TEST(test_yaml_drafts_commit_only_on_apply_and_ui_reports_twenty);
    return UNITY_END();
}
