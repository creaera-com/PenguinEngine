/** @file @brief Low-frequency sampling and optional YAML panel assembly for all editors. */
#include "monitor.h"
#include "pengdraw/pengdraw.h"
#include "platform/process_metrics.h"
#include <stdio.h>
#include <string.h>

static void clean_text(char *destination, size_t capacity, const char *source) {
    snprintf(destination, capacity, "%s", source ? source : "Nicht verfuegbar");
    for (char *p = destination; *p; p++) {
        if ((unsigned char)*p < 32 || *p == 127) {
            *p = ' ';
        }
    }
}

void process_monitor_graphics(ProcessMonitor *m, SDL_Renderer *r) {
    MonitorGraphics g = {0};
    clean_text(g.device, sizeof g.device, "Geraet nicht gemeldet");
    clean_text(g.backend, sizeof g.backend, r ? SDL_GetRendererName(r) : NULL);
    clean_text(g.driver, sizeof g.driver, NULL);
    SDL_GPUDevice *device = r ? SDL_GetPointerProperty(SDL_GetRendererProperties(r),
                                                       SDL_PROP_RENDERER_GPU_DEVICE_POINTER, NULL)
                              : NULL;
    if (device) {
        SDL_PropertiesID props = SDL_GetGPUDeviceProperties(device);
        clean_text(g.device, sizeof g.device,
                   SDL_GetStringProperty(props, SDL_PROP_GPU_DEVICE_NAME_STRING, "Unbekannt"));
        char driver[256];
        snprintf(driver, sizeof driver, "%s / %s", SDL_GetGPUDeviceDriver(device),
                 SDL_GetStringProperty(props, SDL_PROP_GPU_DEVICE_DRIVER_NAME_STRING, "Unbekannt"));
        clean_text(g.driver, sizeof g.driver, driver);
    }
    g.effects_available = pdraw_renderer_gpu_effects(r);
    PDrawGPUStats effects = pdraw_renderer_gpu_stats(r);
    g.effect_bytes = effects.texture_bytes;
    g.gpu_shadow_draws = effects.shadow_draws;
    g.gpu_shadow_failures = effects.shadow_failures;
    g.gpu_gradient_draws = effects.gradient_draws;
    g.gpu_gradient_failures = effects.gradient_failures;
    g.shadow_width = effects.shadow_width;
    g.shadow_height = effects.shadow_height;
    g.shadow_targets = effects.shadow_targets;
    PDrawResourceStats resources = pdraw_renderer_resource_stats(r);
    g.resources_available = resources.available;
    g.resources_complete = resources.complete;
    g.textures_created = resources.textures_created;
    g.textures_released = resources.textures_released;
    g.texture_failures = resources.texture_failures;
    g.live_textures = resources.live_textures;
    g.live_bytes = resources.live_bytes;
    g.peak_bytes = resources.peak_bytes;
    g.artwork_bytes = resources.artwork_bytes;
    g.shadow_bytes = resources.shadow_bytes;
    g.mask_bytes = resources.mask_bytes;
    g.gradient_bytes = resources.gradient_bytes;
    g.auxiliary_bytes = resources.auxiliary_bytes;
    g.shadow_fallback_attempts = resources.shadow_fallback_attempts;
    g.gradient_fallback_attempts = resources.gradient_fallback_attempts;
    g.observed = r != NULL;
    g.observed_at = (double)SDL_GetTicksNS() / 1000000000.;
    m->graphics = g;
}

static void graphics_labels(const ProcessMonitor *m, PUIView *view) {
    char text[128];
    snprintf(text, sizeof text, "GPU: %.120s", m->graphics.device);
    pui_view_set_text(view, "process_monitor_gpu_device", text);
    snprintf(text, sizeof text, "Renderer: %.112s", m->graphics.backend);
    pui_view_set_text(view, "process_monitor_gpu_backend", text);
    snprintf(text, sizeof text, "Treiber: %.116s", m->graphics.driver);
    pui_view_set_text(view, "process_monitor_gpu_driver", text);
    pui_view_set_text(view, "process_monitor_gpu_pipeline",
                      m->graphics.effects_available ? "Schatten / Verlaeufe: GPU"
                                                    : "Effekte: CPU-Kompatibilitaet");
}

void process_monitor_tick(ProcessMonitor *m, PUIView *view, double now) {
    if (m->count && now - m->previous_time < PROCESS_MONITOR_INTERVAL) {
        return;
    }
    graphics_labels(m, view);
    ProcessCounters counters = process_metrics_read();
    counters.gpu_effect_bytes = m->graphics.effect_bytes;
    counters.gpu_memory_available = m->graphics.effects_available;
    if (!process_monitor_record(m, counters, now)) {
        return;
    }
    const ProcessUsage *current = m->count ? process_monitor_sample(m, m->count - 1) : NULL;
    char ram[40] = "--", cpu[40] = "--", gpu[40] = "--", status[160];
    if (current && current->memory_available) {
        snprintf(ram, sizeof ram, "%.1f MiB", current->memory_mib);
    }
    if (current && current->cpu_available) {
        snprintf(cpu, sizeof cpu, "%.1f %%", current->cpu_percent);
    }
    if (current && current->gpu_memory_available) {
        snprintf(gpu, sizeof gpu, "%.1f MiB", current->gpu_memory_mib);
    }
    snprintf(status, sizeof status, "RAM %s | CPU %s | GPU-FX %s", ram, cpu, gpu);
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
