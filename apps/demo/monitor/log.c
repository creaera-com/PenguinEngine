/** @file @brief Bounded JSONL streaming with explicit missing metrics and no input contents. */
#include "log.h"
#include "platform/process_metrics.h"
#include "platform/sdl_memory.h"
#include <inttypes.h>
#include <math.h>
#include <stdio.h>
#include <string.h>

static bool put(MonitorLog *log, const char *text) {
    size_t size = strlen(text);
    return SDL_WriteIO(log->stream, text, size) == size;
}

static bool quoted(MonitorLog *log, const char *text) {
    bool ok = put(log, "\"");
    for (const unsigned char *p = (const unsigned char *)(text ? text : ""); ok && *p; p++) {
        char escaped[8];
        if (*p == '"' || *p == '\\') {
            snprintf(escaped, sizeof escaped, "\\%c", *p);
        } else if (*p < 32) {
            snprintf(escaped, sizeof escaped, "\\u%04x", *p);
        } else {
            escaped[0] = (char)*p;
            escaped[1] = 0;
        }
        ok = put(log, escaped);
    }
    return ok && put(log, "\"");
}

static bool field(MonitorLog *log, const char *key, const char *value) {
    return put(log, ",") && quoted(log, key) && put(log, ":") && quoted(log, value);
}

static bool number(MonitorLog *log, const char *key, double value, bool available) {
    char text[64];
    snprintf(text, sizeof text, "%.6f", value);
    /* Keep JSON valid even if a caller has selected a decimal-comma locale. */
    for (char *p = text; *p; p++) {
        if (*p == ',') {
            *p = '.';
        }
    }
    return put(log, ",") && quoted(log, key) && put(log, ":") &&
           put(log, available && isfinite(value) ? text : "null");
}

static bool selection(MonitorLog *log, const EditorDiagnostics *d) {
    bool ok = put(log, ",\"selected_ids\":[");
    unsigned count = d->selected < EDITOR_DIAGNOSTIC_SELECTION_CAPACITY
                         ? d->selected
                         : EDITOR_DIAGNOSTIC_SELECTION_CAPACITY;
    for (unsigned i = 0; ok && i < count; i++) {
        char id[32];
        snprintf(id, sizeof id, "%s%u", i ? "," : "", d->selected_ids[i]);
        ok = put(log, id);
    }
    return ok && put(log, "]");
}

static bool integer(MonitorLog *log, const char *key, uint64_t value, bool available) {
    char text[32];
    snprintf(text, sizeof text, "%" PRIu64, value);
    return put(log, ",") && quoted(log, key) && put(log, ":") &&
           put(log, available ? text : "null");
}

static bool resources(MonitorLog *log, const MonitorGraphics *g, double now) {
    bool available = g->resources_available;
    return put(log, ",\"render_resources\":{\"available\":") &&
           put(log, available ? "true" : "false") && put(log, ",\"complete\":") &&
           put(log, g->resources_complete ? "true" : "false") &&
           number(log, "age_s", now - g->observed_at, g->observed) &&
           integer(log, "textures_created", g->textures_created, available) &&
           integer(log, "textures_released", g->textures_released, available) &&
           integer(log, "texture_failures", g->texture_failures, available) &&
           integer(log, "live_textures", g->live_textures, available) &&
           integer(log, "live_bytes", g->live_bytes, available) &&
           integer(log, "peak_bytes", g->peak_bytes, available) &&
           integer(log, "artwork_bytes", g->artwork_bytes, available) &&
           integer(log, "shadow_bytes", g->shadow_bytes, available) &&
           integer(log, "mask_bytes", g->mask_bytes, available) &&
           integer(log, "gradient_bytes", g->gradient_bytes, available) &&
           integer(log, "auxiliary_bytes", g->auxiliary_bytes, available) &&
           integer(log, "shadow_fallback_attempts", g->shadow_fallback_attempts, available) &&
           integer(log, "gradient_fallback_attempts", g->gradient_fallback_attempts, available) &&
           integer(log, "gpu_shadow_draws", g->gpu_shadow_draws, g->effects_available) &&
           integer(log, "gpu_shadow_failures", g->gpu_shadow_failures, g->effects_available) &&
           integer(log, "gpu_gradient_draws", g->gpu_gradient_draws, g->effects_available) &&
           integer(log, "gpu_gradient_failures", g->gpu_gradient_failures, g->effects_available) &&
           integer(log, "shadow_width", g->shadow_width, g->effects_available) &&
           integer(log, "shadow_height", g->shadow_height, g->effects_available) &&
           integer(log, "shadow_targets", g->shadow_targets, g->effects_available) && put(log, "}");
}

static bool artwork_cache(MonitorLog *log, const EditorDiagnostics *d) {
    bool available = d->artwork_cache_available;
    return put(log, ",\"artwork_cache\":{\"available\":") &&
           put(log, available ? "true" : "false") &&
           integer(log, "tile_bytes", d->cache_tile_bytes, available) &&
           integer(log, "scratch_bytes", d->cache_scratch_bytes, available) &&
           integer(log, "tiles", d->cache_tiles, available) &&
           integer(log, "hits", d->cache_hits, available) &&
           integer(log, "misses", d->cache_misses, available) &&
           integer(log, "repaints", d->cache_repaints, available) &&
           integer(log, "evictions", d->cache_evictions, available) && put(log, "}");
}

static bool sdl_memory(MonitorLog *log) {
    AppSDLMemory memory = app_sdl_memory_read();
    bool ok = put(log, ",\"sdl_memory\":{\"available\":") &&
              put(log, memory.available ? "true" : "false") && put(log, ",\"complete\":") &&
              put(log, memory.complete ? "true" : "false");
    ok = ok && integer(log, "malloc_calls", memory.malloc_calls, memory.available);
    ok = ok && integer(log, "calloc_calls", memory.calloc_calls, memory.available);
    ok = ok && integer(log, "realloc_calls", memory.realloc_calls, memory.available);
    ok = ok && integer(log, "free_calls", memory.free_calls, memory.available);
    ok = ok && integer(log, "failures", memory.failures, memory.available);
    ok = ok && integer(log, "metadata_failures", memory.metadata_failures, memory.available);
    ok = ok && integer(log, "live_blocks", memory.live_blocks, memory.available);
    ok = ok && integer(log, "live_bytes", memory.live_bytes, memory.available);
    ok = ok && integer(log, "peak_bytes", memory.peak_bytes, memory.available);
    ok = ok && integer(log, "allocated_bytes", memory.allocated_bytes, memory.available);
    ok = ok && integer(log, "freed_bytes", memory.freed_bytes, memory.available);
    ok = ok && integer(log, "unknown_frees", memory.unknown_frees, memory.available);
    ok = ok && integer(log, "unknown_reallocs", memory.unknown_reallocs, memory.available);
    ok = ok && integer(log, "metadata_bytes", memory.metadata_bytes, memory.available);
    return ok && put(log, "}");
}

static bool gpu_wait(MonitorLog *log, const ProcessMonitor *m) {
    return put(log, ",\"gpu_wait\":{\"requested\":") &&
           put(log, m->gpu_wait_requested ? "true" : "false") && put(log, ",\"available\":") &&
           put(log, m->gpu_wait_available ? "true" : "false") && put(log, ",\"active\":") &&
           put(log, m->gpu_wait_active ? "true" : "false") &&
           integer(log, "calls", m->gpu_wait_calls, true) &&
           integer(log, "failures", m->gpu_wait_failures, true) &&
           number(log, "last_ms", m->gpu_wait_last_ms, m->gpu_wait_available) &&
           number(log, "total_ms", m->gpu_wait_total_ms, m->gpu_wait_available) && put(log, "}");
}

static void utc(char *text, size_t capacity, bool filename) {
    SDL_Time time;
    SDL_DateTime date;
    if (!SDL_GetCurrentTime(&time) || !SDL_TimeToDateTime(time, &date, false)) {
        snprintf(text, capacity, "unknown-time");
        return;
    }
    snprintf(text, capacity,
             filename ? "%04d%02d%02d-%02d%02d%02d" : "%04d-%02d-%02dT%02d:%02d:%02dZ", date.year,
             date.month, date.day, date.hour, date.minute, date.second);
}

static void failed(MonitorLog *log) {
    snprintf(log->error, sizeof log->error, "Log-Schreibfehler: %.220s", SDL_GetError());
    SDL_CloseIO(log->stream);
    log->stream = NULL;
}

static void row(MonitorLog *log, const ProcessMonitor *m, const char *editor, EditorDiagnostics d,
                const char *event, const char *action, const char *widget, double now) {
    if (!log->stream) {
        return;
    }
    char timestamp[40];
    utc(timestamp, sizeof timestamp, false);
    const ProcessUsage *sample = m->count ? process_monitor_sample(m, m->count - 1) : NULL;
    ProcessCounters current = process_metrics_read();
    bool ok =
        put(log, "{\"schema\":\"penguengine/performance/v1\"") && field(log, "utc", timestamp) &&
        number(log, "elapsed_s", now - log->started, true) && field(log, "event", event) &&
        field(log, "editor", editor) && field(log, "action", action) &&
        field(log, "widget", widget) && field(log, "operation", d.operation) &&
        number(log, "document", d.document, true) && number(log, "objects", d.objects, true) &&
        number(log, "instances", d.instances, true) && number(log, "selected", d.selected, true) &&
        selection(log, &d) && number(log, "active_artboard", d.active_artboard, true) &&
        number(log, "active_object", d.active_object, true) &&
        number(log, "viewport_x", d.viewport_x, d.viewport_available) &&
        number(log, "viewport_y", d.viewport_y, d.viewport_available) &&
        number(log, "zoom", d.zoom, d.viewport_available) &&
        number(log, "history_cursor", d.history_cursor, true) &&
        number(log, "ram_mib", current.resident_bytes / 1048576., current.memory_available) &&
        number(log, "cpu_percent", sample ? sample->cpu_percent : 0,
               sample && sample->cpu_available) &&
        number(log, "metrics_age_s", sample ? now - sample->time : 0, sample != NULL) &&
        number(log, "gpu_effect_mib", m->graphics.effect_bytes / 1048576.,
               m->graphics.effects_available) &&
        resources(log, &m->graphics, now) && artwork_cache(log, &d) && sdl_memory(log) &&
        gpu_wait(log, m);
    if (!strcmp(event, "session.start")) {
        ok = ok && field(log, "platform", SDL_GetPlatform()) &&
             field(log, "gpu_device", m->graphics.device) &&
             field(log, "renderer", m->graphics.backend) &&
             field(log, "gpu_driver", m->graphics.driver) &&
             field(log, "gpu_memory_scope", "pengDraw shadow effect texture capacity only") &&
             field(log, "render_resource_scope",
                   "pengDraw-owned logical texture handles; excludes UI, images, fonts, swapchain, "
                   "driver copies and deferred GPU releases") &&
             field(log, "cache_counter_scope", "active editor instance; resets on unload") &&
             field(log, "sdl_memory_scope",
                   "requested sizes via SDL allocator callbacks since startup; excludes "
                   "libc/driver allocations, allocator slack and GPU memory");
    }
    if (!ok || !put(log, "}\n") || !SDL_FlushIO(log->stream)) {
        failed(log);
    }
}

void monitor_log_stop(MonitorLog *log, const ProcessMonitor *m, const char *reason, double now) {
    if (!log->stream) {
        return;
    }
    row(log, m, log->editor, log->previous, "session.stop", reason, "", now);
    if (log->stream) {
        bool ok = SDL_CloseIO(log->stream);
        log->stream = NULL;
        if (!ok) {
            snprintf(log->error, sizeof log->error, "Log konnte nicht geschlossen werden");
        }
    }
}

void monitor_log_project(MonitorLog *log, const char *root, const ProcessMonitor *m, double now) {
    root = root ? root : "";
    if (!strcmp(log->project, root)) {
        return;
    }
    monitor_log_stop(log, m, "project.changed", now);
    if (strlen(root) >= sizeof log->project) {
        log->project[0] = 0;
        snprintf(log->error, sizeof log->error, "Projektpfad ist zu lang");
        return;
    }
    snprintf(log->project, sizeof log->project, "%s", root);
}

bool monitor_log_start(MonitorLog *log, const ProcessMonitor *m, const char *editor,
                       EditorDiagnostics d, double now) {
    if (log->stream) {
        return true;
    }
    log->error[0] = 0;
    SDL_PathInfo info;
    if (!*log->project || !SDL_GetPathInfo(log->project, &info) ||
        info.type != SDL_PATHTYPE_DIRECTORY) {
        snprintf(log->error, sizeof log->error, "Bitte zuerst ein Projekt oeffnen");
        return false;
    }
    char timestamp[32];
    utc(timestamp, sizeof timestamp, true);
    /* x prevents overwriting an existing session, including simultaneous processes. */
    for (unsigned i = 0; i < 1000; i++) {
        if (snprintf(log->path, sizeof log->path, "%s/performance-%s-%03u.jsonl", log->project,
                     timestamp, i) >= (int)sizeof log->path) {
            snprintf(log->error, sizeof log->error, "Log-Pfad ist zu lang");
            return false;
        }
        if (SDL_GetPathInfo(log->path, &info)) {
            continue;
        }
        log->stream = SDL_IOFromFile(log->path, "wx");
        if (log->stream) {
            break;
        }
        if (!SDL_GetPathInfo(log->path, &info)) {
            break;
        }
    }
    if (!log->stream) {
        snprintf(log->error, sizeof log->error, "Log kann nicht angelegt werden: %.210s",
                 SDL_GetError());
        return false;
    }
    log->started = now;
    log->last_sample = m->count ? m->previous_time : -1;
    log->previous = d;
    log->observed = true;
    snprintf(log->editor, sizeof log->editor, "%s", editor ? editor : "");
    row(log, m, log->editor, d, "session.start", "", "", now);
    return log->stream != NULL;
}

void monitor_log_action(MonitorLog *log, const ProcessMonitor *m, const char *editor,
                        EditorDiagnostics d, const char *action, const char *widget, double now) {
    row(log, m, editor, d, "action", action, widget, now);
}

void monitor_log_observe(MonitorLog *log, const ProcessMonitor *m, const char *editor,
                         EditorDiagnostics d, double now) {
    if (!log->stream) {
        return;
    }
    bool same_context =
        log->observed && !strcmp(log->editor, editor) && log->previous.document == d.document;
    const char *event = NULL;
    if (!same_context) {
        event = "context.changed";
    } else if (strcmp(log->previous.operation, d.operation) ||
               log->previous.interacting != d.interacting) {
        event = d.interacting ? "interaction.begin" : "interaction.end";
    } else if (log->previous.history_cursor != d.history_cursor ||
               log->previous.objects != d.objects || log->previous.instances != d.instances) {
        event = "document.changed";
    }
    if (event) {
        row(log, m, editor, d, event, log->previous.operation, "", now);
    }
    if (same_context &&
        (log->previous.selected != d.selected || log->previous.active_object != d.active_object ||
         log->previous.active_artboard != d.active_artboard ||
         memcmp(log->previous.selected_ids, d.selected_ids, sizeof d.selected_ids))) {
        row(log, m, editor, d, "selection.changed", "selection.change", "", now);
    }
    if (same_context && d.viewport_available && log->previous.viewport_available &&
        !d.interacting && !log->previous.interacting &&
        (log->previous.viewport_x != d.viewport_x || log->previous.viewport_y != d.viewport_y ||
         log->previous.zoom != d.zoom)) {
        row(log, m, editor, d, "viewport.changed", "viewport.change", "", now);
    }
    snprintf(log->editor, sizeof log->editor, "%s", editor);
    log->previous = d;
    log->observed = true;
    if (m->count && m->previous_time != log->last_sample) {
        row(log, m, editor, d, "sample", "", "", now);
        log->last_sample = m->previous_time;
    }
}
