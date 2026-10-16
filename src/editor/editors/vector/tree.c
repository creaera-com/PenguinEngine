#include "app/editor_internal.h"
#include "editors/vector/internal.h"

typedef struct {
    int board, shape, node, depth;
} VectorRow;

static bool expanded(uint32_t id) {
    for (int i = 0; i < VE_SHAPES; i++) {
        if (app.vector->view.expanded_paths[i] == id) {
            return true;
        }
    }
    return false;
}

static void toggle(uint32_t id) {
    VeView *view = &app.vector->view;
    for (int i = 0; i < VE_SHAPES; i++) {
        if (view->expanded_paths[i] == id) {
            view->expanded_paths[i] = 0;
            return;
        }
    }
    for (int i = 0; i < VE_SHAPES; i++) {
        if (!view->expanded_paths[i]) {
            view->expanded_paths[i] = id;
            return;
        }
    }
}

/* Count first so even a fully expanded document needs only its visible rows. */
static int collect(VectorRow *rows, int *depths) {
    VectorEditor *v = app.vector;
    int count = 0;
    for (int b = 0; b < VE_BOARDS; b++) {
        if (!v->document.boards[b].used) {
            continue;
        }
        if (rows) {
            rows[count] = (VectorRow){b, -1, -1, 0};
            depths[count] = 0;
        }
        count++;
        if (!v->view.expanded[b]) {
            continue;
        }
        for (int i = 0; i < VE_SHAPES; i++) {
            VeShape *s = &v->document.shapes[i];
            if (!s->used || s->board != b) {
                continue;
            }
            if (rows) {
                rows[count] = (VectorRow){b, i, -1, 1};
                depths[count] = 1;
            }
            count++;
            if (s->kind != VE_PATH || !expanded(s->id)) {
                continue;
            }
            for (int n = 0; n < s->count; n++) {
                if (rows) {
                    rows[count] = (VectorRow){b, i, n, 2};
                    depths[count] = 2;
                }
                count++;
            }
        }
    }
    return count;
}

static bool row_button(UiId id, UiRect rect, UiIcon icon, const char *label, const char *hint,
                       bool selected, Uint32 background) {
    UiTheme theme = ui_theme;
    theme.surface = theme.panel = background;
    bool hover = ui_hovered(&app.ui, rect);
    bool enabled = !editor_edit_pending();
    editor_register_ui_hit(rect);
    ui_draw_button_themed(app.renderer, &theme, (SDL_FRect){rect.x, rect.y, rect.w, rect.h}, icon,
                          label, selected, enabled, hover);
    if (hover) {
        static char tooltip[160];
        snprintf(tooltip, sizeof tooltip, "%s", hint);
        editor_tooltip = tooltip;
    }
    return ui_button(&app.ui, id, rect, enabled);
}

static void draw_row(VectorRow row, int index, const int *depths, int count, UiRect area) {
    VectorEditor *v = app.vector;
    VeShape *s = row.shape >= 0 ? &v->document.shapes[row.shape] : NULL;
    bool children = row.node < 0 && (!s || (s->kind == VE_PATH && s->count));
    bool open = s ? expanded(s->id) : v->view.expanded[row.board] != 0;
    float indent = fminf(row.depth * 20.f, fmaxf(0, area.w - 80));
    float x = area.x + indent;
    Uint32 background = editor_tree_row_background(index);
    editor_box((SDL_FRect){area.x, area.y, area.w, area.h}, background);
    UiId scope = s ? ui_id("vector_object") ^ s->id : ui_id("vector_board") ^ (UiId)row.board;
    if (children &&
        row_button(scope ^ ui_id("expand"), (UiRect){x, area.y, 20, 25},
                   open ? ICON_DOWN : ICON_RIGHT, "", "Expand / collapse", false, background)) {
        if (s) {
            toggle(s->id);
        } else {
            v->view.expanded[row.board] = !open;
        }
    }
    const char *label = s ? s->name : v->document.boards[row.board].name;
    const char *hint = s ? "Select object; Shift: multi-select" : "Select artboard";
    UiIcon icon = !s                      ? ICON_IMAGE
                  : s->kind == VE_RECT    ? ICON_RECTANGLE
                  : s->kind == VE_ELLIPSE ? ICON_COLLISION
                                          : ICON_EDIT;
    bool selected = s ? (v->view.selected & (UINT64_C(1) << row.shape)) != 0
                      : v->view.board == row.board && !v->view.selected;
    char detail[128];
    if (row.node >= 0) {
        VeNode *node = &s->nodes[row.node];
        const char *names[] = {"Move", "Line", "Bezier", "Close"};
        if (node->command == VE_CLOSE) {
            snprintf(detail, sizeof detail, "%d: Close", row.node);
        } else {
            snprintf(detail, sizeof detail, "%d: %s (%.1f, %.1f)", row.node, names[node->command],
                     node->p.x, node->p.y);
        }
        label = detail;
        hint = detail;
        selected = selected && v->view.tool == VT_NODE && v->view.node == row.node;
        scope ^= ui_id("vector_segment") + (UiId)row.node * 16777619u;
        icon = node->command == VE_CUBIC ? ICON_EDIT : ICON_SELECT;
    }
    if (row_button(scope ^ ui_id("select"),
                   (UiRect){x + 22, area.y, fmaxf(0, area.w - indent - 22), 25}, icon, label, hint,
                   selected, background)) {
        v->view.board = row.board;
        v->view.node = -1;
        if (!s) {
            v->view.selected = 0;
        } else if (row.node >= 0) {
            v->view.selected = UINT64_C(1) << row.shape;
            v->view.tool = VT_NODE;
            v->view.node = row.node;
        } else if (SDL_GetModState() & SDL_KMOD_SHIFT) {
            v->view.selected ^= UINT64_C(1) << row.shape;
        } else {
            v->view.selected = UINT64_C(1) << row.shape;
        }
        vector_sync();
    }
    ui_draw_tree_guides(app.renderer, depths, count, index, area, 20, 10, fmaxf(0, area.w - 80),
                        x + (children ? -2 : 28), UI_DISABLED);
}

void vector_draw_tree(void) {
    SDL_FRect body = {app.panel_area.x + 6, app.panel_area.y + 6, fmaxf(0, app.panel_area.w - 12),
                      fmaxf(0, app.panel_area.h - 12)};
    editor_box(body, editor_widget_background());
    int count = collect(NULL, NULL);
    VectorRow *rows = malloc((size_t)count * sizeof *rows);
    int *depths = malloc((size_t)count * sizeof *depths);
    if (!rows || !depths) {
        free(rows);
        free(depths);
        return;
    }
    collect(rows, depths);
    app.scene_view.content = count * 26.f;
    app.scene_view.viewport = fmaxf(0, body.h - 8);
    ui_scroll_clamp(&app.scene_view);
    UiRect track = {body.x + body.w - 12, body.y + 4, 8, app.scene_view.viewport};
    ui_scrollbar(&app.ui, UI_SCENE_SCROLL, track, &app.scene_view, UI_AXIS_Y);
    UiRect clip = {body.x + 4, body.y + 4, fmaxf(0, body.w - 20), app.scene_view.viewport};
    ui_clip_push(&app.ui, clip);
    ui_apply_clip(app.renderer, &app.ui);
    for (int i = 0; i < count; i++) {
        float y = clip.y + i * 26 - app.scene_view.offset;
        if (y + 26 <= clip.y || y >= clip.y + clip.h) {
            continue;
        }
        draw_row(rows[i], i, depths, count, (UiRect){clip.x, y, clip.w, 26});
    }
    ui_clip_pop(&app.ui);
    ui_apply_clip(app.renderer, &app.ui);
    ui_draw_scrollbar(app.renderer, &app.ui, UI_SCENE_SCROLL, track, &app.scene_view, UI_AXIS_Y);
    free(rows);
    free(depths);
}
