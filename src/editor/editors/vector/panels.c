#include "app/editor_internal.h"
#include "editors/vector/internal.h"

void vector_draw_tools(SDL_FRect area) {
    editor_draw_yaml_toolbar(area);
}

static void number(float x, float y, float w, const char *key, float value, Field field,
                   bool enabled) {
    char text[64];
    snprintf(text, sizeof text, "%.2f", value);
    UiPropertyRow row = ui_property_row((UiRect){x, y, w, 25}, .42f, 8);
    editor_text(row.label.x, row.label.y + 10, key, UI_MUTED);
    editor_draw_input(field, text, text, row.value, enabled);
}

static void color_field(float x, float y, float w, const char *key, uint32_t rgba, Field field) {
    char text[16];
    snprintf(text, sizeof text, "%08X", rgba);
    UiPropertyRow row = ui_property_row((UiRect){x, y, w, 25}, .42f, 8);
    editor_text(row.label.x, row.label.y + 10, key, UI_MUTED);
    editor_draw_input(field, text, text, row.value, true);
}

void vector_draw_inspector(void) {
    VectorEditor *v = app.vector;
    float x = app.panel_area.x + 9, y = app.panel_area.y + 12 - app.inspector_view.offset,
          w = app.panel_area.w - 32;
    UiRect clip = {app.panel_area.x + 5, app.panel_area.y + 8, app.panel_area.w - 18,
                   app.panel_area.h - 16};
    ui_clip_push(&app.ui, clip);
    ui_apply_clip(app.renderer, &app.ui);
    int index = vector_first(), n = vector_selected_count();
    VeShape *s = index >= 0 ? &v->document.shapes[index] : NULL;
    VeBoard *b = &v->document.boards[v->view.board];
    bool single = n <= 1;
    char label[128];
    editor_text(x, y + 4, s ? (n > 1 ? "MULTIPLE OBJECTS" : "OBJECT") : "ARTBOARD", UI_ACCENT);
    y += 26;
    editor_draw_input(FIELD_V_NAME, s ? s->name : b->name, s ? s->name : b->name,
                      (UiRect){x, y, w, 25}, single);
    y += 35;
    number(x, y, w, "X", s ? s->x : b->x, FIELD_V_X, single);
    y += 29;
    number(x, y, w, "Y", s ? s->y : b->y, FIELD_V_Y, single);
    y += 29;
    number(x, y, w, "Width", s ? s->w : b->w, FIELD_V_W, single);
    y += 29;
    number(x, y, w, "Height", s ? s->h : b->h, FIELD_V_H, single);
    y += 38;
    if (s) {
        if (icon_button(x, y, w, s->fill_enabled ? ICON_CHECK : ICON_CLOSE, "Fill", "Toggle fill",
                        false, true)) {
            bool enabled = !s->fill_enabled;
            for (int i = 0; i < VE_SHAPES; i++) {
                if (v->view.selected & (UINT64_C(1) << i)) {
                    v->document.shapes[i].fill_enabled = enabled;
                }
            }
            vector_commit();
        }
        y += 30;
        color_field(x, y, w, "RGBA", s->fill, FIELD_V_FILL);
        y += 38;
        if (icon_button(x, y, w, s->stroke_enabled ? ICON_CHECK : ICON_CLOSE, "Stroke",
                        "Toggle stroke", false, true)) {
            bool enabled = !s->stroke_enabled;
            for (int i = 0; i < VE_SHAPES; i++) {
                if (v->view.selected & (UINT64_C(1) << i)) {
                    v->document.shapes[i].stroke_enabled = enabled;
                }
            }
            vector_commit();
        }
        y += 30;
        color_field(x, y, w, "RGBA", s->stroke, FIELD_V_STROKE);
        y += 29;
        number(x, y, w, "Border px", s->stroke_width, FIELD_V_STROKE_WIDTH, true);
        y += 37;
        if (button(x, y, w, "Convert to path", false, s->kind != VE_PATH)) {
            vector_action("vector.convert");
        }
        y += 31;
        if (s->kind == VE_PATH) {
            snprintf(label, sizeof label, "Nodes: %d | selected: %d", s->count, v->view.node);
            editor_text(x, y + 8, label, UI_MUTED);
            y += 27;
            if (button(x, y, w, "Make segment a curve", false, v->view.node > 0)) {
                vector_action("vector.curve");
            }
            y += 31;
            if (button(x, y, w, "Split selected segment", false, v->view.node > 0)) {
                vector_action("vector.split");
            }
            y += 31;
            if (button(x, y, w, "Close path", false,
                       s->count > 2 && s->nodes[s->count - 1].command != VE_CLOSE)) {
                vector_action("vector.close_path");
            }
            y += 31;
        }
    } else {
        color_field(x, y, w, "Background", b->background, FIELD_V_BACKGROUND);
        y += 34;
        editor_text(x, y + 8, "Units: px", UI_MUTED);
        y += 30;
    }
    editor_text(x, y + 8, "Shift+click: multi-select", UI_MUTED);
    y += 24;
    editor_text(x, y + 8, "Alt+drag: move artboard", UI_MUTED);
    y += 24;
    editor_text(x, y + 8, "MMB: pan | Wheel: zoom", UI_MUTED);
    y += 30;
    ui_clip_pop(&app.ui);
    ui_apply_clip(app.renderer, &app.ui);
    app.inspector_view.content = y + app.inspector_view.offset - app.panel_area.y + 8;
    app.inspector_view.viewport = fmaxf(0, clip.h);
    ui_scroll_clamp(&app.inspector_view);
    UiRect track = {app.panel_area.x + app.panel_area.w - 15, clip.y, 8, clip.h};
    ui_scrollbar(&app.ui, UI_INSPECTOR_SCROLL, track, &app.inspector_view, UI_AXIS_Y);
    ui_draw_scrollbar(app.renderer, &app.ui, UI_INSPECTOR_SCROLL, track, &app.inspector_view,
                      UI_AXIS_Y);
}

void vector_draw_tree(void) {
    VectorEditor *v = app.vector;
    UiRect clip = {app.panel_area.x + 8, app.panel_area.y + 8, app.panel_area.w - 24,
                   app.panel_area.h - 16};
    ui_clip_push(&app.ui, clip);
    ui_apply_clip(app.renderer, &app.ui);
    float y = clip.y - app.scene_view.offset;
    int row = 0;
    for (int b = 0; b < VE_BOARDS; b++) {
        VeBoard *board = &v->document.boards[b];
        if (!board->used) {
            continue;
        }
        app.widget_scope = ui_id("vector_board") ^ (UiId)b;
        editor_box((SDL_FRect){clip.x, y, clip.w, 26}, editor_tree_row_background(row++));
        bool open = v->view.expanded[b] != 0;
        if (icon_button(clip.x, y, 20, open ? ICON_DOWN : ICON_RIGHT, "", "Expand artboard", false,
                        true)) {
            v->view.expanded[b] = !open;
        }
        if (icon_button(clip.x + 22, y, clip.w - 22, ICON_IMAGE, board->name, "Select artboard",
                        v->view.board == b && !v->view.selected, true)) {
            v->view.board = b;
            v->view.selected = 0;
            vector_sync();
        }
        y += 26;
        if (!open) {
            continue;
        }
        for (int i = 0; i < VE_SHAPES; i++) {
            VeShape *s = &v->document.shapes[i];
            if (!s->used || s->board != b) {
                continue;
            }
            app.widget_scope = ui_id("vector_object") ^ s->id;
            editor_box((SDL_FRect){clip.x, y, clip.w, 26}, editor_tree_row_background(row++));
            if (icon_button(clip.x + 18, y, clip.w - 18,
                            s->kind == VE_RECT      ? ICON_RECTANGLE
                            : s->kind == VE_ELLIPSE ? ICON_COLLISION
                                                    : ICON_EDIT,
                            s->name, "Select object", (v->view.selected & (UINT64_C(1) << i)) != 0,
                            true)) {
                if (SDL_GetModState() & SDL_KMOD_SHIFT) {
                    v->view.selected ^= UINT64_C(1) << i;
                } else {
                    v->view.selected = UINT64_C(1) << i;
                }
                v->view.board = b;
                v->view.node = -1;
                vector_sync();
            }
            y += 26;
        }
    }
    app.widget_scope = 0;
    ui_clip_pop(&app.ui);
    ui_apply_clip(app.renderer, &app.ui);
    app.scene_view.content = y + app.scene_view.offset - clip.y;
    app.scene_view.viewport = clip.h;
    ui_scroll_clamp(&app.scene_view);
    UiRect track = {app.panel_area.x + app.panel_area.w - 14, clip.y, 8, clip.h};
    ui_scrollbar(&app.ui, UI_SCENE_SCROLL, track, &app.scene_view, UI_AXIS_Y);
    ui_draw_scrollbar(app.renderer, &app.ui, UI_SCENE_SCROLL, track, &app.scene_view, UI_AXIS_Y);
}
