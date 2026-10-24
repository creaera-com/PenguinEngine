/** @file @brief Optional diagnostic wait after presentation; normal rendering never waits here. */
#include "monitor.h"
#include <stdio.h>

bool process_monitor_gpu_wait(ProcessMonitor *m, SDL_Renderer *renderer) {
    SDL_GPUDevice *device = renderer
                                ? SDL_GetPointerProperty(SDL_GetRendererProperties(renderer),
                                                         SDL_PROP_RENDERER_GPU_DEVICE_POINTER, NULL)
                                : NULL;
    m->gpu_wait_available = device != NULL;
    m->gpu_wait_active = false;
    if (!m->gpu_wait_requested || !device) {
        return true;
    }
    Uint64 started = SDL_GetTicksNS();
    m->gpu_wait_calls++;
    bool ok = SDL_FlushRenderer(renderer) && SDL_WaitForGPUIdle(device);
    m->gpu_wait_last_ms = (double)(SDL_GetTicksNS() - started) / 1000000.;
    m->gpu_wait_total_ms += m->gpu_wait_last_ms;
    m->gpu_wait_active = ok;
    if (!ok) {
        m->gpu_wait_failures++;
        m->gpu_wait_requested = false;
    }
    return ok;
}

void process_monitor_gpu_wait_sync(const ProcessMonitor *m, PUIView *view) {
    pui_view_set_checked(view, "process_monitor_gpu_wait", m->gpu_wait_requested);
    char text[128];
    if (m->gpu_wait_requested && !m->gpu_wait_available) {
        snprintf(text, sizeof text, "Kein SDL-GPU-Renderer: Warten inaktiv");
    } else if (m->gpu_wait_requested && !m->gpu_wait_active) {
        snprintf(text, sizeof text, "GPU-Warten wird aktiviert (Diagnose)");
    } else if (m->gpu_wait_requested) {
        snprintf(text, sizeof text, "GPU-Warten: %.2f ms (Diagnose)", m->gpu_wait_last_ms);
    } else {
        snprintf(text, sizeof text, "GPU-Warten aus (Diagnose kann verlangsamen)");
    }
    pui_view_set_text(view, "process_monitor_gpu_wait_status", text);
}
