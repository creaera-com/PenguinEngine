#include "editors/vector/document/selection.h"
#include "editors/vector/export/render.h"
#include "editors/vector/rendering/artwork.h"
#include "unity.h"
#include <stdlib.h>

static SDL_Window *window;
static SDL_Renderer *renderer;
static VectorDocument *document;
static VectorArtworkRenderer artwork;

void setUp(void) {
    TEST_ASSERT_TRUE(SDL_Init(SDL_INIT_VIDEO));
    window = SDL_CreateWindow("antialias", 1200, 240, SDL_WINDOW_HIDDEN);
    renderer = SDL_CreateRenderer(window, NULL);
    TEST_ASSERT_NOT_NULL(renderer);
    document = malloc(sizeof *document);
    TEST_ASSERT_NOT_NULL(document);
    vector_document_init(document, 1);
    TEST_ASSERT_TRUE(vector_artboard_add(document, (PUIRect){0, 0, 1200, 240}));
    TEST_ASSERT_TRUE(vector_rectangle_add(document, 1, (PUIRect){50, 45, 110, 80}));
    TEST_ASSERT_TRUE(vector_rectangle_set_fill(document, 1, 0xffffffff));
    TEST_ASSERT_TRUE(vector_rectangle_set_angle(document, 1, 23));
    document->active_rectangle = 0;
    document->selection.count = 0;
    artwork = (VectorArtworkRenderer){0};
}

void tearDown(void) {
    vector_artwork_renderer_destroy(&artwork);
    vector_artwork_renderer_destroy(&artwork);
    vector_history_destroy(&document->history);
    free(document);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
}

static void clear(Uint8 alpha) {
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, alpha);
    SDL_RenderClear(renderer);
}

static int partial_pixels(SDL_Surface *s, bool alpha_channel) {
    int count = 0;
    for (int y = 10; y < 180; y++) {
        for (int x = 10; x < 220; x++) {
            Uint8 r, g, b, a;
            TEST_ASSERT_TRUE(SDL_ReadSurfacePixel(s, x, y, &r, &g, &b, &a));
            Uint8 value = alpha_channel ? a : r;
            count += value > 0 && value < 255;
        }
    }
    return count;
}

static void test_rotated_edges_have_coverage_and_cache_is_reused_without_model_edits(void) {
    clear(255);
    vector_objects_draw(document, &document->artboards[0], renderer, (SDL_FRect){0, 0, 1200, 240},
                        NULL, NULL);
    SDL_Surface *native = SDL_RenderReadPixels(renderer, NULL);
    TEST_ASSERT_NOT_NULL(native);
    int before = partial_pixels(native, false);
    SDL_DestroySurface(native);
    int history = document->history.count;
    VectorViewport viewport = document->viewport;
    clear(255);
    vector_artwork_draw(&artwork, document, &document->artboards[0], renderer,
                        (SDL_FRect){0, 0, 1200, 240}, NULL, NULL);
    SDL_Surface *smooth = SDL_RenderReadPixels(renderer, NULL);
    TEST_ASSERT_NOT_NULL(smooth);
    TEST_ASSERT_TRUE(partial_pixels(smooth, false) > before + 50);
    SDL_DestroySurface(smooth);
    TEST_ASSERT_EQUAL_MEMORY(&viewport, &document->viewport, sizeof viewport);
    TEST_ASSERT_EQUAL_INT(history, document->history.count);
    TEST_ASSERT_TRUE(artwork.drawing.width <= 2048 && artwork.drawing.height <= 2048);
    SDL_Texture *texture = artwork.drawing.target;
    vector_artwork_draw(&artwork, document, &document->artboards[0], renderer,
                        (SDL_FRect){0, 0, 1200, 240}, NULL, NULL);
    TEST_ASSERT_EQUAL_PTR(texture, artwork.drawing.target);
}

static void test_tile_boundary_and_renderer_state_are_preserved(void) {
    TEST_ASSERT_TRUE(vector_rectangle_set_angle(document, 1, 0));
    TEST_ASSERT_TRUE(
        vector_rectangle_update(document, 1, "Across tiles", (PUIRect){0, 0, 1200, 240}));
    clear(255);
    SDL_Rect clip = {3, 4, 1190, 230};
    SDL_SetRenderClipRect(renderer, &clip);
    SDL_SetRenderDrawColor(renderer, 7, 8, 9, 10);
    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_ADD);
    vector_artwork_draw(&artwork, document, &document->artboards[0], renderer,
                        (SDL_FRect){0, 0, 1200, 240}, NULL, NULL);
    SDL_Rect restored;
    SDL_GetRenderClipRect(renderer, &restored);
    TEST_ASSERT_EQUAL_MEMORY(&clip, &restored, sizeof clip);
    SDL_BlendMode blend;
    SDL_GetRenderDrawBlendMode(renderer, &blend);
    TEST_ASSERT_EQUAL_INT(SDL_BLENDMODE_ADD, blend);
    Uint8 r, g, b, a;
    SDL_GetRenderDrawColor(renderer, &r, &g, &b, &a);
    TEST_ASSERT_EQUAL_UINT8(7, r);
    TEST_ASSERT_EQUAL_UINT8(10, a);
    TEST_ASSERT_NULL(SDL_GetRenderTarget(renderer));
    SDL_Surface *s = SDL_RenderReadPixels(renderer, NULL);
    TEST_ASSERT_NOT_NULL(s);
    for (int x = 1022; x <= 1026; x++) {
        TEST_ASSERT_TRUE(SDL_ReadSurfacePixel(s, x, 100, &r, &g, &b, &a));
        TEST_ASSERT_EQUAL_UINT8(255, r);
    }
    TEST_ASSERT_TRUE(SDL_ReadSurfacePixel(s, 1, 1, &r, &g, &b, &a));
    TEST_ASSERT_EQUAL_UINT8(0, r);
    SDL_DestroySurface(s);
}

static void test_transparent_export_keeps_soft_edges_and_straight_alpha(void) {
    VectorExportPlan plan = {.scope = VECTOR_EXPORT_ARTBOARD,
                             .count = 1,
                             .items = {{.board = 1, .bounds = {0, 0, 240, 200}}}};
    SDL_Surface *s = vector_export_render(document, &plan, 0, renderer, NULL, NULL, true);
    TEST_ASSERT_NOT_NULL(s);
    TEST_ASSERT_TRUE(partial_pixels(s, true) > 50);
    int edges = 0;
    for (int y = 10; y < 180; y++) {
        for (int x = 10; x < 220; x++) {
            Uint8 r, g, b, a;
            TEST_ASSERT_TRUE(SDL_ReadSurfacePixel(s, x, y, &r, &g, &b, &a));
            if (a > 0 && a < 255) {
                edges++;
                TEST_ASSERT_UINT8_WITHIN(3, 255, r);
                TEST_ASSERT_UINT8_WITHIN(3, 255, g);
                TEST_ASSERT_UINT8_WITHIN(3, 255, b);
            }
        }
    }
    TEST_ASSERT_TRUE(edges > 50);
    SDL_DestroySurface(s);
}

static void test_live_grid_snap_uses_screen_zoom_instead_of_sampling_zoom(void) {
    TEST_ASSERT_TRUE(vector_rectangle_set_angle(document, 1, 0));
    TEST_ASSERT_TRUE(vector_rectangle_update(document, 1, "Snapping", (PUIRect){50, 45, 96, 80}));
    vector_document_select(document, 1);
    document->grid.snap_enabled = true;
    document->selection_tool.dragging = true;
    document->selection_tool.rectangle_id = 1;
    document->selection_tool.offset_x = -15;
    clear(255);
    vector_artwork_draw(&artwork, document, &document->artboards[0], renderer,
                        (SDL_FRect){0, 0, 1200, 240}, NULL, NULL);
    SDL_Surface *s = SDL_RenderReadPixels(renderer, NULL);
    TEST_ASSERT_NOT_NULL(s);
    Uint8 r, g, b, a;
    TEST_ASSERT_TRUE(SDL_ReadSurfacePixel(s, 34, 90, &r, &g, &b, &a));
    TEST_ASSERT_EQUAL_UINT8(255, r);
    TEST_ASSERT_TRUE(document->selection_tool.dragging);
    TEST_ASSERT_EQUAL_FLOAT(1, document->viewport.zoom);
    SDL_DestroySurface(s);
}

static void test_cached_artwork_invalidates_on_edits_and_undo(void) {
    for (int pass = 0; pass < 3; pass++) {
        if (pass == 1) {
            TEST_ASSERT_TRUE(vector_rectangle_set_fill(document, 1, 0xff0000ff));
        } else if (pass == 2) {
            TEST_ASSERT_TRUE(vector_history_undo(&document->history, document));
        }
        for (int frame = 0; frame < 2; frame++) {
            clear(0);
            vector_artwork_draw(&artwork, document, &document->artboards[0], renderer,
                                (SDL_FRect){0, 0, 1200, 240}, NULL, NULL);
            SDL_Surface *surface = SDL_RenderReadPixels(renderer, NULL);
            TEST_ASSERT_NOT_NULL(surface);
            Uint8 r, g, b, a;
            TEST_ASSERT_TRUE(SDL_ReadSurfacePixel(surface, 100, 80, &r, &g, &b, &a));
            TEST_ASSERT_EQUAL_UINT8(255, r);
            TEST_ASSERT_EQUAL_UINT8(pass == 1 ? 0 : 255, g);
            SDL_DestroySurface(surface);
        }
    }
}

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_cached_artwork_invalidates_on_edits_and_undo);
    RUN_TEST(test_live_grid_snap_uses_screen_zoom_instead_of_sampling_zoom);
    RUN_TEST(test_rotated_edges_have_coverage_and_cache_is_reused_without_model_edits);
    RUN_TEST(test_tile_boundary_and_renderer_state_are_preserved);
    RUN_TEST(test_transparent_export_keeps_soft_edges_and_straight_alpha);
    return UNITY_END();
}
