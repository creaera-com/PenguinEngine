/** @file @brief Deterministic counter/rate math and bounded process history. */
#include "monitor/history.h"
#include "platform/process_metrics.h"
#include "unity.h"
#include <math.h>

void setUp(void) {}

void tearDown(void) {}

static void test_cpu_delta_uses_elapsed_time_and_preserves_multicore_usage(void) {
    ProcessMonitor m = {0};
    ProcessCounters c = {.cpu_seconds = 10,
                         .resident_bytes = 100 * UINT64_C(1048576),
                         .cpu_available = true,
                         .memory_available = true};
    TEST_ASSERT_TRUE(process_monitor_record(&m, c, 1));
    TEST_ASSERT_FALSE(process_monitor_sample(&m, 0)->cpu_available);
    TEST_ASSERT_EQUAL_FLOAT(100, process_monitor_sample(&m, 0)->memory_mib);
    c.cpu_seconds += .25;
    TEST_ASSERT_TRUE(process_monitor_record(&m, c, 1.5));
    TEST_ASSERT_EQUAL_FLOAT(50, process_monitor_sample(&m, 1)->cpu_percent);
    c.cpu_seconds += 1.5;
    TEST_ASSERT_TRUE(process_monitor_record(&m, c, 2));
    TEST_ASSERT_EQUAL_FLOAT(300, process_monitor_sample(&m, 2)->cpu_percent);
    TEST_ASSERT_FALSE(process_monitor_record(&m, c, 2));
    TEST_ASSERT_FALSE(process_monitor_record(&m, c, NAN));
    TEST_ASSERT_EQUAL_UINT(3, m.count);
}

static void test_unavailable_and_reset_counters_do_not_invent_rates(void) {
    ProcessMonitor m = {0};
    process_monitor_record(&m, (ProcessCounters){0}, 1);
    TEST_ASSERT_FALSE(process_monitor_sample(&m, 0)->memory_available);
    ProcessCounters c = {.cpu_seconds = 5, .cpu_available = true};
    process_monitor_record(&m, c, 2);
    TEST_ASSERT_FALSE(process_monitor_sample(&m, 1)->cpu_available);
    c.cpu_seconds = 4;
    process_monitor_record(&m, c, 3);
    TEST_ASSERT_FALSE(process_monitor_sample(&m, 2)->cpu_available);
    c.cpu_seconds = 4.5;
    process_monitor_record(&m, c, 4);
    TEST_ASSERT_EQUAL_FLOAT(50, process_monitor_sample(&m, 3)->cpu_percent);
}

static void test_history_rolls_over_in_chronological_order(void) {
    ProcessMonitor m = {0};
    for (unsigned i = 0; i < 140; i++) {
        ProcessCounters c = {.cpu_seconds = i * .25,
                             .resident_bytes = i * UINT64_C(1048576),
                             .cpu_available = true,
                             .memory_available = true};
        TEST_ASSERT_TRUE(process_monitor_record(&m, c, i * .5));
    }
    TEST_ASSERT_EQUAL_UINT(PROCESS_MONITOR_CAPACITY, m.count);
    TEST_ASSERT_EQUAL_FLOAT(20, process_monitor_sample(&m, 0)->memory_mib);
    TEST_ASSERT_EQUAL_FLOAT(139, process_monitor_sample(&m, 119)->memory_mib);
    TEST_ASSERT_NULL(process_monitor_sample(&m, 120));
}

static void test_native_backend_reads_this_process(void) {
    ProcessCounters a = process_metrics_read(), b = process_metrics_read();
#if defined(_WIN32) || defined(__APPLE__) || defined(__linux__)
    TEST_ASSERT_TRUE(a.cpu_available);
    TEST_ASSERT_TRUE(a.memory_available);
    TEST_ASSERT_TRUE(a.resident_bytes > 0);
    TEST_ASSERT_TRUE(b.cpu_seconds >= a.cpu_seconds);
#else
    TEST_ASSERT_FALSE(a.cpu_available);
    TEST_ASSERT_FALSE(b.memory_available);
#endif
}

static void test_known_gpu_texture_capacity_is_independent_from_process_ram(void) {
    ProcessMonitor monitor = {0};
    ProcessCounters counters = {.gpu_memory_available = true,
                                .gpu_effect_bytes = UINT64_C(32) * 1048576};
    TEST_ASSERT_TRUE(process_monitor_record(&monitor, counters, 1));
    const ProcessUsage *sample = process_monitor_sample(&monitor, 0);
    TEST_ASSERT_TRUE(sample->gpu_memory_available);
    TEST_ASSERT_EQUAL_FLOAT(32, sample->gpu_memory_mib);
    TEST_ASSERT_FALSE(sample->memory_available);
    counters.gpu_effect_bytes = 0;
    TEST_ASSERT_TRUE(process_monitor_record(&monitor, counters, 2));
    TEST_ASSERT_TRUE(process_monitor_sample(&monitor, 1)->gpu_memory_available);
    TEST_ASSERT_EQUAL_FLOAT(0, process_monitor_sample(&monitor, 1)->gpu_memory_mib);
    counters.gpu_memory_available = false;
    TEST_ASSERT_TRUE(process_monitor_record(&monitor, counters, 3));
    TEST_ASSERT_FALSE(process_monitor_sample(&monitor, 2)->gpu_memory_available);
    TEST_ASSERT_EQUAL_FLOAT(32, process_monitor_sample(&monitor, 0)->gpu_memory_mib);
}

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_known_gpu_texture_capacity_is_independent_from_process_ram);
    RUN_TEST(test_cpu_delta_uses_elapsed_time_and_preserves_multicore_usage);
    RUN_TEST(test_unavailable_and_reset_counters_do_not_invent_rates);
    RUN_TEST(test_history_rolls_over_in_chronological_order);
    RUN_TEST(test_native_backend_reads_this_process);
    return UNITY_END();
}
