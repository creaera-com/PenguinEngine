/** @file @brief Explicit diagnostic GPU waits; no waiting in the default path. */
#include "monitor/monitor.h"
#include "pengdraw/pengdraw.h"
#include "unity.h"
#include <stdlib.h>
static SDL_Renderer *renderer;
static SDL_Texture *target;

void setUp(void) {
    TEST_ASSERT_TRUE(SDL_Init(SDL_INIT_VIDEO));
}

void tearDown(void) {
    if (renderer) {
        SDL_SetRenderTarget(renderer, NULL);
    }
    SDL_DestroyTexture(target);
    target = NULL;
    pdraw_destroy_renderer(renderer);
    renderer = NULL;
    SDL_Quit();
}

static void test_no_gpu_does_not_claim_a_wait_or_report_failure(void) {
    ProcessMonitor monitor = {.gpu_wait_requested = true};
    TEST_ASSERT_TRUE(process_monitor_gpu_wait(&monitor, NULL));
    TEST_ASSERT_FALSE(monitor.gpu_wait_available);
    TEST_ASSERT_FALSE(monitor.gpu_wait_active);
    TEST_ASSERT_EQUAL_UINT64(0, monitor.gpu_wait_calls);
    TEST_ASSERT_EQUAL_UINT64(0, monitor.gpu_wait_failures);
}

static void test_wait_is_opt_in_and_only_active_after_success(void) {
    renderer = pdraw_create_renderer(NULL);
    if (!renderer) {
        if (getenv("PDRAW_REQUIRE_GPU")) {
            TEST_FAIL_MESSAGE(SDL_GetError());
        }
        TEST_IGNORE_MESSAGE("GPU unavailable");
    }
    target =
        SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGBA8888, SDL_TEXTUREACCESS_TARGET, 64, 64);
    TEST_ASSERT_NOT_NULL(target);
    TEST_ASSERT_TRUE(SDL_SetRenderTarget(renderer, target));
    SDL_SetRenderDrawColor(renderer, 100, 120, 140, 255);
    SDL_RenderClear(renderer);
    TEST_ASSERT_TRUE(SDL_RenderPresent(renderer));
    ProcessMonitor monitor = {0};
    process_monitor_graphics(&monitor, renderer);
    SDL_GPUDevice *device = SDL_GetPointerProperty(SDL_GetRendererProperties(renderer),
                                                   SDL_PROP_RENDERER_GPU_DEVICE_POINTER, NULL);
    bool instrumented = SDL_GetBooleanProperty(SDL_GetGPUDeviceProperties(device),
                                               "penguengine.vulkan.available", false);
    TEST_ASSERT_EQUAL(instrumented, monitor.graphics.vulkan_available);
    if (getenv("PUI_REQUIRE_VULKAN_DIAGNOSTICS")) {
        TEST_ASSERT_TRUE_MESSAGE(instrumented, "Instrumented Vulkan backend required");
    }
    if (instrumented) {
        TEST_ASSERT_GREATER_THAN_UINT64(0, monitor.graphics.vulkan_counters[6]);
        TEST_ASSERT_GREATER_THAN_UINT64(0, monitor.graphics.vulkan_counters[9]);
        TEST_ASSERT_GREATER_THAN_UINT64(0, monitor.graphics.vulkan_counters[11]);
    }
    TEST_ASSERT_TRUE(process_monitor_gpu_wait(&monitor, renderer));
    TEST_ASSERT_TRUE(monitor.gpu_wait_available);
    TEST_ASSERT_FALSE(monitor.gpu_wait_active);
    TEST_ASSERT_EQUAL_UINT64(0, monitor.gpu_wait_calls);
    monitor.gpu_wait_requested = true;
    TEST_ASSERT_TRUE(process_monitor_gpu_wait(&monitor, renderer));
    TEST_ASSERT_TRUE(monitor.gpu_wait_active);
    TEST_ASSERT_EQUAL_UINT64(1, monitor.gpu_wait_calls);
    TEST_ASSERT_EQUAL_UINT64(0, monitor.gpu_wait_failures);
    TEST_ASSERT_TRUE(monitor.gpu_wait_last_ms >= 0);
    monitor.gpu_wait_requested = false;
    TEST_ASSERT_TRUE(process_monitor_gpu_wait(&monitor, renderer));
    TEST_ASSERT_FALSE(monitor.gpu_wait_active);
    TEST_ASSERT_EQUAL_UINT64(1, monitor.gpu_wait_calls);
}

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_no_gpu_does_not_claim_a_wait_or_report_failure);
    RUN_TEST(test_wait_is_opt_in_and_only_active_after_success);
    return UNITY_END();
}
