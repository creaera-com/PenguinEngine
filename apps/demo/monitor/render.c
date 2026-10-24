/** @file @brief Live process graphs with a fixed 60-second time axis and independent value scales.
 */
#include "monitor.h"
#include <math.h>
#include <stdio.h>
#include <string.h>

static bool available(const ProcessUsage *s, int metric) {
    return metric == 0   ? s->cpu_available
           : metric == 1 ? s->memory_available
                         : s->gpu_memory_available;
}

static double value(const ProcessUsage *s, int metric) {
    return metric == 0 ? s->cpu_percent : metric == 1 ? s->memory_mib : s->gpu_memory_mib;
}

bool process_monitor_draw(const ProcessMonitor *m, SDL_Renderer *r, PUIRect a, const char *widget) {
    bool cpu = !strcmp(widget, "process_monitor_cpu");
    bool gpu = !strcmp(widget, "process_monitor_gpu_memory");
    int metric = cpu ? 0 : gpu ? 2 : 1;
    if (!cpu && !gpu && strcmp(widget, "process_monitor_memory")) {
        return false;
    }
    if (a.w < 80 || a.h < 70) {
        return true;
    }
    const ProcessUsage *latest = m->count ? process_monitor_sample(m, m->count - 1) : NULL;
    char text[96];
    if (latest && available(latest, metric)) {
        snprintf(text, sizeof text,
                 cpu   ? "CPU %.1f %%"
                 : gpu ? "GPU-Effekttexturen %.1f MiB"
                       : "RAM %.1f MiB",
                 value(latest, metric));
    } else {
        snprintf(text, sizeof text, "%s: %s",
                 cpu   ? "CPU"
                 : gpu ? "GPU-Effekttexturen"
                       : "RAM",
                 !latest || (cpu && m->count < 2) ? "Messung ausstehend" : "Nicht verfuegbar");
    }
    pui_text(r, a.x + 5, a.y + 5, text, PUI_TEXT);
    PUIRect plot = {a.x + 48, a.y + 30, a.w - 56, a.h - 52};
    double maximum = cpu ? 100 : gpu ? 1 : 64;
    for (unsigned i = 0; i < m->count; i++) {
        const ProcessUsage *s = process_monitor_sample(m, i);
        if (available(s, metric) && latest && latest->time - s->time <= 60) {
            maximum = fmax(maximum, value(s, metric));
        }
    }
    double step = cpu ? 100 : gpu ? 1 : 32;
    maximum = ceil(maximum / step) * step;
    SDL_BlendMode blend;
    SDL_GetRenderDrawBlendMode(r, &blend);
    SDL_SetRenderDrawBlendMode(r, SDL_BLENDMODE_BLEND);
    for (int i = 0; i < 3; i++) {
        float y = plot.y + plot.h * i / 2;
        pui_render_color(r, PUI_BORDER);
        SDL_RenderLine(r, plot.x, y, plot.x + plot.w, y);
        snprintf(text, sizeof text, gpu ? "%.1f" : "%.0f", maximum * (1 - i / 2.));
        pui_text_scaled(r, a.x + 3, y - 5, text, PUI_DISABLED, .75f);
    }
    pui_text_scaled(r, plot.x, plot.y + plot.h + 8, "-60 s", PUI_DISABLED, .75f);
    pui_text_scaled(r, plot.x + plot.w / 2 - 16, plot.y + plot.h + 8, "-30 s", PUI_DISABLED, .75f);
    pui_text_scaled(r, plot.x + plot.w - 16, plot.y + plot.h + 8, "0 s", PUI_DISABLED, .75f);
    bool previous = false;
    SDL_FPoint last = {0};
    for (unsigned i = 0; i < m->count && latest; i++) {
        const ProcessUsage *s = process_monitor_sample(m, i);
        double age = latest->time - s->time;
        if (!available(s, metric) || age > 60) {
            previous = false;
            continue;
        }
        SDL_FPoint point = {plot.x + plot.w * (float)(1 - age / 60),
                            plot.y + plot.h * (float)(1 - value(s, metric) / maximum)};
        pui_render_color(r, cpu ? PUI_HIGHLIGHT : gpu ? 0xbca4e8ff : 0x78b395ff);
        if (previous) {
            SDL_RenderLine(r, last.x, last.y, point.x, point.y);
        }
        SDL_RenderPoint(r, point.x, point.y);
        last = point;
        previous = true;
    }
    SDL_SetRenderDrawBlendMode(r, blend);
    return true;
}
