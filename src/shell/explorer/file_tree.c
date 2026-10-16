#include "application/internal.h"

typedef struct {
    const char *relative;
    int depth;
} ScanContext;

static bool is_expanded(const char *p);
static SDL_EnumerationResult SDLCALL scan_entry(void *userdata, const char *dirname,
                                                const char *name);
static void scan(const char *relative, int depth);
static void tree_toggle(const char *path);

static bool is_expanded(const char *p) {
    for (int i = 0; i < app.shell.expanded_count; i++) {
        if (!strcmp(p, app.shell.expanded[i])) {
            return true;
        }
    }

    return false;
}

static SDL_EnumerationResult SDLCALL scan_entry(void *userdata, const char *dirname,
                                                const char *name) {
    (void)dirname;
    ScanContext *context = userdata;
    if (name[0] == '.' || app.shell.tree_count == TREE_MAX) {
        return SDL_ENUM_CONTINUE;
    }

    char relative[PE_PATH], full[1024];
    int length = snprintf(relative, sizeof relative, "%s%s%s", context->relative,
                          *context->relative ? "/" : "", name);
    if (length >= (int)sizeof relative) {
        return SDL_ENUM_CONTINUE;
    }
    snprintf(full, sizeof full, "%s/%s", app.project.root, relative);
    SDL_PathInfo info;
    if (!SDL_GetPathInfo(full, &info)) {
        return SDL_ENUM_CONTINUE;
    }
    TreeRow *row = &app.shell.tree[app.shell.tree_count++];
    snprintf(row->path, sizeof row->path, "%s", relative);
    snprintf(row->name, sizeof row->name, "%.127s", name);
    row->depth = context->depth;
    row->directory = info.type == SDL_PATHTYPE_DIRECTORY;
    row->expanded = is_expanded(relative);
    if (row->directory && row->expanded && context->depth < 8) {
        scan(relative, context->depth + 1);
    }

    return SDL_ENUM_CONTINUE;
}

static void scan(const char *relative, int depth) {
    char path[1024];
    snprintf(path, sizeof path, "%s/%s", app.project.root, relative);
    ScanContext context = {relative, depth};
    SDL_EnumerateDirectory(path, scan_entry, &context);
}

void editor_tree_refresh(void) {
    app.shell.tree_count = 0;
    scan("", 0);
    if (app.shell.tree_scroll >= app.shell.tree_count) {
        app.shell.tree_scroll = 0;
    }
}

static void tree_toggle(const char *path) {
    for (int i = 0; i < app.shell.expanded_count; i++) {
        if (!strcmp(app.shell.expanded[i], path)) {
            memmove(app.shell.expanded[i], app.shell.expanded[i + 1],
                    (size_t)(app.shell.expanded_count - i - 1) * PE_PATH);
            app.shell.expanded_count--;
            editor_tree_refresh();
            return;
        }
    }
    if (app.shell.expanded_count < 64) {
        snprintf(app.shell.expanded[app.shell.expanded_count++], PE_PATH, "%s", path);
        editor_tree_refresh();
    }
}

bool editor_suffix(const char *p, const char *s) {
    size_t n = strlen(p), m = strlen(s);
    return n >= m && !SDL_strcasecmp(p + n - m, s);
}

void editor_draw_tree(void) {
    /* Lift the theme surface slightly while keeping a panel-colored outer gutter. */
    SDL_FRect body = {app.shell.panel_area.x + 6, app.shell.panel_area.y + 6,
                      fmaxf(0, app.shell.panel_area.w - 12), fmaxf(0, app.shell.panel_area.h - 12)};
    /* Keep the refresh toolbar on the panel; only the node area is lifted. */
    /* The 25-pixel button starts at +4; leave six pixels below it. */
    SDL_FRect nodes = {body.x, body.y + 35, body.w, fmaxf(0, body.h - 35)};
    editor_box(nodes, editor_widget_background());
    if (icon_button(body.x + body.w - 36, body.y + 4, 32, ICON_REFRESH, "", "Refresh project files",
                    false, true)) {
        editor_tree_refresh();
    }
    app.shell.tree_view.content = app.shell.tree_count * 23.f;
    app.shell.tree_view.viewport = fmaxf(0, nodes.h - 8);
    ui_scroll_clamp(&app.shell.tree_view);
    UiRect track = {nodes.x + nodes.w - 12, nodes.y + 4, 8, app.shell.tree_view.viewport};
    ui_scrollbar(&app.shell.ui, UI_TREE_SCROLL, track, &app.shell.tree_view, UI_AXIS_Y);
    app.shell.tree_scroll = (int)(app.shell.tree_view.offset / 23);
    UiRect tree_clip = {nodes.x + 4, nodes.y + 4, fmaxf(0, nodes.w - 20),
                        app.shell.tree_view.viewport};
    ui_clip_push(&app.shell.ui, tree_clip);
    ui_apply_clip(app.platform.renderer, &app.shell.ui);
    int depths[TREE_MAX];
    for (int i = 0; i < app.shell.tree_count; i++) {
        depths[i] = app.shell.tree[i].depth;
    }
    int visible = (int)(app.shell.tree_view.viewport / 23) + 2;
    for (int i = app.shell.tree_scroll;
         i < app.shell.tree_count && i < app.shell.tree_scroll + visible; i++) {
        TreeRow row = app.shell.tree[i];
        float y = tree_clip.y + i * 23 - app.shell.tree_view.offset;
        char label[128];
        snprintf(label, sizeof label, "%.20s", row.name);
        editor_box((SDL_FRect){tree_clip.x, y, tree_clip.w, 23}, editor_tree_row_background(i));
        SDL_FRect r = {tree_clip.x, y, tree_clip.w, 22};
        bool hover = ui_hovered(&app.shell.ui, (UiRect){r.x, r.y, r.w, r.h});
        if (hover) {
            editor_box(r, UI_HOVER);
        }
        float row_x = tree_clip.x + 2 + row.depth * 12;
        if (row.directory) {
            ui_icon(app.platform.renderer, row.expanded ? ICON_DOWN : ICON_RIGHT, row_x, y + 5, 12,
                    UI_MUTED);
        }
        ui_icon(app.platform.renderer,
                row.directory                     ? ICON_FOLDER
                : editor_suffix(row.path, ".png") ? ICON_IMAGE
                                                  : ICON_FILE,
                row_x + 16, y + 3, 16, row.directory ? UI_ACCENT : UI_MUTED);
        editor_text(row_x + 40, y + 7, label, UI_TEXT);
        ui_draw_tree_guides(app.platform.renderer, depths, app.shell.tree_count, i,
                            (UiRect){tree_clip.x + 2, y, tree_clip.w - 2, 23}, 12, 6, 8 * 12,
                            row_x + (row.directory ? -2 : 14), UI_DISABLED);
        if (app.shell.click && hover && !editor_playing()) {
            if (row.directory) {
                tree_toggle(row.path);
                break;
            }
            if (editor_suffix(row.path, ".svg")) {
                char path[1024];
                snprintf(path, sizeof path, "%s/%s", app.project.root, row.path);
                editor_request(REQUEST_VECTOR_OPEN, path);
            } else if (editor_suffix(row.path, ".js")) {
                char path[1024];
                snprintf(path, sizeof path, "%s/%s", app.project.root, row.path);
                editor_request(REQUEST_SCRIPT_OPEN, path);
            } else if (editor_suffix(row.path, ".png")) {
                char path[1024];
                snprintf(path, sizeof path, "%s/%s", app.project.root, row.path);
                editor_request(REQUEST_IMPORT, path);
            } else if (editor_suffix(row.path, ".json") && strcmp(row.path, "project.json")) {
                char path[1024];
                snprintf(path, sizeof path, "%s/%s", app.project.root, row.path);
                editor_request(REQUEST_OPEN, path);
            }
        }
    }
    ui_clip_pop(&app.shell.ui);
    ui_apply_clip(app.platform.renderer, &app.shell.ui);
    ui_draw_scrollbar(app.platform.renderer, &app.shell.ui, UI_TREE_SCROLL, track,
                      &app.shell.tree_view, UI_AXIS_Y);
}
