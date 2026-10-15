#include "app/editor_internal.h"

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
    for (int i = 0; i < app.expanded_count; i++) {
        if (!strcmp(p, app.expanded[i])) {
            return true;
        }
    }

    return false;
}

static SDL_EnumerationResult SDLCALL scan_entry(void *userdata, const char *dirname,
                                                const char *name) {
    (void)dirname;
    ScanContext *context = userdata;
    if (name[0] == '.' || app.tree_count == TREE_MAX) {
        return SDL_ENUM_CONTINUE;
    }

    char relative[PE_PATH], full[1024];
    int length = snprintf(relative, sizeof relative, "%s%s%s", context->relative,
                          *context->relative ? "/" : "", name);
    if (length >= (int)sizeof relative) {
        return SDL_ENUM_CONTINUE;
    }
    snprintf(full, sizeof full, "%s/%s", app.root, relative);
    SDL_PathInfo info;
    if (!SDL_GetPathInfo(full, &info)) {
        return SDL_ENUM_CONTINUE;
    }
    TreeRow *row = &app.tree[app.tree_count++];
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
    snprintf(path, sizeof path, "%s/%s", app.root, relative);
    ScanContext context = {relative, depth};
    SDL_EnumerateDirectory(path, scan_entry, &context);
}

void editor_tree_refresh(void) {
    app.tree_count = 0;
    scan("", 0);
    if (app.tree_scroll >= app.tree_count) {
        app.tree_scroll = 0;
    }
}

static void tree_toggle(const char *path) {
    for (int i = 0; i < app.expanded_count; i++) {
        if (!strcmp(app.expanded[i], path)) {
            memmove(app.expanded[i], app.expanded[i + 1],
                    (size_t)(app.expanded_count - i - 1) * PE_PATH);
            app.expanded_count--;
            editor_tree_refresh();
            return;
        }
    }
    if (app.expanded_count < 64) {
        snprintf(app.expanded[app.expanded_count++], PE_PATH, "%s", path);
        editor_tree_refresh();
    }
}

bool editor_suffix(const char *p, const char *s) {
    size_t n = strlen(p), m = strlen(s);
    return n >= m && !SDL_strcasecmp(p + n - m, s);
}

void editor_draw_tree(void) {
    /* Lift the theme surface slightly while keeping a panel-colored outer gutter. */
    SDL_FRect body = {app.panel_area.x + 6, app.panel_area.y + 6, fmaxf(0, app.panel_area.w - 12),
                      fmaxf(0, app.panel_area.h - 12)};
    /* Keep the refresh toolbar on the panel; only the node area is lifted. */
    /* The 25-pixel button starts at +4; leave six pixels below it. */
    SDL_FRect nodes = {body.x, body.y + 35, body.w, fmaxf(0, body.h - 35)};
    editor_box(nodes, editor_widget_background());
    if (icon_button(body.x + body.w - 36, body.y + 4, 32, ICON_REFRESH, "", "Refresh project files",
                    false, true)) {
        editor_tree_refresh();
    }
    app.tree_view.content = app.tree_count * 23.f;
    app.tree_view.viewport = fmaxf(0, nodes.h - 8);
    ui_scroll_clamp(&app.tree_view);
    UiRect track = {nodes.x + nodes.w - 12, nodes.y + 4, 8, app.tree_view.viewport};
    ui_scrollbar(&app.ui, UI_TREE_SCROLL, track, &app.tree_view, UI_AXIS_Y);
    app.tree_scroll = (int)(app.tree_view.offset / 23);
    UiRect tree_clip = {nodes.x + 4, nodes.y + 4, fmaxf(0, nodes.w - 20), app.tree_view.viewport};
    ui_clip_push(&app.ui, tree_clip);
    ui_apply_clip(app.renderer, &app.ui);
    int visible = (int)(app.tree_view.viewport / 23) + 2;
    for (int i = app.tree_scroll; i < app.tree_count && i < app.tree_scroll + visible; i++) {
        TreeRow row = app.tree[i];
        float y = tree_clip.y + i * 23 - app.tree_view.offset;
        char label[128];
        snprintf(label, sizeof label, "%.20s", row.name);
        editor_box((SDL_FRect){tree_clip.x, y, tree_clip.w, 23}, editor_tree_row_background(i));
        SDL_FRect r = {tree_clip.x, y, tree_clip.w, 22};
        bool hover = ui_hovered(&app.ui, (UiRect){r.x, r.y, r.w, r.h});
        if (hover) {
            editor_box(r, UI_HOVER);
        }
        float row_x = tree_clip.x + 2 + row.depth * 12;
        if (row.directory) {
            ui_icon(app.renderer, row.expanded ? ICON_DOWN : ICON_RIGHT, row_x, y + 5, 12,
                    UI_MUTED);
        }
        ui_icon(app.renderer,
                row.directory                     ? ICON_FOLDER
                : editor_suffix(row.path, ".png") ? ICON_IMAGE
                                                  : ICON_FILE,
                row_x + 16, y + 3, 16, row.directory ? UI_ACCENT : UI_MUTED);
        editor_text(row_x + 40, y + 7, label, UI_TEXT);
        if (app.click && hover && !app.playing) {
            if (row.directory) {
                tree_toggle(row.path);
                break;
            }
            if (editor_suffix(row.path, ".js")) {
                char path[1024];
                snprintf(path, sizeof path, "%s/%s", app.root, row.path);
                editor_request(REQUEST_SCRIPT_OPEN, path);
            } else if (editor_suffix(row.path, ".png")) {
                char path[1024];
                snprintf(path, sizeof path, "%s/%s", app.root, row.path);
                editor_request(REQUEST_IMPORT, path);
            } else if (editor_suffix(row.path, ".json") && strcmp(row.path, "project.json")) {
                char path[1024];
                snprintf(path, sizeof path, "%s/%s", app.root, row.path);
                editor_request(REQUEST_OPEN, path);
            }
        }
    }
    ui_clip_pop(&app.ui);
    ui_apply_clip(app.renderer, &app.ui);
    ui_draw_scrollbar(app.renderer, &app.ui, UI_TREE_SCROLL, track, &app.tree_view, UI_AXIS_Y);
}
