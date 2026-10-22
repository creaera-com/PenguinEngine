#include "editor_host/host.h"
#include "editors/vector/curve/curve.h"
#include "editors/vector/document/selection.h"
#include "editors/vector/effects/render.h"
#include "editors/vector/ellipse/ellipse.h"
#include "editors/vector/export/render.h"
#include "editors/vector/gradient/render.h"
#include "editors/vector/gradient/tool.h"
#include "editors/vector/internal.h"
#include "editors/vector/mask/mask.h"
#include "editors/vector/mask/render.h"
#include "editors/vector/object/commands.h"
#include "editors/vector/object/group.h"
#include "editors/vector/path/operations.h"
#include "editors/vector/persistence/document_file.h"
#include "editors/vector/preview/render.h"
#include "editors/vector/preview/tool.h"
#include "editors/vector/rectangle/transform.h"
#include "editors/vector/rendering/objects.h"
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
static char folder[4096];

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
    char *cwd = SDL_GetCurrentDirectory();
    snprintf(folder, sizeof folder, "%sexport-test-%llu", cwd,
             (unsigned long long)SDL_GetPerformanceCounter());
    SDL_free(cwd);
    TEST_ASSERT_TRUE(SDL_CreateDirectory(folder));
}

void tearDown(void) {
    int count = 0;
    char **files = SDL_GlobDirectory(folder, "*", 0, &count);
    for (int i = 0; files && i < count; i++) {
        char path[8192];
        snprintf(path, sizeof path, "%s/%s", folder, files[i]);
        SDL_RemovePath(path);
    }
    SDL_free(files);
    SDL_RemovePath(folder);
    editor_shell_destroy(&shell);
    editor_host_destroy(&host);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
}

static uint32_t pixel(SDL_Surface *s, int x, int y) {
    Uint8 r, g, b, a;
    TEST_ASSERT_NOT_NULL(s);
    TEST_ASSERT_TRUE(SDL_ReadSurfacePixel(s, x, y, &r, &g, &b, &a));
    return ((uint32_t)r << 24) | ((uint32_t)g << 16) | ((uint32_t)b << 8) | a;
}

static VectorExportPlan plan(VectorExportScope scope) {
    VectorExportPlan p;
    char error[256];
    TEST_ASSERT_TRUE_MESSAGE(vector_export_plan(document(), scope, &p, error, sizeof error), error);
    return p;
}

static SDL_Surface *render(const VectorExportPlan *p, bool alpha) {
    VectorEditor *e = host.instance;
    return vector_export_render(document(), p, 0, renderer, e->text_renderer, e->image_renderer,
                                alpha);
}

static void test_artboard_dimensions_zoom_independent_and_straight_alpha(void) {
    VectorDocument *d = document();
    TEST_ASSERT_TRUE(vector_rectangle_set_fill(d, 1, 0xff000080));
    d->viewport.zoom = 3;
    d->viewport.x = 923;
    d->viewport.y = -400;
    VectorExportPlan p = plan(VECTOR_EXPORT_ARTBOARD);
    SDL_Surface *s = render(&p, true);
    TEST_ASSERT_EQUAL_INT(600, s->w);
    TEST_ASSERT_EQUAL_INT(400, s->h);
    TEST_ASSERT_EQUAL_HEX32(0, pixel(s, 0, 0));
    uint32_t c = pixel(s, 30, 40);
    TEST_ASSERT_EQUAL_UINT8(255, c >> 24);
    TEST_ASSERT_INT_WITHIN(1, 128, c & 255);
    SDL_DestroySurface(s);
    s = render(&p, false);
    TEST_ASSERT_EQUAL_HEX32(0xffffffff, pixel(s, 0, 0));
    TEST_ASSERT_INT_WITHIN(2, 127, (pixel(s, 30, 40) >> 16) & 255);
    SDL_DestroySurface(s);
    TEST_ASSERT_EQUAL_FLOAT(3, d->viewport.zoom);
    TEST_ASSERT_EQUAL_FLOAT(923, d->viewport.x);
}

static void test_selection_excludes_unselected_and_handles_and_keeps_effect_bounds(void) {
    VectorDocument *d = document();
    TEST_ASSERT_TRUE(vector_rectangle_set_fill(d, 1, 0xff0000ff));
    TEST_ASSERT_TRUE(vector_rectangle_add(d, 1, (PUIRect){20, 30, 40, 30}));
    TEST_ASSERT_TRUE(vector_rectangle_set_fill(d, 2, 0x0000ffff));
    vector_document_select(d, 1);
    VectorExportPlan p = plan(VECTOR_EXPORT_SELECTION);
    TEST_ASSERT_EQUAL_FLOAT(40, p.items[0].bounds.w);
    TEST_ASSERT_EQUAL_FLOAT(30, p.items[0].bounds.h);
    int cursor = d->history.cursor;
    SDL_Surface *s = render(&p, true);
    TEST_ASSERT_EQUAL_HEX32(0xff0000ff, pixel(s, 20, 15));
    SDL_DestroySurface(s);
    TEST_ASSERT_EQUAL_INT(cursor, d->history.cursor);
    TEST_ASSERT_EQUAL_UINT(1, d->active_rectangle);
    VectorShape v = d->rectangles[0];
    v.shadow = vector_shadow_default();
    TEST_ASSERT_TRUE(vector_shape_update(d, &v));
    p = plan(VECTOR_EXPORT_SELECTION);
    TEST_ASSERT_GREATER_THAN(40, p.items[0].bounds.w);
    TEST_ASSERT_GREATER_THAN(30, p.items[0].bounds.h);
    vector_document_select(d, 0);
    char error[256];
    TEST_ASSERT_FALSE(vector_export_plan(d, VECTOR_EXPORT_SELECTION, &p, error, sizeof error));
}

static void test_masks_gradient_and_multiple_artboard_selection(void) {
    VectorDocument *d = document();
    VectorShape shape = d->rectangles[0];
    shape.gradient = vector_gradient_default(0xff000080);
    shape.gradient.stops[1].color = 0x0000ff80;
    TEST_ASSERT_TRUE(vector_shape_update(d, &shape));
    TEST_ASSERT_TRUE(vector_rectangle_add(d, 1, (PUIRect){20, 30, 20, 30}));
    vector_document_select(d, 2);
    vector_document_toggle(d, 1);
    TEST_ASSERT_TRUE(vector_mask_create(d));
    VectorExportPlan p = plan(VECTOR_EXPORT_SELECTION);
    TEST_ASSERT_EQUAL_FLOAT(20, p.items[0].bounds.w);
    SDL_Surface *s = render(&p, true);
    TEST_ASSERT_GREATER_THAN(150, pixel(s, 5, 10) >> 24);
    TEST_ASSERT_INT_WITHIN(1, 128, pixel(s, 5, 10) & 255);
    SDL_DestroySurface(s);
    TEST_ASSERT_TRUE(vector_artboard_add(d, (PUIRect){700, 0, 80, 70}));
    TEST_ASSERT_TRUE(vector_rectangle_add(d, 2, (PUIRect){10, 10, 20, 20}));
    TEST_ASSERT_TRUE(vector_rectangle_set_fill(d, 3, 0x00ff00ff));
    vector_document_select(d, 1);
    vector_document_toggle(d, 3);
    p = plan(VECTOR_EXPORT_SELECTION);
    s = render(&p, true);
    TEST_ASSERT_EQUAL_INT(710, s->w);
    TEST_ASSERT_EQUAL_HEX32(0x00ff00ff, pixel(s, 695, 5));
    SDL_DestroySurface(s);
}

static void test_png_jpeg_batch_files_decode_and_have_correct_backgrounds(void) {
    VectorDocument *d = document();
    TEST_ASSERT_TRUE(vector_rectangle_set_fill(d, 1, 0xff000080));
    TEST_ASSERT_TRUE(vector_artboard_add(d, (PUIRect){700.5f, 0, 80, 70}));
    VectorExportPlan p = plan(VECTOR_EXPORT_ALL);
    TEST_ASSERT_EQUAL_UINT(2, p.count);
    TEST_ASSERT_EQUAL_FLOAT(80, p.items[1].bounds.w);
    VectorEditor *e = host.instance;
    char error[256], path[8192];
    TEST_ASSERT_TRUE_MESSAGE(vector_export_write(d, &p, renderer, e->text_renderer,
                                                 e->image_renderer, folder, "Batch", false, true,
                                                 90, error, sizeof error),
                             error);
    snprintf(path, sizeof path, "%s/Batch-1-Artboard 1.png", folder);
    SDL_Surface *s = editor_image_decode(path);
    TEST_ASSERT_NOT_NULL(s);
    TEST_ASSERT_EQUAL_HEX32(0, pixel(s, 0, 0));
    SDL_DestroySurface(s);
    d->active_artboard = 1;
    p = plan(VECTOR_EXPORT_ARTBOARD);
    TEST_ASSERT_TRUE(vector_export_write(d, &p, renderer, e->text_renderer, e->image_renderer,
                                         folder, "JPEG", true, true, 95, error, sizeof error));
    snprintf(path, sizeof path, "%s/JPEG.jpg", folder);
    s = editor_image_decode(path);
    TEST_ASSERT_NOT_NULL(s);
    TEST_ASSERT_INT_WITHIN(2, 255, pixel(s, 0, 0) >> 24);
    TEST_ASSERT_EQUAL_UINT8(255, pixel(s, 30, 40) & 255);
    SDL_DestroySurface(s);
}

static void test_renderer_state_and_live_previews_are_not_exported(void) {
    VectorDocument *d = document();
    TEST_ASSERT_TRUE(vector_rectangle_set_fill(d, 1, 0x00ff00ff));
    d->fill_preview = (VectorFillPreview){.rectangle_id = 1, .color = 0xff0000ff, .active = true};
    VectorExportPlan p = plan(VECTOR_EXPORT_ARTBOARD);
    SDL_Texture *target =
        SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGBA8888, SDL_TEXTUREACCESS_TARGET, 100, 100);
    TEST_ASSERT_NOT_NULL(target);
    TEST_ASSERT_TRUE(SDL_SetRenderTarget(renderer, target));
    SDL_SetRenderScale(renderer, 2, 2);
    SDL_Rect clip = {1, 2, 10, 20}, viewport = {3, 4, 30, 30};
    SDL_SetRenderViewport(renderer, &viewport);
    SDL_SetRenderClipRect(renderer, &clip);
    SDL_SetRenderDrawColor(renderer, 3, 5, 7, 9);
    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_ADD);
    SDL_Surface *s = render(&p, true);
    TEST_ASSERT_EQUAL_HEX32(0x00ff00ff, pixel(s, 30, 40));
    SDL_DestroySurface(s);
    TEST_ASSERT_EQUAL_PTR(target, SDL_GetRenderTarget(renderer));
    float x, y;
    SDL_GetRenderScale(renderer, &x, &y);
    TEST_ASSERT_EQUAL_FLOAT(2, x);
    SDL_Rect actual;
    SDL_GetRenderClipRect(renderer, &actual);
    TEST_ASSERT_EQUAL_MEMORY(&clip, &actual, sizeof clip);
    SDL_GetRenderViewport(renderer, &actual);
    TEST_ASSERT_EQUAL_MEMORY(&viewport, &actual, sizeof viewport);
    Uint8 r, g, b, a;
    SDL_GetRenderDrawColor(renderer, &r, &g, &b, &a);
    TEST_ASSERT_EQUAL_UINT8(9, a);
    SDL_BlendMode blend;
    SDL_GetRenderDrawBlendMode(renderer, &blend);
    TEST_ASSERT_EQUAL_INT(SDL_BLENDMODE_ADD, blend);
    SDL_SetRenderTarget(renderer, NULL);
    SDL_DestroyTexture(target);
}

static void test_yaml_export_snapshots_current_document_and_rejects_huge_artboards(void) {
    VectorDocument *d = document();
    TEST_ASSERT_TRUE(pui_view_open_modal(shell.view, &shell.input, "vector_export_dialog"));
    commit();
    TEST_ASSERT_TRUE(pui_view_modal_active(shell.view));
    TEST_ASSERT_TRUE(pui_view_set_text(shell.view, "vector_export_directory", folder));
    TEST_ASSERT_TRUE(pui_view_set_text(shell.view, "vector_export_name", "UI.png"));
    int cursor = d->history.cursor;
    TEST_ASSERT_TRUE(editor_host_action(&host, shell.view, "vector.export.write", ""));
    commit();
    char path[8192];
    snprintf(path, sizeof path, "%s/UI.png", folder);
    SDL_Surface *s = editor_image_decode(path);
    TEST_ASSERT_NOT_NULL(s);
    SDL_DestroySurface(s);
    TEST_ASSERT_EQUAL_INT(cursor, d->history.cursor);
    TEST_ASSERT_TRUE(vector_artboard_update(d, 1, "Huge", 10000, 10000));
    VectorExportPlan p;
    char error[256];
    TEST_ASSERT_FALSE(vector_export_plan(d, VECTOR_EXPORT_ALL, &p, error, sizeof error));
}

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_artboard_dimensions_zoom_independent_and_straight_alpha);
    RUN_TEST(test_selection_excludes_unselected_and_handles_and_keeps_effect_bounds);
    RUN_TEST(test_masks_gradient_and_multiple_artboard_selection);
    RUN_TEST(test_png_jpeg_batch_files_decode_and_have_correct_backgrounds);
    RUN_TEST(test_renderer_state_and_live_previews_are_not_exported);
    RUN_TEST(test_yaml_export_snapshots_current_document_and_rejects_huge_artboards);
    return UNITY_END();
}
