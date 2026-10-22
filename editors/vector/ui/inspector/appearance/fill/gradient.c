/** @file @brief Fill type, stop navigation and color edits; persistence lives with shape values. */
#include "gradient.h"
#include "editors/vector/gradient/tool.h"
#include "editors/vector/object/commands.h"
#include "editors/vector/workspace/selection.h"
#include <errno.h>
#include <math.h>
#include <stdlib.h>
#include <string.h>
static const char *kinds[] = {"solid", "linear", "radial", "conical", "reflected"};

bool vector_gradient_inspector_action(VectorGradientInspector *ui, VectorDocument *d, unsigned id,
                                      PUIView *view, const char *action) {
    if (strncmp(action, "vector.gradient.", 16)) {
        return false;
    }
    VectorShape *source = d ? vector_rectangle_find(d, id) : NULL;
    if (!vector_gradient_eligible(source)) {
        return true;
    }
    VectorShape value = *source;
    VectorGradient *g = &value.gradient;
    const char *field = action + 16;
    unsigned count = g->count ? g->count : 2;
    if (ui->stop >= count) {
        ui->stop = 0;
    }
    if (!strcmp(field, "previous") || !strcmp(field, "next")) {
        ui->stop = (ui->stop + (!strcmp(field, "next") ? 1 : count - 1)) % count;
        ui->dirty = true;
        return true;
    }
    bool valid = true;
    if (!strcmp(field, "kind")) {
        const char *mode = pui_view_value(view, "gradient_type");
        if (!g->count) {
            *g = vector_gradient_default(source->fill);
        }
        valid = false;
        for (unsigned i = 0; i < 5; i++) {
            if (mode && !strcmp(mode, kinds[i])) {
                g->kind = i;
                valid = true;
                break;
            }
        }
        if (g->kind == VECTOR_GRADIENT_RADIAL || g->kind == VECTOR_GRADIENT_CONICAL ||
            g->kind == VECTOR_GRADIENT_REFLECTED) {
            if (!source->gradient.count) {
                g->x1 = .5f;
                g->y1 = .5f;
                g->x2 = 1;
                g->y2 = .5f;
            }
        }
    } else {
        if (!g->count) {
            *g = vector_gradient_default(source->fill);
        }
        if (!strcmp(field, "add")) {
            int at = vector_gradient_add_stop(g);
            valid = at >= 0;
            if (valid) {
                ui->stop = (unsigned)at;
            }
        } else if (!strcmp(field, "remove")) {
            valid = vector_gradient_remove_stop(g, ui->stop);
            if (ui->stop >= g->count) {
                ui->stop = g->count - 1;
            }
        } else if (!strcmp(field, "color")) {
            valid = pui_view_color(view, "gradient_stop_color", &g->stops[ui->stop].color);
        } else if (!strcmp(field, "position")) {
            const char *text = pui_view_text(view, "gradient_stop_position");
            char *end = NULL;
            errno = 0;
            float p = text ? strtof(text, &end) : NAN;
            valid = text && *text && end != text && !*end && !errno && isfinite(p);
            if (valid) {
                g->stops[ui->stop].position = p / 100;
            }
        } else if (!strcmp(field, "reverse")) {
            VectorGradient before = *g;
            for (unsigned i = 0; i < g->count; i++) {
                g->stops[i] = before.stops[g->count - 1 - i];
                g->stops[i].position = 1 - g->stops[i].position;
            }
        } else {
            return false;
        }
    }
    if (d->gradient_tool.dragging) {
        vector_gradient_tool_cancel(d);
        d->gradient_tool.enabled = true;
    }
    vector_selection_cancel(d);
    bool changed = valid && vector_shape_update(d, &value);
    ui->dirty = true;
    pui_view_set_text(
        view, "status",
        changed ? "Fuellung aktualisiert"
                : "Unveraendert: gesperrt, ungueltiger Stopp oder Grenze (2 bis 8 Stopps)");
    return true;
}

void vector_gradient_inspector_sync(VectorGradientInspector *ui, PUIView *view,
                                    const VectorGradient *g, const VectorGradient *old,
                                    bool switched) {
    if (switched) {
        ui->stop = 0;
    }
    VectorGradient fallback = vector_gradient_default(0);
    const VectorGradient *settings = g->count ? g : &fallback;
    if (ui->stop >= settings->count) {
        ui->stop = 0;
    }
    if (switched || ui->dirty || memcmp(g, old, sizeof *g)) {
        pui_view_set_value(view, "gradient_type", kinds[g->kind]);
        char text[64];
        snprintf(text, sizeof text, "Farbstopp %u / %u", ui->stop + 1, settings->count);
        pui_view_set_text(view, "gradient_stop_label", text);
        snprintf(text, sizeof text, "%.7g", (double)(settings->stops[ui->stop].position * 100));
        pui_view_set_text(view, "gradient_stop_position", text);
        pui_view_set_color(view, "gradient_stop_color", settings->stops[ui->stop].color);
    }
    ui->dirty = false;
}
