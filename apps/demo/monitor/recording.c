/** @file @brief YAML recording controls and feedback, separate from stream serialization. */
#include "monitor.h"
#include <stdio.h>
#include <string.h>

void process_monitor_recording_sync(const MonitorLog *log, PUIView *view) {
    pui_view_set_checked(view, "process_monitor_log", log->stream != NULL);
    char text[128];
    const char *filename = strrchr(log->path, '/');
    if (*log->error) {
        snprintf(text, sizeof text, "%.127s", log->error);
    } else if (log->stream) {
        snprintf(text, sizeof text, "Log: %.120s", filename ? filename + 1 : log->path);
    } else {
        snprintf(text, sizeof text, "%s",
                 *log->project ? "Aufzeichnung aus" : "Kein Projekt geoeffnet");
    }
    pui_view_set_text(view, "process_monitor_log_status", text);
}

bool process_monitor_recording_action(MonitorLog *log, const ProcessMonitor *monitor, PUIView *view,
                                      const char *editor, EditorDiagnostics context, double now) {
    if (pui_view_checked(view, "process_monitor_log")) {
        monitor_log_start(log, monitor, editor, context, now);
    } else {
        monitor_log_stop(log, monitor, "user.stop", now);
    }
    process_monitor_recording_sync(log, view);
    if (*log->error) {
        pui_view_set_text(view, "status", log->error);
    }
    return true;
}
