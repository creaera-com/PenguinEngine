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
#include "editors/vector/rectangle/render.h"
#include "editors/vector/rectangle/transform.h"
#include "editors/vector/text/text.h"
#include "editors/vector/ui/document_tree/groups.h"
#include "editors/vector/workspace/selection.h"
#include "shell/shell.h"
#include "unity.h"
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
    editor_shell_destroy(&shell);
    editor_host_destroy(&host);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
}

static void filename(char *out, size_t capacity) {
    char *cwd = SDL_GetCurrentDirectory();
    TEST_ASSERT_NOT_NULL(cwd);
    snprintf(out, capacity, "%smask-%llu.pengvec", cwd,
             (unsigned long long)SDL_GetPerformanceCounter());
    SDL_free(cwd);
}

static void prepare(void) {
    VectorDocument *d = document();
    TEST_ASSERT_TRUE(vector_rectangle_add(d, 1, (PUIRect){40, 50, 240, 220}));
    TEST_ASSERT_TRUE(vector_rectangle_set_fill(d, 1, 0xe05030ff));
    TEST_ASSERT_TRUE(vector_ellipse_add(d, 1, (PUIRect){80, 70, 160, 160}));
    vector_document_select(d, 1);
    vector_document_toggle(d, 2);
}

static void test_create_release_and_history_preserve_both_original_objects(void) {
    prepare();
    VectorDocument *d = document();
    VectorShape before[2];
    memcpy(before, d->rectangles, sizeof before);
    int cursor = d->history.cursor;
    TEST_ASSERT_TRUE(vector_mask_can_create(d));
    TEST_ASSERT_TRUE(vector_mask_create(d));
    TEST_ASSERT_EQUAL_INT(cursor + 1, d->history.cursor);
    TEST_ASSERT_EQUAL_INT(2, d->rectangle_count);
    TEST_ASSERT_TRUE(d->rectangles[1].clip_mask);
    TEST_ASSERT_EQUAL_MEMORY(&before[1].curve, &d->rectangles[1].curve, sizeof before[1].curve);
    TEST_ASSERT_EQUAL_UINT(before[1].fill, d->rectangles[1].fill);
    TEST_ASSERT_NOT_NULL(vector_mask_find(d, d->rectangles[0].group_id));
    TEST_ASSERT_FALSE(vector_objects_can_ungroup(d));
    TEST_ASSERT_FALSE(vector_paths_can_operate(d, VECTOR_PATH_UNION));
    TEST_ASSERT_TRUE(vector_history_undo(&d->history, d));
    TEST_ASSERT_EQUAL_MEMORY(before, d->rectangles, sizeof before);
    TEST_ASSERT_TRUE(vector_history_redo(&d->history, d));
    TEST_ASSERT_TRUE(vector_mask_release(d));
    TEST_ASSERT_FALSE(d->rectangles[1].clip_mask);
    TEST_ASSERT_TRUE(vector_history_undo(&d->history, d));
    TEST_ASSERT_TRUE(d->rectangles[1].clip_mask);
}

static void test_group_content_and_independent_mask_selection_transform_and_copy(void) {
    prepare();
    VectorDocument *d = document();
    TEST_ASSERT_TRUE(vector_mask_create(d));
    vector_document_select(d, 1);
    TEST_ASSERT_EQUAL_INT(2, d->selection.count);
    unsigned ids[] = {1, 2};
    TEST_ASSERT_TRUE(vector_rectangles_translate(d, ids, 2, 10, 20));
    TEST_ASSERT_EQUAL_FLOAT(50, d->rectangles[0].rect.x);
    TEST_ASSERT_EQUAL_FLOAT(90, d->rectangles[1].rect.x);
    TEST_ASSERT_TRUE(vector_objects_duplicate(d));
    TEST_ASSERT_EQUAL_INT(4, d->rectangle_count);
    TEST_ASSERT_TRUE(d->rectangles[3].clip_mask);
    TEST_ASSERT_TRUE(d->rectangles[2].group_id != d->rectangles[0].group_id);
    TEST_ASSERT_TRUE(vector_objects_delete(d));
    TEST_ASSERT_EQUAL_INT(2, d->rectangle_count);
    vector_document_select(d, 2);
    vector_document_selection_sync(d);
    TEST_ASSERT_EQUAL_INT(1, d->selection.count);
    TEST_ASSERT_FALSE(vector_mask_can_release(d));
    VectorClipboard copy;
    TEST_ASSERT_TRUE(vector_objects_copy(d, &copy));
    TEST_ASSERT_EQUAL_INT(1, copy.rectangle_count);
    TEST_ASSERT_FALSE(copy.rectangles[0].clip_mask);
    TEST_ASSERT_TRUE(vector_objects_delete(d)); // Removing only the mask reveals the content.
    TEST_ASSERT_NULL(vector_mask_find(d, d->rectangles[0].group_id));
    TEST_ASSERT_TRUE(vector_history_undo(&d->history, d));
    TEST_ASSERT_TRUE(d->rectangles[1].clip_mask);
}

static void
test_existing_group_can_be_clipped_but_nested_masks_and_locked_edits_are_rejected(void) {
    prepare();
    VectorDocument *d = document();
    TEST_ASSERT_TRUE(vector_objects_group(d));
    TEST_ASSERT_TRUE(vector_rectangle_add(d, 1, (PUIRect){100, 100, 120, 120}));
    vector_document_select(d, 3);
    vector_document_toggle(d, 1);
    TEST_ASSERT_TRUE(vector_mask_create(d));
    TEST_ASSERT_EQUAL_INT(3, d->selection.count);
    TEST_ASSERT_TRUE(d->rectangles[2].clip_mask);
    TEST_ASSERT_TRUE(vector_rectangle_set_state(d, 1, VECTOR_OBJECT_LOCKED));
    TEST_ASSERT_FALSE(vector_mask_can_release(d));
    TEST_ASSERT_TRUE(vector_history_undo(&d->history, d));
    TEST_ASSERT_TRUE(vector_rectangle_add(d, 1, (PUIRect){0, 0, 100, 100}));
    vector_document_select(d, 4);
    vector_document_toggle(d, 1);
    TEST_ASSERT_FALSE(vector_mask_can_create(d));
    TEST_ASSERT_TRUE(vector_objects_delete(d));
    TEST_ASSERT_EQUAL_INT(0, d->rectangle_count);
}

static void test_mask_picking_rejects_invisible_corners_and_respects_holes(void) {
    prepare();
    VectorDocument *d = document();
    TEST_ASSERT_TRUE(vector_mask_create(d));
    TEST_ASSERT_TRUE(vector_mask_contains(d, &d->rectangles[0], 160, 150));
    TEST_ASSERT_FALSE(vector_mask_contains(d, &d->rectangles[0], 81, 71));
    TEST_ASSERT_FALSE(vector_mask_contains(d, &d->rectangles[1], 160, 150));
    TEST_ASSERT_FALSE(vector_mask_intersects(d, &d->rectangles[0], (PUIRect){40, 50, 10, 10}));
    TEST_ASSERT_TRUE(vector_mask_intersects(d, &d->rectangles[0], (PUIRect){150, 140, 20, 20}));
    VectorShape *m = &d->rectangles[1];
    m->curve = (VectorBezier){0};
    m->curve_kind = 0;
    m->path = (VectorPath){.count = 8,
                           .contours = 2,
                           .points = {{0, 0},
                                      {1, 0},
                                      {1, 1},
                                      {0, 1},
                                      {.25f, .25f},
                                      {.25f, .75f},
                                      {.75f, .75f},
                                      {.75f, .25f}},
                           .ends = {4, 8}};
    TEST_ASSERT_FALSE(vector_mask_contains(d, &d->rectangles[0], 160, 150));
    TEST_ASSERT_TRUE(vector_mask_contains(d, &d->rectangles[0], 90, 150));
}

static Uint8 pixel_red(int x, int y) {
    SDL_Surface *surface = SDL_RenderReadPixels(renderer, &(SDL_Rect){x, y, 1, 1});
    TEST_ASSERT_NOT_NULL(surface);
    Uint8 r, g, b, a;
    TEST_ASSERT_TRUE(SDL_ReadSurfacePixel(surface, 0, 0, &r, &g, &b, &a));
    SDL_DestroySurface(surface);
    return r;
}

static void paint(void) {
    VectorDocument *d = document();
    d->viewport.zoom = 1;
    vector_document_select(d, 0);
    SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
    SDL_RenderClear(renderer);
    vector_rectangles_draw(d, &d->artboards[0], renderer, (SDL_FRect){0, 0, 600, 400},
                           ((VectorEditor *)host.instance)->text_renderer,
                           ((VectorEditor *)host.instance)->image_renderer);
}

static void test_render_clips_curves_strokes_and_alpha_without_painting_the_mask(void) {
    prepare();
    VectorDocument *d = document();
    TEST_ASSERT_TRUE(vector_mask_create(d));
    TEST_ASSERT_TRUE(vector_rectangle_set_fill(d, 1, 0x80402080));
    TEST_ASSERT_TRUE(vector_rectangle_set_fill(d, 2, 0x00ff0000)); // Mask style has no influence.
    paint();
    TEST_ASSERT_INT_WITHIN(2, 191, pixel_red(160, 150)); // Premultiplied content over white.
    TEST_ASSERT_EQUAL_UINT8(255, pixel_red(81, 71));
    TEST_ASSERT_EQUAL_UINT8(255, pixel_red(50, 60));
    TEST_ASSERT_NULL(SDL_GetRenderTarget(renderer));
    TEST_ASSERT_FALSE(SDL_RenderClipEnabled(renderer));
    VectorShape hole = d->rectangles[1];
    hole.curve = (VectorBezier){0};
    hole.curve_kind = 0;
    hole.path = (VectorPath){.count = 8,
                             .contours = 2,
                             .points = {{0, 0},
                                        {1, 0},
                                        {1, 1},
                                        {0, 1},
                                        {.25f, .25f},
                                        {.25f, .75f},
                                        {.75f, .75f},
                                        {.75f, .25f}},
                             .ends = {4, 8}};
    TEST_ASSERT_TRUE(vector_shape_update(d, &hole));
    TEST_ASSERT_TRUE(vector_rectangle_set_angle(d, 2, 45));
    paint();
    TEST_ASSERT_EQUAL_UINT8(255, pixel_red(160, 150));
    TEST_ASSERT_INT_WITHIN(2, 191, pixel_red(90, 150));
    TEST_ASSERT_EQUAL_UINT8(255, pixel_red(81, 71));
    TEST_ASSERT_TRUE(vector_rectangle_set_state(d, 2, VECTOR_OBJECT_HIDDEN));
    paint();
    TEST_ASSERT_EQUAL_UINT8(255, pixel_red(160, 150));
}

static void test_file_roundtrip_and_editor_switch_preserve_editable_clipping_groups(void) {
    prepare();
    VectorDocument *d = document();
    TEST_ASSERT_TRUE(vector_mask_create(d));
    char file[4096], error[256];
    filename(file, sizeof file);
    TEST_ASSERT_TRUE_MESSAGE(vector_document_file_save(d, file, error, sizeof error), error);
    VectorDocument *loaded = calloc(1, sizeof *loaded);
    TEST_ASSERT_NOT_NULL(loaded);
    vector_document_init(loaded, 1);
    TEST_ASSERT_TRUE_MESSAGE(vector_document_file_load(loaded, file, error, sizeof error), error);
    TEST_ASSERT_TRUE(loaded->rectangles[1].clip_mask);
    TEST_ASSERT_EQUAL_MEMORY(&d->rectangles[1].curve, &loaded->rectangles[1].curve,
                             sizeof d->rectangles[1].curve);
    vector_history_destroy(&loaded->history);
    free(loaded);
    SDL_RemovePath(file);
    activate("scripts");
    activate("vector");
    d = document();
    TEST_ASSERT_EQUAL_INT(2, d->rectangle_count);
    TEST_ASSERT_TRUE(d->rectangles[1].clip_mask);
    TEST_ASSERT_TRUE(vector_history_undo(&d->history, d));
    TEST_ASSERT_FALSE(d->rectangles[1].clip_mask);
}

static void test_reader_accepts_v1_and_rejects_malformed_mask_relations(void) {
    VectorDocument *d = document();
    TEST_ASSERT_TRUE(vector_rectangle_add(d, 1, (PUIRect){10, 20, 80, 90}));
    char path[4096], error[256];
    filename(path, sizeof path);
    TEST_ASSERT_TRUE(vector_document_file_save(d, path, error, sizeof error));
    size_t length;
    unsigned char *bytes = SDL_LoadFile(path, &length);
    TEST_ASSERT_NOT_NULL(bytes);
    /* One artboard and one shape: remove mask role, image/repeat flags and v5/v6 fill fields and v7
     * corner radius for v1.
     */
    unsigned name_length = bytes[28] | ((unsigned)bytes[29] << 8);
    size_t mask_offset = 64 + name_length;
    TEST_ASSERT_TRUE(mask_offset + 4 < length);
    memmove(bytes + mask_offset, bytes + mask_offset + 4, length - mask_offset - 4);
    bytes[8] = 1;
    TEST_ASSERT_TRUE(SDL_SaveFile(path, bytes, length - 76));
    SDL_free(bytes);
    VectorDocument *loaded = calloc(1, sizeof *loaded);
    TEST_ASSERT_NOT_NULL(loaded);
    vector_document_init(loaded, 1);
    TEST_ASSERT_TRUE_MESSAGE(vector_document_file_load(loaded, path, error, sizeof error), error);
    TEST_ASSERT_EQUAL_INT(1, loaded->rectangle_count);
    TEST_ASSERT_FALSE(loaded->rectangles[0].clip_mask);
    TEST_ASSERT_EQUAL_FLOAT(80, loaded->rectangles[0].rect.w);
    vector_history_destroy(&loaded->history);
    free(loaded);
    SDL_RemovePath(path);

    TEST_ASSERT_TRUE(vector_ellipse_add(d, 1, (PUIRect){10, 20, 50, 50}));
    vector_document_select(d, 1);
    vector_document_toggle(d, 2);
    TEST_ASSERT_TRUE(vector_mask_create(d));
    VectorClipboard copy;
    TEST_ASSERT_TRUE(vector_objects_copy(d, &copy));
    TEST_ASSERT_TRUE(vector_clipboard_valid(&copy));
    copy.rectangles[0].clip_mask = 1;
    TEST_ASSERT_FALSE(vector_clipboard_valid(&copy)); // Two masks in one group.
    copy.rectangles[0].clip_mask = 0;
    copy.rectangles[1].group_id = 0;
    TEST_ASSERT_FALSE(vector_clipboard_valid(&copy)); // Orphan mask.
}

static void test_masked_group_renders_text_and_strokes_inside_the_same_boundary(void) {
    VectorDocument *d = document();
    TEST_ASSERT_TRUE(vector_rectangle_add(d, 1, (PUIRect){130, 80, 60, 150}));
    TEST_ASSERT_TRUE(
        vector_rectangle_set_stroke(d, 1, (VectorStroke){0x4560a0ff, 90, VECTOR_STROKE_OUTSIDE}));
    TEST_ASSERT_TRUE(vector_text_add(d, 1, (PUIRect){50, 90, 300, 110}));
    VectorText text = d->rectangles[1].text;
    snprintf(text.content, sizeof text.content, "MMMMMMMM");
    text.size = 64;
    TEST_ASSERT_TRUE(vector_text_update(d, 2, &text));
    TEST_ASSERT_TRUE(vector_ellipse_add(d, 1, (PUIRect){80, 70, 160, 160}));
    vector_document_select(d, 1);
    vector_document_toggle(d, 2);
    vector_document_toggle(d, 3);
    TEST_ASSERT_TRUE(vector_mask_create(d));
    paint();
    TEST_ASSERT_EQUAL_UINT8(255, pixel_red(81, 71));
    TEST_ASSERT_EQUAL_UINT8(255, pixel_red(75, 150));
    TEST_ASSERT_TRUE(pixel_red(160, 150) < 255);
    TEST_ASSERT_TRUE(pixel_red(90, 150) < 255); // Outside stroke survives inside mask.
    TEST_ASSERT_EQUAL_UINT8(255, pixel_red(245, 150));
}

static void pointer(Uint32 type, float x, float y) {
    VectorViewport *v = &document()->viewport;
    SDL_Event e = {.type = type};
    if (type == SDL_EVENT_MOUSE_MOTION) {
        e.motion.x = v->bounds.x + v->x + x * v->zoom;
        e.motion.y = v->bounds.y + v->y + y * v->zoom;
    } else {
        e.button.windowID = SDL_GetWindowID(window);
        e.button.button = SDL_BUTTON_LEFT;
        e.button.x = v->bounds.x + v->x + x * v->zoom;
        e.button.y = v->bounds.y + v->y + y * v->zoom;
    }
    TEST_ASSERT_TRUE(editor_host_event(&host, shell.view, &e));
    commit();
}

static void test_yaml_actions_tree_and_mouse_edit_the_mask_without_moving_content(void) {
    prepare();
    TEST_ASSERT_TRUE(editor_host_action(&host, shell.view, "vector.objects.mask_create", ""));
    commit();
    VectorDocument *d = document();
    TEST_ASSERT_TRUE(d->rectangles[1].clip_mask);
    TEST_ASSERT_EQUAL_INT(4, pui_view_tree_item_count(shell.view, "vector_document_tree"));
    TEST_ASSERT_TRUE(editor_host_action(&host, shell.view, "vector.rectangle.select",
                                        "vector_document_1.rectangle_2"));
    commit();
    TEST_ASSERT_EQUAL_INT(1, d->selection.count);
    TEST_ASSERT_EQUAL_STRING("Schnittmaske", pui_view_text(shell.view, "rectangle_kind"));
    TEST_ASSERT_EQUAL_STRING("vector_document_1.rectangle_2",
                             pui_view_selected_tree_item(shell.view, "vector_document_tree"));
    int history = d->history.cursor;
    pointer(SDL_EVENT_MOUSE_BUTTON_DOWN, 130, 120);
    pointer(SDL_EVENT_MOUSE_MOTION, 150, 135);
    TEST_ASSERT_EQUAL_INT(history, d->history.cursor);
    pointer(SDL_EVENT_MOUSE_BUTTON_UP, 150, 135);
    TEST_ASSERT_EQUAL_INT(history + 1, d->history.cursor);
    TEST_ASSERT_EQUAL_FLOAT(100, d->rectangles[1].rect.x);
    TEST_ASSERT_EQUAL_FLOAT(85, d->rectangles[1].rect.y);
    TEST_ASSERT_EQUAL_FLOAT(40, d->rectangles[0].rect.x);
    TEST_ASSERT_TRUE(vector_history_undo(&d->history, d));
    TEST_ASSERT_EQUAL_FLOAT(80, d->rectangles[1].rect.x);
    TEST_ASSERT_TRUE(
        editor_host_action(&host, shell.view, "vector.group.select", "vector_document_1.group_1"));
    TEST_ASSERT_TRUE(editor_host_action(&host, shell.view, "vector.objects.mask_release", ""));
    TEST_ASSERT_FALSE(d->rectangles[1].clip_mask);
}

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_yaml_actions_tree_and_mouse_edit_the_mask_without_moving_content);
    RUN_TEST(test_masked_group_renders_text_and_strokes_inside_the_same_boundary);
    RUN_TEST(test_reader_accepts_v1_and_rejects_malformed_mask_relations);
    RUN_TEST(test_create_release_and_history_preserve_both_original_objects);
    RUN_TEST(test_group_content_and_independent_mask_selection_transform_and_copy);
    RUN_TEST(test_existing_group_can_be_clipped_but_nested_masks_and_locked_edits_are_rejected);
    RUN_TEST(test_mask_picking_rejects_invisible_corners_and_respects_holes);
    RUN_TEST(test_render_clips_curves_strokes_and_alpha_without_painting_the_mask);
    RUN_TEST(test_file_roundtrip_and_editor_switch_preserve_editable_clipping_groups);
    return UNITY_END();
}
