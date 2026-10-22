/** @file @brief Convert cumulative CPU counters to interval utilization without inventing data. */
#include "history.h"
#include <math.h>
#include <stddef.h>

bool process_monitor_record(ProcessMonitor *m, ProcessCounters c, double now) {
    if (!m || !isfinite(now) || now < 0 || (m->count && now <= m->previous_time)) {
        return false;
    }
    ProcessUsage sample = {.time = now,
                           .memory_available = c.memory_available,
                           .memory_mib = c.resident_bytes / 1048576.0};
    bool cpu = c.cpu_available && isfinite(c.cpu_seconds) && c.cpu_seconds >= 0;
    if (cpu && m->cpu_baseline && c.cpu_seconds >= m->previous_cpu) {
        sample.cpu_percent = 100 * (c.cpu_seconds - m->previous_cpu) / (now - m->previous_time);
        sample.cpu_available = isfinite(sample.cpu_percent);
    }
    m->cpu_baseline = cpu;
    m->previous_cpu = c.cpu_seconds;
    m->previous_time = now;
    m->samples[m->head] = sample;
    m->head = (m->head + 1) % PROCESS_MONITOR_CAPACITY;
    if (m->count < PROCESS_MONITOR_CAPACITY) {
        m->count++;
    }
    return true;
}

const ProcessUsage *process_monitor_sample(const ProcessMonitor *m, unsigned index) {
    if (!m || index >= m->count) {
        return NULL;
    }
    return &m->samples[(m->head + PROCESS_MONITOR_CAPACITY - m->count + index) %
                       PROCESS_MONITOR_CAPACITY];
}
