/** @file @brief Standalone pengDraw pixels, replay, masking and resource-state contracts. */
#include "pengdraw/pengdraw.h"
#include "unity.h"
static SDL_Window *window;
static SDL_Renderer *renderer;
static PDrawRenderer cache;
static int calls;
static SDL_Texture *paint_target;

void setUp(void) {
    TEST_ASSERT_TRUE(SDL_Init(SDL_INIT_VIDEO));
    window = SDL_CreateWindow("pengDraw only", 160, 120, SDL_WINDOW_HIDDEN);
    TEST_ASSERT_NOT_NULL(window);
    renderer = SDL_CreateRenderer(window, NULL);
    TEST_ASSERT_NOT_NULL(renderer);
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
    SDL_RenderClear(renderer);
    calls = 0;
}

void tearDown(void) {
    pdraw_renderer_destroy(&cache);
    pdraw_renderer_destroy(&cache);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
}

static uint32_t pixel(int x, int y) {
    SDL_Surface *s = SDL_RenderReadPixels(renderer, NULL);
    TEST_ASSERT_NOT_NULL(s);
    Uint8 r, g, b, a;
    TEST_ASSERT_TRUE(SDL_ReadSurfacePixel(s, x, y, &r, &g, &b, &a));
    SDL_DestroySurface(s);
    return ((uint32_t)r << 24) | ((uint32_t)g << 16) | ((uint32_t)b << 8) | a;
}

static void paint(void *context, SDL_Renderer *r, float x, float y, float scale) {
    const PDrawShape *shape = context;
    calls++;
    paint_target = SDL_GetRenderTarget(r);
    pdraw_shape_draw(r, shape, (PDrawRect){20, 20, 80, 60}, 0, (SDL_FRect){x, y, 0, 0}, scale,
                     0x336699ff);
}

static void test_standalone_supersampling_and_clipping(void) {
    PDrawShape shape = {.ellipse = true};
    SDL_Rect clip = {30, 0, 130, 120};
    SDL_SetRenderClipRect(renderer, &clip);
    SDL_SetRenderDrawColor(renderer, 7, 8, 9, 10);
    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_ADD);
    pdraw_render(&cache, renderer, (SDL_FRect){0, 0, 160, 120}, paint, &shape);
    TEST_ASSERT_TRUE(calls > 0);
    TEST_ASSERT_NOT_NULL(cache.target);
    TEST_ASSERT_EQUAL_HEX32(0x336699ff, pixel(60, 50));
    TEST_ASSERT_EQUAL_HEX32(0x000000ff, pixel(25, 50));
    TEST_ASSERT_NULL(SDL_GetRenderTarget(renderer));
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
}

static void test_borrowed_texture_modulation_is_restored(void) {
    SDL_Surface *s = SDL_CreateSurface(2, 2, SDL_PIXELFORMAT_RGBA32);
    TEST_ASSERT_NOT_NULL(s);
    SDL_FillSurfaceRect(s, NULL, SDL_MapSurfaceRGBA(s, 255, 255, 255, 255));
    SDL_Texture *texture = SDL_CreateTextureFromSurface(renderer, s);
    SDL_DestroySurface(s);
    TEST_ASSERT_NOT_NULL(texture);
    SDL_SetTextureColorMod(texture, 11, 22, 33);
    SDL_SetTextureAlphaMod(texture, 44);
    SDL_FRect destination = {10, 10, 30, 30};
    TEST_ASSERT_TRUE(
        pdraw_texture_draw(renderer, texture, NULL, &destination, 0, SDL_FLIP_NONE, 0xff0000ff));
    TEST_ASSERT_EQUAL_HEX32(0xff0000ff, pixel(20, 20));
    Uint8 r, g, b, a;
    SDL_GetTextureColorMod(texture, &r, &g, &b);
    SDL_GetTextureAlphaMod(texture, &a);
    TEST_ASSERT_EQUAL_UINT8(11, r);
    TEST_ASSERT_EQUAL_UINT8(22, g);
    TEST_ASSERT_EQUAL_UINT8(33, b);
    TEST_ASSERT_EQUAL_UINT8(44, a);
    SDL_DestroyTexture(texture);
}

static void test_cached_tiles_reuse_and_invalidate_content(void) {
    PDrawShape shape = {0};
    SDL_FRect bounds = {0, 0, 160, 120};
    pdraw_render_cached(&cache, renderer, bounds, 1, paint, &shape);
    TEST_ASSERT_EQUAL_INT(1, calls);
    TEST_ASSERT_EQUAL_HEX32(0x336699ff, pixel(25, 25));
    SDL_RenderClear(renderer);
    pdraw_render_cached(&cache, renderer, bounds, 1, paint, &shape);
    TEST_ASSERT_EQUAL_INT(1, calls);
    TEST_ASSERT_EQUAL_HEX32(0x336699ff, pixel(25, 25));
    shape.ellipse = true;
    SDL_RenderClear(renderer);
    pdraw_render_cached(&cache, renderer, bounds, 2, paint, &shape);
    TEST_ASSERT_EQUAL_INT(2, calls);
    TEST_ASSERT_EQUAL_HEX32(0x000000ff, pixel(25, 25));
    TEST_ASSERT_EQUAL_HEX32(0x336699ff, pixel(60, 50));
}

static void test_cached_tiles_include_clip_target_and_restore_state(void) {
    PDrawShape shape = {0};
    SDL_FRect bounds = {0, 0, 160, 120};
    pdraw_render_cached(&cache, renderer, bounds, 1, paint, &shape);
    SDL_Rect clip = {30, 0, 130, 120};
    SDL_SetRenderClipRect(renderer, &clip);
    SDL_SetRenderDrawColor(renderer, 7, 8, 9, 10);
    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_ADD);
    pdraw_render_cached(&cache, renderer, bounds, 1, paint, &shape);
    TEST_ASSERT_EQUAL_INT(2, calls);
    pdraw_render_cached(&cache, renderer, bounds, 1, paint, &shape);
    TEST_ASSERT_EQUAL_INT(2, calls);
    SDL_Rect restored;
    SDL_GetRenderClipRect(renderer, &restored);
    TEST_ASSERT_EQUAL_MEMORY(&clip, &restored, sizeof clip);
    Uint8 r, g, b, a;
    SDL_GetRenderDrawColor(renderer, &r, &g, &b, &a);
    TEST_ASSERT_EQUAL_UINT8(7, r);
    TEST_ASSERT_EQUAL_UINT8(10, a);
    SDL_BlendMode blend;
    SDL_GetRenderDrawBlendMode(renderer, &blend);
    TEST_ASSERT_EQUAL_INT(SDL_BLENDMODE_ADD, blend);
    SDL_Texture *target =
        SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGBA8888, SDL_TEXTUREACCESS_TARGET, 160, 120);
    TEST_ASSERT_NOT_NULL(target);
    SDL_SetRenderTarget(renderer, target);
    pdraw_render_cached(&cache, renderer, bounds, 1, paint, &shape);
    TEST_ASSERT_EQUAL_INT(3, calls);
    TEST_ASSERT_EQUAL_PTR(target, SDL_GetRenderTarget(renderer));
    SDL_SetRenderTarget(renderer, NULL);
    pdraw_renderer_destroy(&cache);
    SDL_DestroyTexture(target);
}

static void test_cached_tiles_evict_old_revisions(void) {
    PDrawShape shape = {0};
    SDL_FRect bounds = {0, 0, 160, 120};
    for (uint64_t revision = 0; revision < 40; revision++) {
        pdraw_render_cached(&cache, renderer, bounds, revision, paint, &shape);
    }
    TEST_ASSERT_EQUAL_INT(40, calls);
    pdraw_render_cached(&cache, renderer, bounds, 39, paint, &shape);
    TEST_ASSERT_EQUAL_INT(40, calls);
    pdraw_render_cached(&cache, renderer, bounds, 0, paint, &shape);
    TEST_ASSERT_EQUAL_INT(41, calls);
}

static void test_content_changes_repaint_the_same_texture(void) {
    PDrawShape shape = {0};
    SDL_FRect bounds = {0, 0, 160, 120};
    pdraw_render_cached(&cache, renderer, bounds, 0, paint, &shape);
    SDL_Texture *first = paint_target;
    TEST_ASSERT_NOT_NULL(first);
    for (uint64_t revision = 1; revision <= 200; revision++) {
        shape.ellipse = (revision % 2) != 0;
        pdraw_render_cached(&cache, renderer, bounds, revision, paint, &shape);
        TEST_ASSERT_EQUAL_PTR(first, paint_target);
    }
    TEST_ASSERT_EQUAL_INT(201, calls);
    TEST_ASSERT_EQUAL_HEX32(0x336699ff, pixel(25, 25));
    shape.ellipse = true;
    SDL_RenderClear(renderer);
    pdraw_render_cached(&cache, renderer, bounds, 201, paint, &shape);
    TEST_ASSERT_EQUAL_HEX32(0x000000ff, pixel(25, 25));
    TEST_ASSERT_EQUAL_HEX32(0x336699ff, pixel(60, 50));
    TEST_ASSERT_EQUAL_PTR(first, paint_target);
}

static void test_cached_tiles_enforce_byte_budget_before_slot_limit(void) {
    PDrawShape shape = {0};
    SDL_Texture *target =
        SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGBA8888, SDL_TEXTUREACCESS_TARGET, 1024, 1024);
    TEST_ASSERT_NOT_NULL(target);
    SDL_SetRenderTarget(renderer, target);
    SDL_FRect bounds = {0, 0, 1024, 1024};
    /* Five different view bounds each need one supersampled 16 MiB tile: five cannot fit in the 64
     * MiB budget, even though the cache has sixteen slots. */
    for (uint64_t revision = 1; revision <= 5; revision++) {
        bounds.x = -(float)revision;
        bounds.w = 1024 + (float)revision;
        pdraw_render_cached(&cache, renderer, bounds, revision, paint, &shape);
    }
    TEST_ASSERT_EQUAL_INT(5, calls);
    TEST_ASSERT_EQUAL_UINT(4, pdraw_renderer_cache_stats(&cache).tiles);
    TEST_ASSERT_EQUAL_UINT64(1, pdraw_renderer_cache_stats(&cache).evictions);
    TEST_ASSERT_EQUAL_UINT64(64u * 1024u * 1024u,
                             pdraw_renderer_resource_stats(renderer).live_bytes);
    TEST_ASSERT_EQUAL_UINT64(64u * 1024u * 1024u,
                             pdraw_renderer_resource_stats(renderer).peak_bytes);
    pdraw_render_cached(&cache, renderer, bounds, 5, paint, &shape);
    TEST_ASSERT_EQUAL_INT(5, calls);
    bounds.x = -1;
    bounds.w = 1025;
    pdraw_render_cached(&cache, renderer, bounds, 1, paint, &shape);
    TEST_ASSERT_EQUAL_INT(6, calls);
    pdraw_renderer_destroy(&cache);
    SDL_SetRenderTarget(renderer, NULL);
    SDL_DestroyTexture(target);
}

static void test_resource_accounting_and_cache_statistics_survive_repaints_and_release(void) {
    TEST_ASSERT_FALSE(pdraw_renderer_resource_stats(renderer).available);
    TEST_ASSERT_FALSE(pdraw_renderer_resource_stats(NULL).available);
    PDrawShape shape = {0};
    SDL_FRect bounds = {0, 0, 160, 120};
    pdraw_render_cached(&cache, renderer, bounds, 0, paint, &shape);
    for (uint64_t revision = 1; revision <= 20; revision++) {
        pdraw_render_cached(&cache, renderer, bounds, revision, paint, &shape);
    }
    pdraw_render_cached(&cache, renderer, bounds, 20, paint, &shape);
    PDrawResourceStats resources = pdraw_renderer_resource_stats(renderer);
    PDrawCacheStats cached = pdraw_renderer_cache_stats(&cache);
    TEST_ASSERT_TRUE(resources.available);
    TEST_ASSERT_TRUE(resources.complete);
    TEST_ASSERT_EQUAL_UINT64(1, resources.textures_created);
    TEST_ASSERT_EQUAL_UINT64(0, resources.textures_released);
    TEST_ASSERT_EQUAL_UINT64(1, resources.live_textures);
    TEST_ASSERT_EQUAL_UINT64(320 * 240 * 4, resources.live_bytes);
    TEST_ASSERT_EQUAL_UINT64(resources.live_bytes, resources.artwork_bytes);
    TEST_ASSERT_EQUAL_UINT64(resources.live_bytes, cached.tile_bytes);
    TEST_ASSERT_EQUAL_UINT(1, cached.tiles);
    TEST_ASSERT_EQUAL_UINT64(1, cached.misses);
    TEST_ASSERT_EQUAL_UINT64(20, cached.repaints);
    TEST_ASSERT_EQUAL_UINT64(1, cached.hits);
    /* Unrelated UI/app textures must not be reported as pengDraw allocations. */
    SDL_Texture *external =
        SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGBA8888, SDL_TEXTUREACCESS_TARGET, 16, 16);
    TEST_ASSERT_NOT_NULL(external);
    TEST_ASSERT_EQUAL_UINT64(1, pdraw_renderer_resource_stats(renderer).textures_created);
    SDL_DestroyTexture(external);
    pdraw_renderer_destroy(&cache);
    resources = pdraw_renderer_resource_stats(renderer);
    TEST_ASSERT_EQUAL_UINT64(0, resources.live_textures);
    TEST_ASSERT_EQUAL_UINT64(0, resources.live_bytes);
    TEST_ASSERT_EQUAL_UINT64(resources.textures_created, resources.textures_released);
    TEST_ASSERT_EQUAL_UINT64(320 * 240 * 4, resources.peak_bytes);
    TEST_ASSERT_EQUAL_UINT(0, pdraw_renderer_cache_stats(&cache).tiles);
    TEST_ASSERT_EQUAL_UINT64(0, pdraw_renderer_cache_stats(NULL).tile_bytes);
}

static void fallback_silhouette(void *context, float dx, float dy) {
    SDL_Renderer *r = context;
    SDL_SetRenderDrawColor(r, 255, 255, 255, 255);
    SDL_RenderFillRect(r, &(SDL_FRect){20 + dx, 20 + dy, 40, 30});
}

static void test_cpu_fallbacks_count_and_release_temporary_textures(void) {
    PDrawShadow shadow = pdraw_shadow_default();
    shadow.enabled = true;
    TEST_ASSERT_TRUE(pdraw_shadow_draw(renderer, &shadow, 1, (PDrawRect){20, 20, 40, 30},
                                       fallback_silhouette, renderer));
    PDrawShape shape = {0};
    shape.gradient = pdraw_gradient_default(0xff0000ff);
    shape.gradient.kind = PDRAW_GRADIENT_LINEAR;
    TEST_ASSERT_TRUE(
        pdraw_gradient_draw(renderer, &shape, (PDrawRect){20, 20, 40, 30}, 0, (SDL_FRect){0}, 1));
    PDrawResourceStats resources = pdraw_renderer_resource_stats(renderer);
    TEST_ASSERT_TRUE(resources.complete);
    TEST_ASSERT_EQUAL_UINT64(1, resources.shadow_fallback_attempts);
    TEST_ASSERT_EQUAL_UINT64(1, resources.gradient_fallback_attempts);
    TEST_ASSERT_EQUAL_UINT64(3, resources.textures_created);
    TEST_ASSERT_EQUAL_UINT64(3, resources.textures_released);
    TEST_ASSERT_EQUAL_UINT64(0, resources.live_bytes);
    TEST_ASSERT_EQUAL_UINT64(0, resources.shadow_bytes);
    TEST_ASSERT_EQUAL_UINT64(0, resources.gradient_bytes);
    TEST_ASSERT_EQUAL_UINT64(0, resources.texture_failures);
    TEST_ASSERT_GREATER_THAN_UINT64(0, resources.peak_bytes);
}

static void paint_mask_content(void *context) {
    SDL_Renderer *r = context;
    SDL_SetRenderDrawColor(r, 255, 255, 255, 255);
    SDL_RenderFillRect(r, &(SDL_FRect){0, 0, 160, 120});
}

static void test_mask_texture_bytes_are_separate_and_released(void) {
    PDrawShape shape = {0};
    PDrawMaskRenderer masks = {.renderer = renderer};
    pdraw_mask_render(&masks, &shape, (PDrawRect){10, 10, 60, 40}, 0, (SDL_FRect){0}, 1,
                      paint_mask_content, renderer);
    PDrawResourceStats resources = pdraw_renderer_resource_stats(renderer);
    TEST_ASSERT_EQUAL_UINT64(2, resources.live_textures);
    TEST_ASSERT_EQUAL_UINT64(160 * 120 * 4 * 2, resources.mask_bytes);
    TEST_ASSERT_EQUAL_UINT64(resources.mask_bytes, resources.live_bytes);
    pdraw_mask_renderer_destroy(&masks);
    resources = pdraw_renderer_resource_stats(renderer);
    TEST_ASSERT_EQUAL_UINT64(2, resources.textures_released);
    TEST_ASSERT_EQUAL_UINT64(0, resources.live_textures);
    TEST_ASSERT_EQUAL_UINT64(0, resources.mask_bytes);
}

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_mask_texture_bytes_are_separate_and_released);
    RUN_TEST(test_resource_accounting_and_cache_statistics_survive_repaints_and_release);
    RUN_TEST(test_cpu_fallbacks_count_and_release_temporary_textures);
    RUN_TEST(test_content_changes_repaint_the_same_texture);
    RUN_TEST(test_cached_tiles_enforce_byte_budget_before_slot_limit);
    RUN_TEST(test_cached_tiles_reuse_and_invalidate_content);
    RUN_TEST(test_cached_tiles_include_clip_target_and_restore_state);
    RUN_TEST(test_cached_tiles_evict_old_revisions);
    RUN_TEST(test_standalone_supersampling_and_clipping);
    RUN_TEST(test_borrowed_texture_modulation_is_restored);
    return UNITY_END();
}
