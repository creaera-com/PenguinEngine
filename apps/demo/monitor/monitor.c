/** @file @brief Low-frequency sampling and optional YAML panel assembly for all editors. */
#include "monitor.h"
#include "platform/process_metrics.h"
#include <stdio.h>
#include <string.h>

void process_monitor_tick(ProcessMonitor *m, PUIView *view, double now) {
    if (m->count && now - m->previous_time < PROCESS_MONITOR_INTERVAL) {
        return;
    }
    if (!process_monitor_record(m, process_metrics_read(), now)) {
        return;
    }
    const ProcessUsage *current = m->count ? process_monitor_sample(m, m->count - 1) : NULL;
    char ram[40] = "--", cpu[40] = "--", status[100];
    if (current && current->memory_available) {
        snprintf(ram, sizeof ram, "%.1f MiB", current->memory_mib);
    }
    if (current && current->cpu_available) {
        snprintf(cpu, sizeof cpu, "%.1f %%", current->cpu_percent);
    }
    snprintf(status, sizeof status, "RAM %s | CPU %s", ram, cpu);
    pui_view_set_text(view, "process_status", status);
}

PUILayout *process_monitor_layout(const EditorServices *services, const PUILayout *base,
                                  bool *requested, char *error, size_t capacity) {
    *requested = false;
    for (int i = 0; i < base->count; i++) {
        if (base->nodes[i].type == PUI_NODE_SLOT &&
            !strcmp(base->nodes[i].name, "process_monitor_content")) {
            *requested = true;
            break;
        }
    }
    if (!*requested) {
        return NULL;
    }
    PUILayout *panel = editor_load_ui(services, "monitor.yaml", error, capacity);
    PUILayout *result =
        panel ? pui_layout_compose(base, "process_monitor_content", panel, error, capacity) : NULL;
    pui_layout_free(panel);
    return result;
}
