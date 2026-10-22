#include "editor_host/host.h"
#include "editors/vector/document/selection.h"
#include "editors/vector/effects/render.h"
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

static VectorShadow hard_shadow(void) {
    return (VectorShadow){.enabled = 1, .color = 0x000000ff, .offset_x = 100};
}

static void set_shadow(VectorShadow s) {
    VectorShape value = document()->rectangles[0];
    value.shadow = s;
    TEST_ASSERT_TRUE(vector_shape_update(document(), &value));
}

static SDL_Surface *render_objects(void) {
    VectorDocument *d = document();
    VectorEditor *editor = host.instance;
    vector_document_select(d, 0);
    SDL_SetRenderClipRect(renderer, NULL);
    SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
    SDL_RenderClear(renderer);
    d->viewport.zoom = 1;
    vector_rectangles_draw(d, &d->artboards[0], renderer, (SDL_FRect){0, 0, 600, 400},
                           editor->text_renderer, editor->image_renderer);
    return SDL_RenderReadPixels(renderer, NULL);
}

static Uint8 red(SDL_Surface *surface, int x, int y) {
    Uint8 r, g, b, a;
    TEST_ASSERT_NOT_NULL(surface);
    TEST_ASSERT_TRUE(SDL_ReadSurfacePixel(surface, x, y, &r, &g, &b, &a));
    return r;
}

static void test_shadow_is_undoable_validated_and_inherits_locks(void) {
    VectorDocument *d = document();
    int cursor = d->history.cursor;
    set_shadow(vector_shadow_default());
    TEST_ASSERT_EQUAL_INT(cursor + 1, d->history.cursor);
    TEST_ASSERT_TRUE(vector_history_undo(&d->history, d));
    TEST_ASSERT_FALSE(d->rectangles[0].shadow.enabled);
    TEST_ASSERT_TRUE(vector_history_redo(&d->history, d));
    TEST_ASSERT_EQUAL_FLOAT(12, d->rectangles[0].shadow.blur);
    VectorShape bad = d->rectangles[0];
    bad.shadow.blur = NAN;
    TEST_ASSERT_FALSE(vector_shape_update(d, &bad));
    bad.shadow.blur = -1;
    TEST_ASSERT_FALSE(vector_shape_update(d, &bad));
    bad.shadow.blur = 129;
    TEST_ASSERT_FALSE(vector_shape_update(d, &bad));
    TEST_ASSERT_TRUE(vector_artboard_set_state(d, 1, VECTOR_OBJECT_LOCKED));
    bad = d->rectangles[0];
    bad.shadow.offset_x = 42;
    TEST_ASSERT_FALSE(vector_shape_update(d, &bad));
}

static void test_alpha_silhouette_softness_spread_and_opacity(void) {
    set_shadow(hard_shadow());
    SDL_Surface *pixels = render_objects();
    TEST_ASSERT_EQUAL_UINT8(0, red(pixels, 130, 40));
    TEST_ASSERT_EQUAL_UINT8(255, red(pixels, 118, 40));
    SDL_DestroySurface(pixels);
    VectorShadow s = hard_shadow();
    s.blur = 9;
    set_shadow(s);
    pixels = render_objects();
    TEST_ASSERT_INT_WITHIN(100, 150, red(pixels, 118, 40));
    SDL_DestroySurface(pixels);
    s.blur = 0;
    s.spread = 5;
    s.color = 0x00000080;
    set_shadow(s);
    pixels = render_objects();
    TEST_ASSERT_INT_WITHIN(2, 127, red(pixels, 117, 40));
    SDL_DestroySurface(pixels);
    /* A transparent fill and a visible stroke cast a ring, not an opaque bounding box. */
    s.spread = 0;
    s.color = 0x000000ff;
    set_shadow(s);
    TEST_ASSERT_TRUE(vector_rectangle_set_fill(document(), 1, 0));
    TEST_ASSERT_TRUE(vector_rectangle_set_stroke(
        document(), 1, (VectorStroke){0xff0000ff, 3, VECTOR_STROKE_INSIDE}));
    pixels = render_objects();
    TEST_ASSERT_EQUAL_UINT8(255, red(pixels, 140, 45));
    TEST_ASSERT_EQUAL_UINT8(0, red(pixels, 121, 40));
    SDL_DestroySurface(pixels);
}

static void test_linked_instances_draw_the_same_effect(void) {
    set_shadow((VectorShadow){.enabled = 1, .color = 0x000000ff, .offset_y = 40});
    VectorRepeat r = vector_repeat_default();
    r.columns = 2;
    r.rows = 1;
    r.gap_x = 60;
    TEST_ASSERT_TRUE(vector_repeat_apply(document(), &r, true));
    SDL_Surface *pixels = render_objects();
    TEST_ASSERT_EQUAL_UINT8(0, red(pixels, 30, 80));
    TEST_ASSERT_EQUAL_UINT8(0, red(pixels, 130, 80));
    SDL_DestroySurface(pixels);
}

static void test_shadow_persistence_clipboard_and_editor_checkpoint(void) {
    VectorShadow s = vector_shadow_default();
    s.enabled = 0; /* Disabled effects retain their configured parameters. */
    s.offset_x = -15;
    s.spread = 4;
    set_shadow(s);
    VectorDocument *d = document();
    TEST_ASSERT_TRUE(vector_objects_duplicate(d));
    TEST_ASSERT_EQUAL_MEMORY(&s, &d->rectangles[1].shadow, sizeof s);
    TEST_ASSERT_TRUE(vector_history_undo(&d->history, d));
    char file[4096], error[256];
    char *cwd = SDL_GetCurrentDirectory();
    snprintf(file, sizeof file, "%sshadow-%llu.pengvec", cwd,
             (unsigned long long)SDL_GetPerformanceCounter());
    SDL_free(cwd);
    TEST_ASSERT_TRUE_MESSAGE(vector_document_file_save(d, file, error, sizeof error), error);
    VectorDocument *loaded = calloc(1, sizeof *loaded);
    TEST_ASSERT_NOT_NULL(loaded);
    vector_document_init(loaded, 1);
    TEST_ASSERT_TRUE_MESSAGE(vector_document_file_load(loaded, file, error, sizeof error), error);
    TEST_ASSERT_EQUAL_MEMORY(&s, &loaded->rectangles[0].shadow, sizeof s);
    vector_history_destroy(&loaded->history);
    free(loaded);
    SDL_RemovePath(file);
    activate("scripts");
    activate("vector");
    d = document();
    TEST_ASSERT_EQUAL_MEMORY(&s, &d->rectangles[0].shadow, sizeof s);
    TEST_ASSERT_TRUE(vector_history_undo(&d->history, d));
    TEST_ASSERT_EQUAL_FLOAT(0, d->rectangles[0].shadow.blur);
}

static void test_yaml_actions_and_rejected_inputs_restore_presented_values(void) {
    VectorDocument *d = document();
    TEST_ASSERT_TRUE(pui_view_set_checked(shell.view, "shadow_enabled", true));
    TEST_ASSERT_TRUE(
        editor_host_action(&host, shell.view, "vector.shadow.enabled", "shadow_enabled"));
    commit();
    TEST_ASSERT_TRUE(d->rectangles[0].shadow.enabled);
    TEST_ASSERT_EQUAL_STRING("12", pui_view_text(shell.view, "shadow_blur"));
    TEST_ASSERT_TRUE(pui_view_set_text(shell.view, "shadow_x", "-20"));
    TEST_ASSERT_TRUE(editor_host_action(&host, shell.view, "vector.shadow.x", "shadow_x"));
    commit();
    TEST_ASSERT_EQUAL_FLOAT(-20, d->rectangles[0].shadow.offset_x);
    TEST_ASSERT_TRUE(pui_view_set_text(shell.view, "shadow_blur", "-10"));
    TEST_ASSERT_TRUE(editor_host_action(&host, shell.view, "vector.shadow.blur", "shadow_blur"));
    commit();
    TEST_ASSERT_EQUAL_STRING("12", pui_view_text(shell.view, "shadow_blur"));
    TEST_ASSERT_EQUAL_FLOAT(12, d->rectangles[0].shadow.blur);
}

static void test_offscreen_source_still_casts_visible_shadow_and_zoom_scales_it(void) {
    TEST_ASSERT_TRUE(
        vector_rectangle_update(document(), 1, "Outside", (PUIRect){-100, 30, 40, 30}));
    VectorShadow s = hard_shadow();
    s.offset_x = 160;
    set_shadow(s);
    SDL_Surface *pixels = render_objects();
    TEST_ASSERT_EQUAL_UINT8(0, red(pixels, 70, 40));
    SDL_DestroySurface(pixels);
    VectorDocument *d = document();
    TEST_ASSERT_TRUE(vector_rectangle_update(d, 1, "Inside", (PUIRect){20, 30, 40, 30}));
    s.offset_x = 100;
    set_shadow(s);
    d->viewport.zoom = 2;
    VectorEditor *editor = host.instance;
    SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
    SDL_RenderClear(renderer);
    SDL_Rect clip = {0, 0, 500, 300};
    SDL_SetRenderClipRect(renderer, &clip);
    SDL_SetRenderDrawColor(renderer, 7, 8, 9, 10);
    vector_rectangles_draw(d, &d->artboards[0], renderer, (SDL_FRect){0, 0, 1200, 800},
                           editor->text_renderer, editor->image_renderer);
    SDL_Rect after;
    SDL_GetRenderClipRect(renderer, &after);
    TEST_ASSERT_EQUAL_MEMORY(&clip, &after, sizeof clip);
    TEST_ASSERT_NULL(SDL_GetRenderTarget(renderer));
    pixels = SDL_RenderReadPixels(renderer, NULL);
    TEST_ASSERT_EQUAL_UINT8(0, red(pixels, 250, 80));
    TEST_ASSERT_EQUAL_UINT8(255, red(pixels, 130, 80));
    SDL_DestroySurface(pixels);
}

static void test_version_four_files_load_without_effects(void) {
    char path[4096], error[256];
    char *cwd = SDL_GetCurrentDirectory();
    snprintf(path, sizeof path, "%sshadow-legacy-%llu.pengvec", cwd,
             (unsigned long long)SDL_GetPerformanceCounter());
    SDL_free(cwd);
    TEST_ASSERT_TRUE(vector_document_file_save(document(), path, error, sizeof error));
    size_t length = 0;
    unsigned char *bytes = SDL_LoadFile(path, &length);
    TEST_ASSERT_NOT_NULL(bytes);
    TEST_ASSERT_TRUE(length > 24);
    bytes[8] = 4;
    TEST_ASSERT_TRUE(SDL_SaveFile(path, bytes, length - 76));
    SDL_free(bytes);
    VectorDocument *loaded = calloc(1, sizeof *loaded);
    TEST_ASSERT_NOT_NULL(loaded);
    vector_document_init(loaded, 1);
    TEST_ASSERT_TRUE_MESSAGE(vector_document_file_load(loaded, path, error, sizeof error), error);
    TEST_ASSERT_FALSE(loaded->rectangles[0].shadow.enabled);
    vector_history_destroy(&loaded->history);
    free(loaded);
    SDL_RemovePath(path);
}

static void test_clipping_masks_clip_shadows_and_preview_uses_the_same_rendering(void) {
    set_shadow(hard_shadow());
    VectorDocument *d = document();
    TEST_ASSERT_TRUE(vector_rectangle_add(d, 1, (PUIRect){0, 0, 140, 100}));
    vector_document_select(d, 2);
    vector_document_toggle(d, 1);
    TEST_ASSERT_TRUE(vector_mask_create(d));
    SDL_Surface *pixels = render_objects();
    TEST_ASSERT_EQUAL_UINT8(0, red(pixels, 130, 40));
    TEST_ASSERT_EQUAL_UINT8(255, red(pixels, 150, 40));
    SDL_DestroySurface(pixels);
    d->preview.board = 1;
    d->preview.region = (PUIRect){0, 0, 200, 100};
    VectorEditor *editor = host.instance;
    vector_preview_draw(d, renderer, (PUIRect){0, 0, 208, 128}, editor->text_renderer,
                        editor->image_renderer, NULL);
    pixels = SDL_RenderReadPixels(renderer, NULL);
    TEST_ASSERT_EQUAL_UINT8(0, red(pixels, 134, 64));
    TEST_ASSERT_EQUAL_UINT8(242, red(pixels, 154, 64));
    SDL_DestroySurface(pixels);
}

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_clipping_masks_clip_shadows_and_preview_uses_the_same_rendering);
    RUN_TEST(test_offscreen_source_still_casts_visible_shadow_and_zoom_scales_it);
    RUN_TEST(test_version_four_files_load_without_effects);
    RUN_TEST(test_shadow_is_undoable_validated_and_inherits_locks);
    RUN_TEST(test_alpha_silhouette_softness_spread_and_opacity);
    RUN_TEST(test_linked_instances_draw_the_same_effect);
    RUN_TEST(test_shadow_persistence_clipboard_and_editor_checkpoint);
    RUN_TEST(test_yaml_actions_and_rejected_inputs_restore_presented_values);
    return UNITY_END();
}
