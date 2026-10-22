/** @file @brief Linux procfs/process clock, macOS Mach/rusage and Windows process counters. */
#if !defined(_WIN32) && !defined(_POSIX_C_SOURCE)
#define _POSIX_C_SOURCE 200809L
#endif
#include "process_metrics.h"
#if defined(_WIN32)
#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#include <psapi.h>

static double seconds(FILETIME t) {
    ULARGE_INTEGER value;
    value.LowPart = t.dwLowDateTime;
    value.HighPart = t.dwHighDateTime;
    return (double)value.QuadPart / 10000000.;
}
#elif defined(__APPLE__)
#include <mach/mach.h>
#include <sys/resource.h>
#elif defined(__linux__)
#include <stdio.h>
#include <time.h>
#include <unistd.h>
#endif
ProcessCounters process_metrics_read(void) {
    ProcessCounters c = {0};
#if defined(_WIN32)
    FILETIME created, exited, kernel, user;
    if (GetProcessTimes(GetCurrentProcess(), &created, &exited, &kernel, &user)) {
        c.cpu_available = true;
        c.cpu_seconds = seconds(kernel) + seconds(user);
    }
    PROCESS_MEMORY_COUNTERS memory = {0};
    memory.cb = sizeof memory;
    if (GetProcessMemoryInfo(GetCurrentProcess(), &memory, sizeof memory)) {
        c.memory_available = true;
        c.resident_bytes = (uint64_t)memory.WorkingSetSize;
    }
#elif defined(__APPLE__)
    struct rusage usage;
    if (!getrusage(RUSAGE_SELF, &usage)) {
        c.cpu_available = true;
        c.cpu_seconds = usage.ru_utime.tv_sec + usage.ru_stime.tv_sec +
                        (usage.ru_utime.tv_usec + usage.ru_stime.tv_usec) / 1000000.;
    }
    mach_task_basic_info_data_t memory;
    mach_msg_type_number_t count = MACH_TASK_BASIC_INFO_COUNT;
    if (task_info(mach_task_self(), MACH_TASK_BASIC_INFO, (task_info_t)&memory, &count) ==
        KERN_SUCCESS) {
        c.memory_available = true;
        c.resident_bytes = (uint64_t)memory.resident_size;
    }
#elif defined(__linux__)
    struct timespec cpu;
    if (!clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &cpu)) {
        c.cpu_available = true;
        c.cpu_seconds = cpu.tv_sec + cpu.tv_nsec / 1000000000.;
    }
    FILE *file = fopen("/proc/self/statm", "r");
    if (file) {
        unsigned long long total, resident;
        long page = sysconf(_SC_PAGESIZE);
        if (fscanf(file, "%llu %llu", &total, &resident) == 2 && page > 0 &&
            resident <= UINT64_MAX / (uint64_t)page) {
            c.memory_available = true;
            c.resident_bytes = (uint64_t)resident * (uint64_t)page;
        }
        fclose(file);
    }
#endif
    return c;
}
