/** @file @brief Typographic controls stay outside the text model and font backend. */
#include "text.h"
#include "editors/vector/text/text.h"
#include "editors/vector/text/tool.h"
#include <errno.h>
#include <math.h>
#include <stdlib.h>
#include <string.h>

static const char *fields[] = {"size", "line_height", "letter_spacing", "columns", "column_gap"};

static bool number(PUIView *view, const char *field, float *out) {
    char id[64];
    snprintf(id, sizeof id, "text_%s", field);
    const char *text = pui_view_text(view, id);
    char *end;
    errno = 0;
    *out = text ? strtof(text, &end) : 0;
    return text && *text && end != text && !*end && !errno && isfinite(*out);
}

static void select_face(VectorText *text, const PUIFontEntry *font) {
    if (!font) {
        return;
    }
    snprintf(text->font, sizeof text->font, "%s", font->path);
    text->face = font->index;
}

bool vector_text_inspector_action(VectorTextInspector *ui, VectorDocument *d, unsigned id,
                                  PUIView *view, const char *action) {
    if (strncmp(action, "vector.text.", 12)) {
        return false;
    }
    VectorShape *s = d ? vector_rectangle_find(d, id) : NULL;
    if (!s || !s->text.enabled) {
        return true;
    }
    vector_text_tool_end(d);
    if (!strcmp(action, "vector.text.font_search")) {
        const char *search = pui_view_text(view, "text_font_search");
        snprintf(ui->search, sizeof ui->search, "%s", search ? search : "");
        ui->page = 0;
        ui->rebuild_fonts = true;
        return true;
    }
    if (!strcmp(action, "vector.text.font_page")) {
        ui->page++;
        ui->rebuild_fonts = true;
        return true;
    }
    VectorText after = s->text;
    const char *field = action + 12;
    bool valid = true;
    if (!strcmp(field, "font")) {
        const char *value = pui_view_value(view, "text_font");
        char *end = NULL;
        long n = value ? strtol(value, &end, 10) : -1;
        const PUIFontEntry *entry = end && !*end ? pui_fonts_entry(ui->fonts, (int)n) : NULL;
        valid = entry != NULL;
        if (valid) {
            select_face(&after, entry);
            after.bold = entry->bold;
            after.italic = entry->italic;
        }
    } else if (!strcmp(field, "bold") || !strcmp(field, "italic")) {
        bool bold = !strcmp(field, "bold");
        if (bold) {
            after.bold = pui_view_checked(view, "text_bold");
        } else {
            after.italic = pui_view_checked(view, "text_italic");
        }
        int current = pui_fonts_find(ui->fonts, after.font, after.face);
        int index = pui_fonts_style(ui->fonts, current, after.bold != 0, after.italic != 0);
        select_face(&after, pui_fonts_entry(ui->fonts, index));
        ui->rebuild_fonts = true;
    } else if (!strcmp(field, "alignment")) {
        const char *value = pui_view_value(view, "text_alignment");
        valid = value != NULL;
        if (value) {
            after.alignment = !strcmp(value, "center")    ? VECTOR_TEXT_CENTER
                              : !strcmp(value, "right")   ? VECTOR_TEXT_RIGHT
                              : !strcmp(value, "justify") ? VECTOR_TEXT_JUSTIFY
                                                          : VECTOR_TEXT_LEFT;
        }
    } else {
        float value = 0;
        valid = number(view, field, &value);
        if (!strcmp(field, "size")) {
            after.size = value;
        } else if (!strcmp(field, "line_height")) {
            after.line_height = value;
        } else if (!strcmp(field, "letter_spacing")) {
            after.letter_spacing = value;
        } else if (!strcmp(field, "column_gap")) {
            after.column_gap = value;
        } else if (!strcmp(field, "columns")) {
            valid &= value >= 1 && value <= 16 && value == floorf(value);
            after.columns = valid ? (unsigned)value : 0;
        } else {
            return false;
        }
    }
    bool changed = valid && vector_text_update(d, id, &after);
    ui->dirty = ~0u;
    pui_view_set_text(view, "status",
                      changed ? "Textformat aktualisiert"
                              : "Text gesperrt oder ungueltiger Formatwert");
    return true;
}

static PUILayout *font_choices(VectorTextInspector *ui, const VectorText *text) {
    PUILayout *layout = calloc(1, sizeof *layout);
    if (!layout) {
        return NULL;
    }
    layout->root = 0;
    layout->count = 1;
    PUINode *root = &layout->nodes[0];
    *root = (PUINode){
        .type = PUI_NODE_DROPDOWN, .height = 30, .enabled = true, .child = -1, .next = -1};
    snprintf(root->name, sizeof root->name, "text_font");
    root->id = pui_id(root->name);
    snprintf(root->action, sizeof root->action, "vector.text.font");
    int current = pui_fonts_find(ui->fonts, text->font, text->face);
    snprintf(root->value, sizeof root->value, "%d", current);
    int *matches = malloc((size_t)pui_fonts_count(ui->fonts) * sizeof *matches);
    if (!matches) {
        free(layout);
        return NULL;
    }
    int count = 0;
    for (int i = 0; i < pui_fonts_count(ui->fonts); i++) {
        const PUIFontEntry *e = pui_fonts_entry(ui->fonts, i);
        if (!*ui->search || SDL_strcasestr(e->label, ui->search)) {
            matches[count++] = i;
        }
    }
    const int page_size = 24;
    if (ui->page * page_size >= count) {
        ui->page = 0;
    }
    // Keep the selected saved face available even when searching another family/page.
    int visible[25], used = 1;
    visible[0] = current;
    for (int i = ui->page * page_size; i < count && i < (ui->page + 1) * page_size; i++) {
        if (matches[i] != current) {
            visible[used++] = matches[i];
        }
    }
    for (int i = 0; i < used; i++) {
        int index = layout->count++;
        PUINode *option = &layout->nodes[index];
        *option = (PUINode){.type = PUI_NODE_OPTION,
                            .enabled = true,
                            .child = -1,
                            .next = i + 1 < used ? index + 1 : -1};
        snprintf(option->name, sizeof option->name, "text_font_%d", visible[i]);
        option->id = pui_id(option->name);
        snprintf(option->value, sizeof option->value, "%d", visible[i]);
        snprintf(option->text, sizeof option->text, "%s",
                 pui_fonts_entry(ui->fonts, visible[i])->label);
    }
    free(matches);
    root->child = used ? 1 : -1;
    return layout;
}

bool vector_text_inspector_sync(VectorTextInspector *ui, VectorDocument *d, VectorShape *s,
                                PUIView *view, PUIContext *input, bool remounted, char *error,
                                size_t capacity) {
    bool active = s && s->text.enabled;

    bool switched = ui->id != (active ? s->id : 0) || remounted;
    if (remounted || ui->mounted != active) {
        if (!pui_view_replace_slot(view, input, "text_properties", active ? ui->layout : NULL,
                                   error, capacity)) {
            return false;
        }
        ui->mounted = active;
        ui->rebuild_fonts = true;
    }
    if (!active) {
        ui->id = 0;
        return true;
    }
    const VectorText *t = &s->text;
    if (switched) {
        ui->search[0] = 0;
        ui->page = 0;
        pui_view_set_text(view, "text_font_search", "");
    }
    if (switched || ui->rebuild_fonts || strcmp(t->font, ui->presented.font) ||
        t->face != ui->presented.face) {
        PUILayout *fonts = font_choices(ui, t);
        bool ok = fonts &&
                  pui_view_replace_slot(view, input, "text_font_choices", fonts, error, capacity);
        pui_layout_free(fonts);
        if (!ok) {
            return false;
        }
        ui->rebuild_fonts = false;
    }
    float values[] = {t->size, t->line_height, t->letter_spacing, t->columns, t->column_gap};
    float previous[] = {ui->presented.size, ui->presented.line_height, ui->presented.letter_spacing,
                        ui->presented.columns, ui->presented.column_gap};
    for (int i = 0; i < 5; i++) {
        if (switched || ui->dirty || values[i] != previous[i]) {
            char id[64], value[48];
            snprintf(id, sizeof id, "text_%s", fields[i]);
            snprintf(value, sizeof value, "%.7g", (double)values[i]);
            pui_view_set_text(view, id, value);
        }
    }
    pui_view_set_checked(view, "text_bold", t->bold != 0);
    pui_view_set_checked(view, "text_italic", t->italic != 0);
    const char *align[] = {"left", "center", "right", "justify"};
    pui_view_set_value(view, "text_alignment", align[t->alignment]);
    VectorTextLayout layout;
    vector_text_measure(ui->renderer, s, &layout);
    pui_view_set_text(view, "text_overflow",
                      layout.overflow ? "Textueberlauf: Rahmen vergroessern"
                                      : "Text passt in den Rahmen");
    ui->presented = *t;
    ui->id = s->id;
    ui->dirty = 0;
    (void)d;
    return true;
}
