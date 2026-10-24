/** @file
 * @brief Bounded process-usage history and CPU rate calculation; no platform or UI APIs. */
#ifndef PENGUENGINE_MONITOR_HISTORY_H
#define PENGUENGINE_MONITOR_HISTORY_H
#include <stdbool.h>
#include <stdint.h>
#define PROCESS_MONITOR_CAPACITY 120
#define PROCESS_MONITOR_INTERVAL .5

/** @brief Cumulative process CPU time and current resident memory, with independent validity. */
typedef struct {
    double cpu_seconds;
    uint64_t resident_bytes;
    uint64_t gpu_effect_bytes; ///< Known pengDraw effect texture capacity, not total VRAM.
    bool gpu_memory_available;
    bool cpu_available, memory_available;
} ProcessCounters;

/** @brief One observation; CPU 100 percent means one fully occupied core across all threads. */
typedef struct {
    double time, cpu_percent, memory_mib, gpu_memory_mib;
    bool gpu_memory_available;
    bool cpu_available, memory_available;
} ProcessUsage;

/** @brief Last observed renderer metadata and known allocations; values only, no GPU handles. */
typedef struct {
    char device[128], backend[64], driver[128];
    uint64_t effect_bytes;
    bool effects_available, observed, resources_available, resources_complete;
    double observed_at;
    uint64_t textures_created, textures_released, texture_failures, live_textures;
    uint64_t live_bytes, peak_bytes;
    uint64_t artwork_bytes, shadow_bytes, mask_bytes, gradient_bytes, auxiliary_bytes;
    uint64_t shadow_fallback_attempts, gradient_fallback_attempts;
    uint64_t gpu_shadow_draws, gpu_shadow_failures, gpu_gradient_draws, gpu_gradient_failures;
    unsigned shadow_width, shadow_height, shadow_targets;
} MonitorGraphics;

/** @brief Fixed-size ring: memory and work remain bounded regardless of application lifetime. */
typedef struct {
    ProcessUsage samples[PROCESS_MONITOR_CAPACITY];
    MonitorGraphics graphics;
    unsigned head, count;
    double previous_time, previous_cpu;
    bool cpu_baseline;
    bool gpu_wait_requested, gpu_wait_available, gpu_wait_active;
    uint64_t gpu_wait_calls, gpu_wait_failures;
    double gpu_wait_last_ms, gpu_wait_total_ms;
} ProcessMonitor;

/** @brief Append valid chronological observations; reject non-finite or non-increasing times.
 * Missing CPU readings reset the baseline; the next reading cannot yet produce a rate. */
bool process_monitor_record(ProcessMonitor *monitor, ProcessCounters counters, double now);
/** @brief Borrow a sample in oldest-to-newest order; NULL for an invalid index. */
const ProcessUsage *process_monitor_sample(const ProcessMonitor *monitor, unsigned index);
#endif
