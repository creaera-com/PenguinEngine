/** @file
 * @brief Opt-in streaming performance/action recording in the active project. */
#ifndef PENGUENGINE_MONITOR_LOG_H
#define PENGUENGINE_MONITOR_LOG_H
#include "editor_host/diagnostics.h"
#include "history.h"
#include <SDL3/SDL.h>

/** @brief Owns one stream and bounded metadata; recorded rows are never retained in RAM. */
typedef struct {
    SDL_IOStream *stream;
    char project[4096], path[4096], error[256], editor[32];
    EditorDiagnostics previous;
    double started, last_sample;
    bool observed;
} MonitorLog;

/** @brief Stop on project change; never automatically resume in another project. */
void monitor_log_project(MonitorLog *log, const char *root, const ProcessMonitor *monitor,
                         double now);
/** @brief Exclusively create a new JSONL file in the project root. False leaves error populated. */
bool monitor_log_start(MonitorLog *log, const ProcessMonitor *monitor, const char *editor,
                       EditorDiagnostics context, double now);
/** @brief Write and flush a row with fresh RAM and the latest interval CPU/GPU observations. */
void monitor_log_action(MonitorLog *log, const ProcessMonitor *monitor, const char *editor,
                        EditorDiagnostics context, const char *action, const char *widget,
                        double now);
/** @brief Record context/gesture changes and new monitor samples; no per-motion file writes. */
void monitor_log_observe(MonitorLog *log, const ProcessMonitor *monitor, const char *editor,
                         EditorDiagnostics context, double now);
/** @brief Flush a final row and close; safe repeatedly, including failed starts. */
void monitor_log_stop(MonitorLog *log, const ProcessMonitor *monitor, const char *reason,
                      double now);
#endif
