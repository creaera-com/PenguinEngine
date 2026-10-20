/** @file @brief Small property editor avoids a huge form: choose property, then its two axis steps.
 */
#include "repeat.h"
#include "editors/vector/artboard/tool.h"
#include "editors/vector/repeat/repeat.h"
#include <errno.h>
#include <math.h>
#include <stdlib.h>
#include <string.h>
static const char *properties[] = {"width",    "height",   "angle",    "stroke",
                                   "fill_h",   "fill_s",   "fill_l",   "fill_a",
                                   "stroke_h", "stroke_s", "stroke_l", "stroke_a"};

static void number(PUIView *view, const char *id, float value) {
    char text[40];
    snprintf(text, sizeof text, "%.7g", value);
    pui_view_set_text(view, id, text);
}

static void show(VectorRepeatUI *ui, PUIView *view) {
    number(view, "repeat_columns", ui->draft.columns);
    number(view, "repeat_rows", ui->draft.rows);
    number(view, "repeat_gap_x", ui->draft.gap_x);
    number(view, "repeat_gap_y", ui->draft.gap_y);
    number(view, "repeat_column_step", ui->draft.column.value[ui->property]);
    number(view, "repeat_row_step", ui->draft.row.value[ui->property]);
    pui_view_set_value(view, "repeat_mode", ui->linked ? "linked" : "detached");
    pui_view_set_value(view, "repeat_property", properties[ui->property]);
    char text[120];
    snprintf(text, sizeof text, "%u Exemplare inkl. Original", ui->draft.columns * ui->draft.rows);
    pui_view_set_text(view, "repeat_count", text);
}

static bool read_number(PUIView *view, const char *id, float *out) {
    const char *text = pui_view_text(view, id);
    if (!text || !*text) {
        return false;
    }
    char *end;
    errno = 0;
    float n = strtof(text, &end);
    if (errno || end == text || *end || !isfinite(n)) {
        return false;
    }
    *out = n;
    return true;
}

void vector_repeat_ui_sync(VectorRepeatUI *ui, VectorDocument *d, PUIView *view) {
    VectorShape *s = vector_repeat_source(d);
    if (!ui->initialized || ui->document != (d ? d->id : 0) || ui->source != (s ? s->id : 0) ||
        (s && memcmp(&ui->presented, &s->repeat, sizeof s->repeat))) {
        ui->document = d ? d->id : 0;
        ui->source = s ? s->id : 0;
        ui->presented = s ? s->repeat : (VectorRepeat){0};
        ui->draft = s && s->repeat.enabled ? s->repeat : vector_repeat_default();
        ui->linked = true;
        ui->initialized = true;
        show(ui, view);
    }
    pui_view_set_text(view, "repeat_source",
                      s ? s->name : "Ein einzelnes freies Objekt auswaehlen");
}

bool vector_repeat_ui_action(VectorRepeatUI *ui, VectorDocument *d, PUIView *view,
                             const char *action) {
    if (strncmp(action, "vector.repeat.", 14)) {
        return false;
    }
    vector_repeat_ui_sync(ui, d, view);
    if (!strcmp(action, "vector.repeat.property")) {
        const char *value = pui_view_value(view, "repeat_property");
        for (unsigned i = 0; i < VECTOR_REPEAT_PROPERTIES; i++) {
            if (value && !strcmp(value, properties[i])) {
                ui->property = i;
                break;
            }
        }
        show(ui, view);
        return true;
    }
    if (!strcmp(action, "vector.repeat.mode")) {
        const char *value = pui_view_value(view, "repeat_mode");
        ui->linked = !value || strcmp(value, "detached");
        return true;
    }
    if (!strcmp(action, "vector.repeat.apply") || !strcmp(action, "vector.repeat.detach") ||
        !strcmp(action, "vector.repeat.remove")) {
        if (d) {
            vector_artboard_tool_cancel(d);
        }
        bool changed = d && (!strcmp(action, "vector.repeat.apply")
                                 ? vector_repeat_apply(d, &ui->draft, ui->linked)
                             : !strcmp(action, "vector.repeat.detach") ? vector_repeat_detach(d)
                                                                       : vector_repeat_remove(d));
        pui_view_set_text(
            view, "status",
            changed
                ? "Wiederholung aktualisiert"
                : "Keine Aenderung: ein freies Objekt waehlen; unabh. Kopien insgesamt max. 64");
        if (changed) {
            ui->initialized = false;
        }
        return true;
    }
    VectorRepeat before = ui->draft;
    const char *id = NULL;
    float *target = NULL, n;
    bool columns = !strcmp(action, "vector.repeat.columns"),
         rows = !strcmp(action, "vector.repeat.rows");
    if (columns || rows) {
        id = columns ? "repeat_columns" : "repeat_rows";
        if (read_number(view, id, &n) && n >= 1 && n <= 64 && floorf(n) == n) {
            if (columns) {
                ui->draft.columns = (unsigned)n;
            } else {
                ui->draft.rows = (unsigned)n;
            }
        } else {
            pui_view_set_text(view, "status", "Anzahl: ganze Zahl von 1 bis 64");
        }
    } else {
        if (!strcmp(action, "vector.repeat.gap_x")) {
            id = "repeat_gap_x";
            target = &ui->draft.gap_x;
        }
        if (!strcmp(action, "vector.repeat.gap_y")) {
            id = "repeat_gap_y";
            target = &ui->draft.gap_y;
        }
        if (!strcmp(action, "vector.repeat.column_step")) {
            id = "repeat_column_step";
            target = &ui->draft.column.value[ui->property];
        }
        if (!strcmp(action, "vector.repeat.row_step")) {
            id = "repeat_row_step";
            target = &ui->draft.row.value[ui->property];
        }
        if (target && read_number(view, id, &n)) {
            *target = n;
        }
    }
    if (!vector_repeat_valid(&ui->draft)) {
        ui->draft = before;
        pui_view_set_text(view, "status", "Ungueltige Werte oder mehr als 1024 Exemplare");
    }
    show(ui, view);
    return true;
}
