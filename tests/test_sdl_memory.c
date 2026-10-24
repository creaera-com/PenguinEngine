/** @file @brief Allocator forwarding, realloc failure, alignment and concurrent accounting. */
#include "platform/sdl_memory.h"
#include "unity.h"
#include <SDL3/SDL.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>

static void *old_pointer, *old_realloc_pointer;
static int malloc_failure_countdown;
static bool fail_realloc;

static void *SDLCALL underlying_malloc(size_t size) {
    if (malloc_failure_countdown && !--malloc_failure_countdown) {
        return NULL;
    }
    return malloc(size);
}

static void *SDLCALL underlying_calloc(size_t count, size_t size) {
    return calloc(count, size);
}

static void *SDLCALL underlying_realloc(void *pointer, size_t size) {
    if (fail_realloc) {
        fail_realloc = false;
        return NULL;
    }
    return realloc(pointer, size);
}

static void SDLCALL underlying_free(void *pointer) {
    free(pointer);
}

void setUp(void) {}

void tearDown(void) {}

static void test_older_pointers_are_forwarded_without_reading_a_fake_header(void) {
    AppSDLMemory before = app_sdl_memory_read();
    SDL_free(old_pointer);
    old_pointer = NULL;
    AppSDLMemory after = app_sdl_memory_read();
    TEST_ASSERT_EQUAL_UINT64(before.unknown_frees + 1, after.unknown_frees);
    TEST_ASSERT_EQUAL_UINT64(before.live_bytes, after.live_bytes);
}

static void test_older_realloc_pointers_are_adopted_without_changing_their_data(void) {
    AppSDLMemory before = app_sdl_memory_read();
    unsigned char *p = SDL_realloc(old_realloc_pointer, 64);
    TEST_ASSERT_NOT_NULL(p);
    old_realloc_pointer = NULL;
    for (int i = 0; i < 17; i++) {
        TEST_ASSERT_EQUAL_UINT8(0x55, p[i]);
    }
    AppSDLMemory after = app_sdl_memory_read();
    TEST_ASSERT_EQUAL_UINT64(before.unknown_reallocs + 1, after.unknown_reallocs);
    TEST_ASSERT_EQUAL_UINT64(before.live_bytes + 64, after.live_bytes);
    SDL_free(p);
    TEST_ASSERT_EQUAL_UINT64(before.live_bytes, app_sdl_memory_read().live_bytes);
}

static void test_calloc_overflow_is_rejected_and_counted(void) {
    AppSDLMemory before = app_sdl_memory_read();
    volatile size_t count = SIZE_MAX;
    TEST_ASSERT_NULL(SDL_calloc(count, 2));
    TEST_ASSERT_EQUAL_UINT64(before.failures + 1, app_sdl_memory_read().failures);
}

static void test_sizes_alignment_and_realloc_preserve_user_memory(void) {
    AppSDLMemory before = app_sdl_memory_read();
    unsigned char *p = SDL_malloc(64);
    unsigned char *zero = SDL_calloc(8, 16);
    TEST_ASSERT_NOT_NULL(p);
    TEST_ASSERT_NOT_NULL(zero);
    TEST_ASSERT_EQUAL_UINT64(0, (uintptr_t)p % _Alignof(max_align_t));
    for (int i = 0; i < 128; i++) {
        TEST_ASSERT_EQUAL_UINT8(0, zero[i]);
    }
    memset(p, 0x7a, 64);
    p = SDL_realloc(p, 256);
    TEST_ASSERT_NOT_NULL(p);
    for (int i = 0; i < 64; i++) {
        TEST_ASSERT_EQUAL_UINT8(0x7a, p[i]);
    }
    AppSDLMemory during = app_sdl_memory_read();
    TEST_ASSERT_TRUE(during.available);
    TEST_ASSERT_TRUE(during.complete);
    TEST_ASSERT_EQUAL_UINT64(before.live_blocks + 2, during.live_blocks);
    TEST_ASSERT_EQUAL_UINT64(before.live_bytes + 384, during.live_bytes);
    p = SDL_realloc(p, 16);
    TEST_ASSERT_NOT_NULL(p);
    TEST_ASSERT_EQUAL_UINT64(before.live_bytes + 144, app_sdl_memory_read().live_bytes);
    SDL_free(p);
    SDL_free(zero);
    AppSDLMemory after = app_sdl_memory_read();
    TEST_ASSERT_EQUAL_UINT64(before.live_bytes, after.live_bytes);
    TEST_ASSERT_EQUAL_UINT64(before.live_blocks, after.live_blocks);
    TEST_ASSERT_EQUAL_UINT64(after.live_bytes, after.allocated_bytes - after.freed_bytes);
}

static void test_failed_realloc_keeps_the_existing_block_and_size(void) {
    unsigned char *p = SDL_malloc(64);
    TEST_ASSERT_NOT_NULL(p);
    memset(p, 0x33, 64);
    AppSDLMemory before = app_sdl_memory_read();
    fail_realloc = true;
    TEST_ASSERT_NULL(SDL_realloc(p, 256));
    for (int i = 0; i < 64; i++) {
        TEST_ASSERT_EQUAL_UINT8(0x33, p[i]);
    }
    AppSDLMemory failed = app_sdl_memory_read();
    TEST_ASSERT_EQUAL_UINT64(before.failures + 1, failed.failures);
    SDL_free(p);
    TEST_ASSERT_EQUAL_UINT64(failed.live_bytes - 64, app_sdl_memory_read().live_bytes);
}

static void test_zero_realloc_and_aligned_allocations_follow_sdl_semantics(void) {
    AppSDLMemory before = app_sdl_memory_read();
    void *p = SDL_realloc(NULL, 0);
    TEST_ASSERT_NOT_NULL(p);
    TEST_ASSERT_EQUAL_UINT64(before.live_bytes + 1, app_sdl_memory_read().live_bytes);
    SDL_free(p);
    void *aligned = SDL_aligned_alloc(64, 130);
    TEST_ASSERT_NOT_NULL(aligned);
    TEST_ASSERT_EQUAL_UINT64(0, (uintptr_t)aligned % 64);
    SDL_aligned_free(aligned);
    TEST_ASSERT_EQUAL_UINT64(before.live_bytes, app_sdl_memory_read().live_bytes);
}

static int SDLCALL allocate_worker(void *context) {
    (void)context;
    for (int i = 0; i < 4000; i++) {
        unsigned char *p = SDL_malloc(32);
        if (!p) {
            return 1;
        }
        memset(p, i & 255, 32);
        void *grown = SDL_realloc(p, 80);
        if (!grown) {
            SDL_free(p);
            return 2;
        }
        SDL_free(grown);
    }
    return 0;
}

static void test_threads_keep_counts_balanced_and_install_is_idempotent(void) {
    SDL_Thread *threads[4];
    for (int i = 0; i < 4; i++) {
        threads[i] = SDL_CreateThread(allocate_worker, "memory-test", NULL);
        TEST_ASSERT_NOT_NULL(threads[i]);
    }
    for (int i = 0; i < 4; i++) {
        int status;
        SDL_WaitThread(threads[i], &status);
        TEST_ASSERT_EQUAL_INT(0, status);
    }
    AppSDLMemory snapshot = app_sdl_memory_read();
    TEST_ASSERT_TRUE(snapshot.complete);
    TEST_ASSERT_EQUAL_UINT64(snapshot.live_bytes, snapshot.allocated_bytes - snapshot.freed_bytes);
    TEST_ASSERT_TRUE(app_sdl_memory_install());
    TEST_ASSERT_EQUAL_UINT64(snapshot.allocated_bytes, app_sdl_memory_read().allocated_bytes);
}

static void test_missing_metadata_does_not_break_user_allocation_or_free(void) {
    AppSDLMemory before = app_sdl_memory_read();
    malloc_failure_countdown = 2; /* User allocation succeeds; only its side-table node fails. */
    void *p = SDL_malloc(512);
    TEST_ASSERT_NOT_NULL(p);
    memset(p, 0x66, 512);
    AppSDLMemory during = app_sdl_memory_read();
    TEST_ASSERT_FALSE(during.complete);
    TEST_ASSERT_EQUAL_UINT64(before.metadata_failures + 1, during.metadata_failures);
    TEST_ASSERT_EQUAL_UINT64(before.live_bytes, during.live_bytes);
    SDL_free(p);
    TEST_ASSERT_EQUAL_UINT64(before.unknown_frees + 1, app_sdl_memory_read().unknown_frees);
}

int main(void) {
    /* These callbacks are installed before any SDL threads; no allocator is replaced at runtime. */
    SDL_SetMemoryFunctions(underlying_malloc, underlying_calloc, underlying_realloc,
                           underlying_free);
    old_pointer = SDL_malloc(13);
    old_realloc_pointer = SDL_malloc(17);
    memset(old_realloc_pointer, 0x55, 17);
    if (!app_sdl_memory_install()) {
        return 1;
    }
    if (!SDL_Init(0)) {
        return 1;
    }
    UNITY_BEGIN();
    RUN_TEST(test_older_pointers_are_forwarded_without_reading_a_fake_header);
    RUN_TEST(test_older_realloc_pointers_are_adopted_without_changing_their_data);
    RUN_TEST(test_sizes_alignment_and_realloc_preserve_user_memory);
    RUN_TEST(test_failed_realloc_keeps_the_existing_block_and_size);
    RUN_TEST(test_calloc_overflow_is_rejected_and_counted);
    RUN_TEST(test_zero_realloc_and_aligned_allocations_follow_sdl_semantics);
    RUN_TEST(test_threads_keep_counts_balanced_and_install_is_idempotent);
    RUN_TEST(test_missing_metadata_does_not_break_user_allocation_or_free);
    SDL_Quit();
    return UNITY_END();
}
