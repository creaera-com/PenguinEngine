/** @file @brief Real GPU API pixel tests; Vulkan software devices are valid for correctness. */
#include "pengdraw/pengdraw.h"
#include "unity.h"
#include <stdio.h>
#include <stdlib.h>
static SDL_Renderer *renderer;
static SDL_Texture *target;
static int paints;

void setUp(void) {
    TEST_ASSERT_TRUE(SDL_Init(SDL_INIT_VIDEO));
    SDL_SetHint(SDL_HINT_RENDER_DRIVER, "gpu");
    renderer = pdraw_create_renderer(NULL);
    if (!renderer) {
        if (getenv("PDRAW_REQUIRE_GPU")) {
            TEST_FAIL_MESSAGE(SDL_GetError());
        }
        TEST_IGNORE_MESSAGE("GPU device unavailable; set PDRAW_REQUIRE_GPU=1 to require this test");
    }
    TEST_ASSERT_TRUE(pdraw_renderer_gpu_effects(renderer));
    target =
        SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGBA8888, SDL_TEXTUREACCESS_TARGET, 160, 120);
    TEST_ASSERT_NOT_NULL(target);
    TEST_ASSERT_TRUE(SDL_SetRenderTarget(renderer, target));
    SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
    SDL_RenderClear(renderer);
    paints = 0;
}

void tearDown(void) {
    if (renderer) {
        SDL_SetRenderTarget(renderer, NULL);
    }
    SDL_DestroyTexture(target);
    target = NULL;
    pdraw_destroy_renderer(renderer);
    renderer = NULL;
    SDL_ResetHint(SDL_HINT_RENDER_DRIVER);
    SDL_Quit();
}

static void silhouette(void *context, float dx, float dy) {
    paints++;
    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
    SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
    SDL_FRect base = context ? *(SDL_FRect *)context : (SDL_FRect){20, 30, 40, 30};
    SDL_FRect rect = {base.x + dx, base.y + dy, base.w, base.h};
    SDL_RenderFillRect(renderer, &rect);
}

static Uint8 red(int x, int y) {
    SDL_Surface *pixels = SDL_RenderReadPixels(renderer, NULL);
    TEST_ASSERT_NOT_NULL(pixels);
    Uint8 r, g, b, a;
    TEST_ASSERT_TRUE(SDL_ReadSurfacePixel(pixels, x, y, &r, &g, &b, &a));
    SDL_DestroySurface(pixels);
    return r;
}

static void test_gpu_shadow_silhouette_blur_spread_and_state(void) {
    PDrawShadow s = {.enabled = 1, .color = 0x000000ff, .offset_x = 70};
    SDL_Rect clip = {0, 0, 150, 110};
    SDL_SetRenderClipRect(renderer, &clip);
    SDL_SetRenderDrawColor(renderer, 7, 8, 9, 10);
    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_ADD);
    TEST_ASSERT_TRUE(
        pdraw_shadow_draw(renderer, &s, 1, (PDrawRect){20, 30, 40, 30}, silhouette, NULL));
    TEST_ASSERT_EQUAL_UINT8(0, red(100, 40));
    TEST_ASSERT_EQUAL_UINT8(255, red(80, 40));
    TEST_ASSERT_EQUAL_UINT64(1, pdraw_renderer_gpu_stats(renderer).shadow_draws);
    TEST_ASSERT_EQUAL_PTR(target, SDL_GetRenderTarget(renderer));
    SDL_Rect after;
    SDL_GetRenderClipRect(renderer, &after);
    TEST_ASSERT_EQUAL_MEMORY(&clip, &after, sizeof clip);
    Uint8 r, g, b, a;
    SDL_GetRenderDrawColor(renderer, &r, &g, &b, &a);
    TEST_ASSERT_EQUAL_UINT8(7, r);
    TEST_ASSERT_EQUAL_UINT8(10, a);
    SDL_BlendMode blend;
    SDL_GetRenderDrawBlendMode(renderer, &blend);
    TEST_ASSERT_EQUAL_INT(SDL_BLENDMODE_ADD, blend);
    SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
    SDL_RenderClear(renderer);
    s.blur = 9;
    TEST_ASSERT_TRUE(
        pdraw_shadow_draw(renderer, &s, 1, (PDrawRect){20, 30, 40, 30}, silhouette, NULL));
    TEST_ASSERT_GREATER_THAN(0, red(89, 40));
    TEST_ASSERT_LESS_THAN(255, red(89, 40));
    SDL_RenderClear(renderer);
    s.blur = 0;
    s.spread = 5;
    TEST_ASSERT_TRUE(
        pdraw_shadow_draw(renderer, &s, 1, (PDrawRect){20, 30, 40, 30}, silhouette, NULL));
    TEST_ASSERT_EQUAL_UINT8(0, red(87, 40));
    TEST_ASSERT_EQUAL_UINT64(3, pdraw_renderer_gpu_stats(renderer).shadow_draws);
    TEST_ASSERT_EQUAL_UINT64(0, pdraw_renderer_gpu_stats(renderer).shadow_failures);
}

static void paint_artwork(void *context, SDL_Renderer *r, float x, float y, float scale) {
    (void)context;
    TEST_ASSERT_EQUAL_PTR(renderer, r);
    PDrawShadow s = pdraw_shadow_default();
    SDL_FRect shape = {x + 20 * scale, y + 30 * scale, 40 * scale, 30 * scale};
    s.offset_x = 70;
    TEST_ASSERT_TRUE(pdraw_shadow_draw(
        r, &s, scale, (PDrawRect){shape.x, shape.y, shape.w, shape.h}, silhouette, &shape));
}

static void test_gpu_effects_reuse_textures_and_artwork_cache(void) {
    PDrawRenderer cache = {0};
    for (int i = 0; i < 4; i++) {
        pdraw_render_cached(&cache, renderer, (SDL_FRect){0, 0, 160, 120}, 1, paint_artwork, NULL);
    }
    TEST_ASSERT_EQUAL_INT(1, paints);
    TEST_ASSERT_EQUAL_UINT64(1, pdraw_renderer_gpu_stats(renderer).shadow_draws);
    TEST_ASSERT_GREATER_THAN(0, pdraw_renderer_gpu_stats(renderer).texture_bytes);
    pdraw_render_cached(&cache, renderer, (SDL_FRect){0, 0, 160, 120}, 2, paint_artwork, NULL);
    TEST_ASSERT_EQUAL_INT(2, paints);
    TEST_ASSERT_EQUAL_UINT64(0, pdraw_renderer_gpu_stats(renderer).shadow_failures);
    pdraw_renderer_destroy(&cache);
}

static void test_gpu_gradients_match_value_sampler_for_all_types_and_alpha(void) {
    PDrawShape shape = {0};
    shape.gradient = pdraw_gradient_default(0xff0000ff);
    shape.gradient.count = 3;
    shape.gradient.stops[1] = (PDrawGradientStop){.4f, 0x00ff0080};
    shape.gradient.stops[2] = (PDrawGradientStop){1, 0x0000ffff};
    const int positions[][2] = {{30, 30}, {70, 50}, {110, 90}};
    for (int kind = PDRAW_GRADIENT_LINEAR; kind <= PDRAW_GRADIENT_REFLECTED; kind++) {
        shape.gradient.kind = kind;
        SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
        SDL_RenderClear(renderer);
        TEST_ASSERT_TRUE(pdraw_gradient_draw(renderer, &shape, (PDrawRect){20, 20, 100, 80}, 0,
                                             (SDL_FRect){0}, 1));
        SDL_Surface *pixels = SDL_RenderReadPixels(renderer, NULL);
        TEST_ASSERT_NOT_NULL(pixels);
        for (unsigned i = 0; i < sizeof positions / sizeof *positions; i++) {
            int x = positions[i][0], y = positions[i][1];
            uint32_t expected =
                pdraw_gradient_sample(&shape.gradient, x + .5f - 20, y + .5f - 20, 100, 80);
            Uint8 r, g, b, a;
            TEST_ASSERT_TRUE(SDL_ReadSurfacePixel(pixels, x, y, &r, &g, &b, &a));
            float alpha = (expected & 255) / 255.f;
            TEST_ASSERT_UINT8_WITHIN(3, (expected >> 24) * alpha + 255 * (1 - alpha), r);
            TEST_ASSERT_UINT8_WITHIN(3, ((expected >> 16) & 255) * alpha + 255 * (1 - alpha), g);
            TEST_ASSERT_UINT8_WITHIN(3, ((expected >> 8) & 255) * alpha + 255 * (1 - alpha), b);
        }
        SDL_DestroySurface(pixels);
    }
    TEST_ASSERT_EQUAL_UINT64(4, pdraw_renderer_gpu_stats(renderer).gradient_draws);
    TEST_ASSERT_EQUAL_UINT64(0, pdraw_renderer_gpu_stats(renderer).gradient_failures);
}

static void test_gpu_shadow_targets_keep_capacity_for_smaller_objects(void) {
    PDrawShadow shadow = pdraw_shadow_default();
    TEST_ASSERT_TRUE(
        pdraw_shadow_draw(renderer, &shadow, 1, (PDrawRect){20, 30, 40, 30}, silhouette, NULL));
    size_t initial = pdraw_renderer_gpu_stats(renderer).texture_bytes;
    TEST_ASSERT_TRUE(
        pdraw_shadow_draw(renderer, &shadow, 1, (PDrawRect){20, 30, 80, 50}, silhouette, NULL));
    size_t grown = pdraw_renderer_gpu_stats(renderer).texture_bytes;
    TEST_ASSERT_TRUE(grown > initial);
    TEST_ASSERT_TRUE(
        pdraw_shadow_draw(renderer, &shadow, 1, (PDrawRect){20, 30, 40, 30}, silhouette, NULL));
    TEST_ASSERT_EQUAL_UINT64(grown, pdraw_renderer_gpu_stats(renderer).texture_bytes);
    TEST_ASSERT_EQUAL_UINT64(3, pdraw_renderer_gpu_stats(renderer).shadow_draws);
    TEST_ASSERT_EQUAL_UINT64(0, pdraw_renderer_gpu_stats(renderer).shadow_failures);
}

static void test_gpu_resource_accounting_reuses_and_releases_shadow_targets(void) {
    PDrawResourceStats initial = pdraw_renderer_resource_stats(renderer);
    TEST_ASSERT_TRUE(initial.available);
    TEST_ASSERT_TRUE(initial.complete);
    TEST_ASSERT_EQUAL_UINT64(1, initial.textures_created); /* Shared white gradient texel. */
    TEST_ASSERT_EQUAL_UINT64(4, initial.auxiliary_bytes);
    PDrawShadow shadow = pdraw_shadow_default();
    shadow.enabled = true;
    for (int i = 0; i < 20; i++) {
        shadow.offset_x = (float)i;
        TEST_ASSERT_TRUE(
            pdraw_shadow_draw(renderer, &shadow, 1, (PDrawRect){20, 30, 40, 30}, silhouette, NULL));
    }
    PDrawResourceStats resources = pdraw_renderer_resource_stats(renderer);
    PDrawGPUStats effects = pdraw_renderer_gpu_stats(renderer);
    TEST_ASSERT_EQUAL_UINT64(4, resources.textures_created);
    TEST_ASSERT_EQUAL_UINT64(4, resources.live_textures);
    TEST_ASSERT_EQUAL_UINT64(0, resources.textures_released);
    TEST_ASSERT_EQUAL_UINT64(0, resources.shadow_fallback_attempts);
    TEST_ASSERT_EQUAL_UINT64(0, resources.texture_failures);
    TEST_ASSERT_EQUAL_UINT(3, effects.shadow_targets);
    TEST_ASSERT_EQUAL_UINT64((uint64_t)effects.shadow_width * effects.shadow_height * 4 * 3,
                             resources.shadow_bytes);
    TEST_ASSERT_EQUAL_UINT64(effects.texture_bytes, resources.shadow_bytes);
    SDL_FRect bigger = {0, 0, 120, 100};
    TEST_ASSERT_TRUE(
        pdraw_shadow_draw(renderer, &shadow, 1, (PDrawRect){0, 0, 120, 100}, silhouette, &bigger));
    resources = pdraw_renderer_resource_stats(renderer);
    TEST_ASSERT_EQUAL_UINT64(7, resources.textures_created);
    TEST_ASSERT_EQUAL_UINT64(3, resources.textures_released);
    TEST_ASSERT_EQUAL_UINT64(4, resources.live_textures);
    TEST_ASSERT_EQUAL_UINT64(resources.shadow_bytes + 4, resources.live_bytes);
}

static void test_instrumented_vulkan_counts_native_pool_work(void) {
    SDL_GPUDevice *device = SDL_GetPointerProperty(SDL_GetRendererProperties(renderer),
                                                   SDL_PROP_RENDERER_GPU_DEVICE_POINTER, NULL);
    SDL_PropertiesID props = SDL_GetGPUDeviceProperties(device);
    if (!SDL_GetBooleanProperty(props, "penguengine.vulkan.available", false)) {
        if (getenv("PUI_REQUIRE_VULKAN_DIAGNOSTICS")) {
            TEST_FAIL_MESSAGE("Instrumented Vulkan backend required");
        }
        TEST_IGNORE_MESSAGE("Native Vulkan diagnostics disabled");
    }
    PDrawShadow shadow = {.enabled = 1, .color = 0x000000ff, .blur = 4};
    for (int frame = 0; frame < 4; frame++) {
        TEST_ASSERT_TRUE(
            pdraw_shadow_draw(renderer, &shadow, 1, (PDrawRect){20, 30, 40, 30}, silhouette, NULL));
        TEST_ASSERT_TRUE(SDL_RenderPresent(renderer));
        TEST_ASSERT_TRUE(SDL_WaitForGPUIdle(device));
    }
    Sint64 pools = SDL_GetNumberProperty(props, "penguengine.vulkan.descriptor_pools_created", 0);
    Sint64 sets = SDL_GetNumberProperty(props, "penguengine.vulkan.descriptor_sets_allocated", 0);
    TEST_ASSERT_GREATER_THAN_INT64(0, pools);
    TEST_ASSERT_EQUAL_INT64(pools * 128, sets);
    TEST_ASSERT_GREATER_THAN_INT64(
        0, SDL_GetNumberProperty(props, "penguengine.vulkan.descriptor_sets_fetched", 0));
    TEST_ASSERT_GREATER_THAN_INT64(
        0, SDL_GetNumberProperty(props, "penguengine.vulkan.descriptor_caches_returned", 0));
    TEST_ASSERT_GREATER_THAN_INT64(
        0, SDL_GetNumberProperty(props, "penguengine.vulkan.descriptor_caches_reused", 0));
    TEST_ASSERT_GREATER_THAN_INT64(
        0, SDL_GetNumberProperty(props, "penguengine.vulkan.memory_maps", 0));
    TEST_ASSERT_EQUAL_INT64(0, SDL_GetNumberProperty(props, "penguengine.vulkan.failures", 0));
}

static Sint64 native_counter(SDL_PropertiesID props, const char *group, unsigned index,
                             const char *field) {
    char key[128];
    snprintf(key, sizeof key, "penguengine.vulkan.%s.%u.%s", group, index, field);
    return SDL_GetNumberProperty(props, key, 0);
}

static void test_vulkan_memory_breakdown_matches_native_totals(void) {
    SDL_GPUDevice *device = SDL_GetPointerProperty(SDL_GetRendererProperties(renderer),
                                                   SDL_PROP_RENDERER_GPU_DEVICE_POINTER, NULL);
    SDL_PropertiesID props = SDL_GetGPUDeviceProperties(device);
    if (SDL_GetNumberProperty(props, "penguengine.vulkan.version", 0) < 2) {
        if (getenv("PUI_REQUIRE_VULKAN_DIAGNOSTICS")) {
            TEST_FAIL_MESSAGE("Version 2 Vulkan diagnostics required");
        }
        TEST_IGNORE_MESSAGE("Detailed native Vulkan diagnostics disabled");
    }
    PDrawShadow shadow = {.enabled = 1, .color = 0x000000ff, .blur = 4};
    TEST_ASSERT_TRUE(
        pdraw_shadow_draw(renderer, &shadow, 1, (PDrawRect){20, 30, 40, 30}, silhouette, NULL));
    TEST_ASSERT_TRUE(SDL_RenderPresent(renderer));
    TEST_ASSERT_TRUE(SDL_WaitForGPUIdle(device));
    const char *total_names[] = {"memory_allocations", "memory_frees", "memory_bytes_allocated",
                                 "memory_bytes_freed"};
    const char *detail_names[] = {"allocations", "frees", "allocated_bytes", "freed_bytes"};
    unsigned types =
        (unsigned)SDL_GetNumberProperty(props, "penguengine.vulkan.memory_type_count", 0);
    TEST_ASSERT_GREATER_THAN_UINT(0, types);
    TEST_ASSERT_LESS_OR_EQUAL_UINT(32, types);
    for (unsigned f = 0; f < 4; f++) {
        Sint64 classes_sum = 0, types_sum = 0;
        for (unsigned c = 0; c < 4; c++) {
            classes_sum += native_counter(props, "class", c, detail_names[f]);
        }
        for (unsigned t = 0; t < types; t++) {
            types_sum += native_counter(props, "memory_type", t, detail_names[f]);
        }
        char key[128];
        snprintf(key, sizeof key, "penguengine.vulkan.%s", total_names[f]);
        Sint64 total = SDL_GetNumberProperty(props, key, 0);
        TEST_ASSERT_EQUAL_INT64(total, classes_sum);
        TEST_ASSERT_EQUAL_INT64(total, types_sum);
    }
    TEST_ASSERT_GREATER_THAN_INT64(0, native_counter(props, "class", 1, "bindings"));
    TEST_ASSERT_GREATER_THAN_INT64(0, native_counter(props, "class", 3, "bindings"));
    for (unsigned c = 0; c < 4; c++) {
        TEST_ASSERT_GREATER_OR_EQUAL_INT64(native_counter(props, "class", c, "resource_bytes"),
                                           native_counter(props, "class", c, "requirement_bytes"));
    }
}

static void test_small_effect_uploads_do_not_cycle_large_artwork_buffers(void) {
    SDL_GPUDevice *device = SDL_GetPointerProperty(SDL_GetRendererProperties(renderer),
                                                   SDL_PROP_RENDERER_GPU_DEVICE_POINTER, NULL);
    SDL_PropertiesID props = SDL_GetGPUDeviceProperties(device);
    bool diagnostics = SDL_GetNumberProperty(props, "penguengine.vulkan.version", 0) >= 2;
    SDL_Vertex vertices[3] = {
        {{10, 10}, {1, 0, 0, 1}, {0, 0}},
        {{20, 10}, {1, 0, 0, 1}, {0, 0}},
        {{10, 20}, {1, 0, 0, 1}, {0, 0}},
    };
    const int indices_count = 8192 * 3;
    int *indices = SDL_malloc((size_t)indices_count * sizeof *indices);
    TEST_ASSERT_NOT_NULL(indices);
    for (int i = 0; i < indices_count; i++) {
        indices[i] = i % 3;
    }
    bool queued = SDL_RenderGeometry(renderer, NULL, vertices, 3, indices, indices_count);
    SDL_free(indices);
    TEST_ASSERT_TRUE(queued);
    TEST_ASSERT_TRUE(SDL_FlushRenderer(renderer));
    Sint64 before = native_counter(props, "class", 2, "allocated_bytes");
    PDrawShadow shadow = {.enabled = 1, .color = 0x000000ff, .offset_x = 70, .blur = 4};
    /* Intentionally no Present or GPU-idle wait between the large batch and
     * these tiny passes: this is the allocation pattern seen with many clones. */
    for (int i = 0; i < 24; i++) {
        TEST_ASSERT_TRUE(
            pdraw_shadow_draw(renderer, &shadow, 1, (PDrawRect){20, 30, 40, 30}, silhouette, NULL));
    }
    TEST_ASSERT_TRUE(SDL_FlushRenderer(renderer));
    if (diagnostics) {
        Sint64 after = native_counter(props, "class", 2, "allocated_bytes");
        SDL_Log("Large batch + 24 shadows: %llu transfer bytes allocated",
                (unsigned long long)(after - before));
        TEST_ASSERT_LESS_THAN_INT64_MESSAGE(
            4 * 1024 * 1024, after - before,
            "Tiny effect passes must not allocate a full large vertex staging buffer each");
    }
    TEST_ASSERT_EQUAL_UINT8(0, red(100, 40));
    TEST_ASSERT_EQUAL_UINT8(255, red(145, 110));
    TEST_ASSERT_TRUE(SDL_RenderPresent(renderer));
    TEST_ASSERT_TRUE(SDL_WaitForGPUIdle(device));
}

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_small_effect_uploads_do_not_cycle_large_artwork_buffers);
    RUN_TEST(test_vulkan_memory_breakdown_matches_native_totals);
    RUN_TEST(test_instrumented_vulkan_counts_native_pool_work);
    RUN_TEST(test_gpu_resource_accounting_reuses_and_releases_shadow_targets);
    RUN_TEST(test_gpu_shadow_targets_keep_capacity_for_smaller_objects);
    RUN_TEST(test_gpu_gradients_match_value_sampler_for_all_types_and_alpha);
    RUN_TEST(test_gpu_shadow_silhouette_blur_spread_and_state);
    RUN_TEST(test_gpu_effects_reuse_textures_and_artwork_cache);
    return UNITY_END();
}
