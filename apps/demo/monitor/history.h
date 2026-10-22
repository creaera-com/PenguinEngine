/** @file @brief Bounded process-usage history and CPU rate calculation; no platform or UI APIs. */
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
    bool cpu_available, memory_available;
} ProcessCounters;

/** @brief One observation; CPU 100 percent means one fully occupied core across all threads. */
typedef struct {
    double time, cpu_percent, memory_mib;
    bool cpu_available, memory_available;
} ProcessUsage;

/** @brief Fixed-size ring: memory and work remain bounded regardless of application lifetime. */
typedef struct {
    ProcessUsage samples[PROCESS_MONITOR_CAPACITY];
    unsigned head, count;
    double previous_time, previous_cpu;
    bool cpu_baseline;
} ProcessMonitor;

/** @brief Append valid chronological observations; reject non-finite or non-increasing times.
 * Missing CPU readings reset the baseline; the next reading cannot yet produce a rate. */
bool process_monitor_record(ProcessMonitor *monitor, ProcessCounters counters, double now);
/** @brief Borrow a sample in oldest-to-newest order; NULL for an invalid index. */
const ProcessUsage *process_monitor_sample(const ProcessMonitor *monitor, unsigned index);
#endif
