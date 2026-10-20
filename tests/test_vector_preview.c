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
    commit();
}

void tearDown(void) {
    editor_shell_destroy(&shell);
    editor_host_destroy(&host);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
}

static void pointer(Uint32 type, float x, float y) {
    VectorViewport *v = &document()->viewport;
    SDL_Event e = {.type = type};
    if (type == SDL_EVENT_MOUSE_MOTION) {
        e.motion.x = v->bounds.x + v->x + x * v->zoom;
        e.motion.y = v->bounds.y + v->y + y * v->zoom;
    } else {
        e.button.button = SDL_BUTTON_LEFT;
        e.button.windowID = SDL_GetWindowID(window);
        e.button.x = v->bounds.x + v->x + x * v->zoom;
        e.button.y = v->bounds.y + v->y + y * v->zoom;
    }
    TEST_ASSERT_TRUE(editor_host_event(&host, shell.view, &e));
    commit();
}

static void select_region(float x, float y, float right, float bottom) {
    TEST_ASSERT_TRUE(editor_host_action(&host, shell.view, "vector.preview.select", ""));
    pointer(SDL_EVENT_MOUSE_BUTTON_DOWN, x, y);
    pointer(SDL_EVENT_MOUSE_MOTION, right, bottom);
    pointer(SDL_EVENT_MOUSE_BUTTON_UP, right, bottom);
}

static void test_window_keeps_pixel_scale_and_follows_large_selection(void) {
    VectorPreviewWindow w;
    TEST_ASSERT_TRUE(vector_preview_window((PUIRect){10, 20, 80, 60}, (PUIRect){100, 200, 200, 100},
                                           900, 900, &w));
    TEST_ASSERT_EQUAL_FLOAT(80, w.source.w);
    TEST_ASSERT_EQUAL_FLOAT(w.source.w, w.destination.w);
    TEST_ASSERT_EQUAL_FLOAT(160, w.destination.x);
    TEST_ASSERT_EQUAL_FLOAT(220, w.destination.y);
    TEST_ASSERT_TRUE(vector_preview_window((PUIRect){10, 20, 800, 600}, (PUIRect){0, 0, 200, 100},
                                           500, 300, &w));
    TEST_ASSERT_EQUAL_FLOAT(400, w.source.x);
    TEST_ASSERT_EQUAL_FLOAT(250, w.source.y);
    TEST_ASSERT_TRUE(vector_preview_window((PUIRect){10, 20, 800, 600}, (PUIRect){0, 0, 200, 100},
                                           -900, 9000, &w));
    TEST_ASSERT_EQUAL_FLOAT(10, w.source.x);
    TEST_ASSERT_EQUAL_FLOAT(520, w.source.y);
    TEST_ASSERT_FALSE(
        vector_preview_window((PUIRect){0, 0, 0, 20}, (PUIRect){0, 0, 20, 20}, 0, 0, &w));
    TEST_ASSERT_FALSE(vector_preview_valid(1, (PUIRect){NAN, 0, 20, 20}));
    TEST_ASSERT_TRUE(vector_preview_valid(1, (PUIRect){0, 0, 1000000, 1000000}));
}

static void test_mouse_region_is_zoom_independent_and_does_not_enter_history(void) {
    VectorDocument *d = document();
    int history = d->history.cursor;
    d->viewport.zoom = 1.5f;
    select_region(120, 100, 40, 30);
    TEST_ASSERT_EQUAL_UINT(1, d->preview.board);
    TEST_ASSERT_EQUAL_FLOAT(40, d->preview.region.x);
    TEST_ASSERT_EQUAL_FLOAT(30, d->preview.region.y);
    TEST_ASSERT_EQUAL_FLOAT(80, d->preview.region.w);
    TEST_ASSERT_EQUAL_FLOAT(70, d->preview.region.h);
    TEST_ASSERT_FALSE(d->preview.armed);
    TEST_ASSERT_EQUAL_INT(history, d->history.cursor);
    PUIRect region;
    TEST_ASSERT_NOT_NULL(vector_preview_region(d, &region));
    TEST_ASSERT_TRUE(vector_artboard_update(d, 1, "Smaller", 70, 60));
    TEST_ASSERT_NOT_NULL(vector_preview_region(d, &region));
    TEST_ASSERT_EQUAL_FLOAT(30, region.w);
    TEST_ASSERT_EQUAL_FLOAT(30, region.h);
    TEST_ASSERT_TRUE(vector_history_undo(&d->history, d));
    TEST_ASSERT_NOT_NULL(vector_preview_region(d, &region));
    TEST_ASSERT_EQUAL_FLOAT(80, region.w);
}

static void test_cancel_and_tool_switch_preserve_the_previous_region(void) {
    select_region(20, 20, 100, 100);
    VectorDocument *d = document();
    PUIRect before = d->preview.region;
    TEST_ASSERT_TRUE(editor_host_action(&host, shell.view, "vector.preview.select", ""));
    pointer(SDL_EVENT_MOUSE_BUTTON_DOWN, 40, 40);
    pointer(SDL_EVENT_MOUSE_MOTION, 80, 90);
    SDL_Event e = {.type = SDL_EVENT_KEY_DOWN};
    e.key.key = SDLK_ESCAPE;
    TEST_ASSERT_TRUE(editor_host_event(&host, shell.view, &e));
    TEST_ASSERT_FALSE(d->preview.dragging);
    TEST_ASSERT_EQUAL_MEMORY(&before, &d->preview.region, sizeof before);
    TEST_ASSERT_TRUE(editor_host_action(&host, shell.view, "vector.preview.select", ""));
    TEST_ASSERT_TRUE(d->preview.armed);
    TEST_ASSERT_TRUE(editor_host_action(&host, shell.view, "vector.add_rect", ""));
    TEST_ASSERT_FALSE(d->preview.armed);
    TEST_ASSERT_TRUE(d->rectangle_tool.enabled);
    TEST_ASSERT_TRUE(editor_host_action(&host, shell.view, "vector.preview.clear", ""));
    TEST_ASSERT_EQUAL_UINT(0, d->preview.board);
}

static void test_regions_are_per_document_and_survive_editor_unloading(void) {
    select_region(20, 30, 180, 150);
    TEST_ASSERT_TRUE(editor_host_action(&host, shell.view, "vector.add_document", ""));
    commit();
    TEST_ASSERT_EQUAL_UINT(0, document()->preview.board);
    activate("scripts");
    activate("vector");
    VectorEditor *editor = host.instance;
    VectorDocument *first = vector_documents_find(&editor->documents, 1);
    TEST_ASSERT_EQUAL_UINT(1, first->preview.board);
    TEST_ASSERT_EQUAL_FLOAT(160, first->preview.region.w);
    TEST_ASSERT_FALSE(first->preview.armed);
    TEST_ASSERT_EQUAL_UINT(0, document()->preview.board);
}

static Uint8 red(int x, int y) {
    SDL_Surface *surface = SDL_RenderReadPixels(renderer, &(SDL_Rect){x, y, 1, 1});
    TEST_ASSERT_NOT_NULL(surface);
    Uint8 r, g, b, a;
    TEST_ASSERT_TRUE(SDL_ReadSurfacePixel(surface, 0, 0, &r, &g, &b, &a));
    SDL_DestroySurface(surface);
    return r;
}

static void test_preview_draws_masks_at_one_to_one_without_mutating_selection_or_view(void) {
    VectorDocument *d = document();
    TEST_ASSERT_TRUE(vector_rectangle_add(d, 1, (PUIRect){40, 50, 240, 220}));
    TEST_ASSERT_TRUE(vector_rectangle_set_fill(d, 1, 0xe05030ff));
    TEST_ASSERT_TRUE(vector_ellipse_add(d, 1, (PUIRect){80, 70, 160, 160}));
    vector_document_select(d, 1);
    vector_document_toggle(d, 2);
    TEST_ASSERT_TRUE(vector_mask_create(d));
    d->preview.board = 1;
    d->preview.region = (PUIRect){60, 50, 200, 200};
    d->viewport.zoom = .5f;
    VectorViewport before = d->viewport;
    VectorRectangleSelection selected = d->selection;
    SDL_SetRenderClipRect(renderer, NULL);
    vector_preview_draw(d, renderer, (PUIRect){0, 0, 208, 228},
                        ((VectorEditor *)host.instance)->text_renderer,
                        ((VectorEditor *)host.instance)->image_renderer);
    // Region starts at (60,50), displayed at (4,24), regardless of workspace zoom.
    TEST_ASSERT_EQUAL_UINT8(0xe0, red(104, 124));
    TEST_ASSERT_EQUAL_UINT8(242, red(25, 45)); // Ellipse corner is clipped out.
    TEST_ASSERT_EQUAL_UINT8(242, red(14, 34));
    TEST_ASSERT_FALSE(SDL_RenderClipEnabled(renderer));
    TEST_ASSERT_EQUAL_MEMORY(&before, &d->viewport, sizeof before);
    TEST_ASSERT_EQUAL_MEMORY(&selected, &d->selection, sizeof selected);
}

static void test_preview_shows_live_move_without_selection_handles_or_model_changes(void) {
    VectorDocument *d = document();
    TEST_ASSERT_TRUE(vector_rectangle_add(d, 1, (PUIRect){20, 20, 20, 20}));
    TEST_ASSERT_TRUE(vector_rectangle_set_fill(d, 1, 0xe05030ff));
    vector_document_select(d, 1);
    d->preview.board = 1;
    d->preview.region = (PUIRect){0, 0, 100, 100};
    d->selection_tool = (VectorSelectionTool){.dragging = true,
                                              .artboard_id = 1,
                                              .rectangle_id = 1,
                                              .offset_x = 40,
                                              .offset_y = 20,
                                              .before = d->selection};
    VectorShape before = d->rectangles[0];
    SDL_SetRenderClipRect(renderer, NULL);
    vector_preview_draw(d, renderer, (PUIRect){0, 0, 108, 128},
                        ((VectorEditor *)host.instance)->text_renderer,
                        ((VectorEditor *)host.instance)->image_renderer);
    TEST_ASSERT_EQUAL_UINT8(242, red(30, 50));
    TEST_ASSERT_EQUAL_UINT8(0xe0, red(70, 70));
    TEST_ASSERT_EQUAL_MEMORY(&before, &d->rectangles[0], sizeof before);
    TEST_ASSERT_TRUE(d->selection_tool.dragging);
}

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_preview_shows_live_move_without_selection_handles_or_model_changes);
    RUN_TEST(test_window_keeps_pixel_scale_and_follows_large_selection);
    RUN_TEST(test_mouse_region_is_zoom_independent_and_does_not_enter_history);
    RUN_TEST(test_cancel_and_tool_switch_preserve_the_previous_region);
    RUN_TEST(test_regions_are_per_document_and_survive_editor_unloading);
    RUN_TEST(test_preview_draws_masks_at_one_to_one_without_mutating_selection_or_view);
    return UNITY_END();
}
