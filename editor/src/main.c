#include "penguin/render.h"
#include "penguin/session.h"
#include "penguin/ui_layout.h"
#include "penguin/ui_sdl.h"
#include "script_editor.h"
#include <SDL3/SDL_main.h>
#include <errno.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define LEFT ((int)app.left_width)
#define RIGHT ((int)app.right_width)
#define TOP 50
#define TABS 32
#define BOTTOM 28
#define PANEL_GAP (app.gap)
#define GLOBAL_NAV_X 232
#define SIDE_TOP (TOP + TABS)
#define TREE_MAX 256
#define UI_SPLIT_LEFT 0x10001u
#define UI_SPLIT_RIGHT 0x10002u
#define UI_TREE_SCROLL 0x10003u
#define UI_INSPECTOR_SCROLL 0x10004u
#define UI_SCRIPT_VSCROLL 0x10005u
#define UI_SCRIPT_HSCROLL 0x10006u
#define UI_SCRIPT_DOCUMENT 0x10007u
#define UI_MAP_VIEW 0x10008u
#define UI_SCRIPT_VIEW 0x10009u
#define UI_EXPLORER_TAB 0x1000au
#define UI_SCENE_TAB 0x1000bu
#define UI_INSPECTOR_TAB 0x1000cu
#define UI_INFO_TAB 0x1000du
#define UI_SCENE_SCROLL 0x1000eu
#define UI_INFO_SCROLL 0x1000fu
typedef enum { BRUSH, ERASE, RECTANGLE, FILL, SELECT } Tool;
typedef enum {
    FIELD_NONE,
    FIELD_IMPORT,
    FIELD_TILESIZE,
    FIELD_LAYER,
    FIELD_X,
    FIELD_Y,
    FIELD_W,
    FIELD_H
} Field;
typedef enum {
    REQUEST_NONE,
    REQUEST_QUIT,
    REQUEST_OPEN,
    REQUEST_SCRIPT_OPEN,
    REQUEST_IMPORT
} Request;
typedef struct {
    char path[PE_PATH];
    char name[128];
    int depth;
    bool directory, expanded;
} TreeRow;
typedef struct {
    UiId id, selected, order[UI_MAX_TABS];
    int count, open, dragging;
    float scroll;
} WidgetState;
typedef struct {
    PeScene scene, runtime;
    PeHistory history;
    PeAssets assets;
} MapPayload;
typedef struct {
    SDL_Window *window;
    SDL_Renderer *renderer;
    SDL_Cursor *resize_cursor;
    UiContext ui;
    UiId widget_scope;
    UiTabs workspaces, left_tabs, right_tabs;
    UiScroll tree_view, inspector_view, scene_view, info_view;
    float left_width, right_width;
    bool layers_open, entity_open, palette_open;
    MapPayload *map;
    PeScriptEditor *script;
    UiId loaded_view;
    char session_paths[2][1200];
    bool session_exists[2], session_dirty[2];
    PeUiLayout *shell_layout, *active_layout;
    char ui_path[1024];
    int gap, tab_gap;
    float min_left, min_right, min_center;
    SDL_FRect panel_area;
    float panel_top;
    int global_nav_x;
    WidgetState widgets[64];
    int widget_count;
    UiRect hits[PE_UI_MAX_NODES];
    int hit_count;
    float wheel_delta;
    bool canvas_enabled, rendering, deferred_save, needs_layout;
    UiId pending_view;
    Request deferred_request;
    char deferred_path[1024];
    PeCamera camera;
    char root[768], scene_path[1024], status[256];
    int width, height, layer, tile, tool, palette_scroll, tree_scroll;
    bool playing, grid, collision, running, smoke, painting, panning, dragging;
    int start_x, start_y, last_x, last_y;
    float drag_x, drag_y;
    uint32_t selected;
    float mx, my;
    bool click;
    SDL_FRect viewport;
    Field field;
    char input[1024];
    Request request;
    char open_path[1024];
    TreeRow tree[TREE_MAX];
    int tree_count;
    char expanded[64][PE_PATH];
    int expanded_count;
    double accumulator;
    Uint64 last_time;
} Editor;
static Editor app;
static bool map_active(void) {
    return app.loaded_view == UI_MAP_VIEW;
}
static bool scripts_active(void) {
    return app.loaded_view == UI_SCRIPT_VIEW;
}
static bool edit_pending(void) {
    return app.map && app.map->history.pending != NULL;
}
static bool map_dirty(void) {
    return app.map ? pe_history_dirty(&app.map->history) : app.session_dirty[0];
}
static bool script_dirty(void) {
    return app.script ? se_dirty(app.script) : app.session_dirty[1];
}
static bool switch_editor(UiId id);
static bool import_png(const char *path);
static bool reload_layout(void);
static const cJSON *editor_entry(UiId id);
static PeUiLayout *load_editor_layout(UiId id, char *error, size_t size);
static void apply_shell_layout(void);
static void draw_yaml_node(const cJSON *node, SDL_FRect area, int column);
static void yaml_action(const char *name);
static void draw_yaml_toolbar(SDL_FRect area);
static int yaml_toolbar_rows(float width);
typedef struct {
    UiId id;
    const char *label;
    UiIcon icon;
    int action_count;
    void (*draw_tools)(SDL_FRect area);
    void (*draw_content)(void);
} EditorView;
static void draw_map_tools(SDL_FRect area);
static void draw_script_tools(SDL_FRect area);
static void draw_map_content(void);
static void draw_script_content(void);
static const EditorView editor_views[] = {
    {UI_MAP_VIEW, "Map", ICON_GRID, 9, draw_map_tools, draw_map_content},
    {UI_SCRIPT_VIEW, "Scripts", ICON_FILE, 1, draw_script_tools, draw_script_content}};
#define VIEW_COUNT ((int)(sizeof editor_views / sizeof editor_views[0]))
static const EditorView *editor_view(UiId id) {
    for (int i = 0; i < VIEW_COUNT; i++)
        if (editor_views[i].id == id)
            return &editor_views[i];
    return &editor_views[0];
}
static const char *screenshot_path;
static bool start_scripts;
static bool inside(float x, float y, SDL_FRect r) {
    return x >= r.x && y >= r.y && x < r.x + r.w && y < r.y + r.h;
}
static void draw_color(uint32_t c) {
    SDL_SetRenderDrawColor(app.renderer, (Uint8)(c >> 24), (Uint8)(c >> 16), (Uint8)(c >> 8),
                           (Uint8)c);
}
static void box(SDL_FRect r, uint32_t c) {
    draw_color(c);
    SDL_RenderFillRect(app.renderer, &r);
}
static void draw_split_grip(UiRect area, bool highlighted) {
    float x = floorf(area.x + area.w * .5f), y = floorf(area.y + area.h * .5f);
    draw_color(highlighted ? UI_ACCENT : UI_MUTED);
    for (int i = -1; i <= 1; i++) {
        float cy = y + i * 6;
        SDL_RenderPoint(app.renderer, x, cy - 1);
        SDL_RenderLine(app.renderer, x - 1, cy, x + 1, cy);
        SDL_RenderPoint(app.renderer, x, cy + 1);
    }
}
static void text(float x, float y, const char *label, uint32_t color) {
    draw_color(color);
    ui_text(app.renderer, x, y - 4, label, color);
}
static const char *tooltip;
static void register_ui_hit(UiRect rect) {
    if (app.ui.clip_depth) {
        UiRect clip = app.ui.clips[app.ui.clip_depth - 1];
        float right = fminf(rect.x + rect.w, clip.x + clip.w),
              bottom = fminf(rect.y + rect.h, clip.y + clip.h);
        rect.x = fmaxf(rect.x, clip.x);
        rect.y = fmaxf(rect.y, clip.y);
        rect.w = right - rect.x;
        rect.h = bottom - rect.y;
    }
    if (app.hit_count < PE_UI_MAX_NODES && rect.w > 0 && rect.h > 0)
        app.hits[app.hit_count++] = rect;
}
static bool icon_button_impl(UiId callsite, float x, float y, float w, UiIcon icon,
                             const char *label, const char *hint, bool active, bool enabled) {
    SDL_FRect r = {x, y, w, 25};
    UiRect hit = {x, y, w, 25};
    register_ui_hit(hit);
    bool hover = ui_hovered(&app.ui, hit);
    ui_draw_button(app.renderer, r, icon, label, active, enabled, hover);
    if (hover && hint)
        tooltip = hint;
    UiId id = ui_id("editor_button") ^ (callsite * 16777619u) ^ app.widget_scope;
    return ui_button(&app.ui, id ? id : 1, hit, enabled);
}
#define icon_button(...) icon_button_impl((UiId)__LINE__, __VA_ARGS__)
static bool button_impl(UiId callsite, float x, float y, float w, const char *label, bool active,
                        bool enabled) {
    return icon_button_impl(callsite, x, y, w, ICON_NONE, label, NULL, active, enabled);
}
#define button(...) button_impl((UiId)__LINE__, __VA_ARGS__)
static void section(float x, float y, UiIcon icon, const char *title) {
    ui_icon(app.renderer, icon, x, y - 4, 16, UI_ACCENT);
    text(x + 24, y, title, UI_MUTED);
}
static void message(const char *s) {
    char copy[256] = {0};
    snprintf(copy, sizeof copy, "%s", s);
    memcpy(app.status, copy, sizeof copy);
    if (app.script)
        memcpy(app.script->status, copy, sizeof copy);
}
static bool begin(void) {
    if (!pe_history_begin(&app.map->history, &app.map->scene)) {
        message("Cannot begin edit (pending edit or out of memory)");
        return false;
    }
    return true;
}
static void commit(void) {
    if (!pe_history_commit(&app.map->history, &app.map->scene))
        message("Out of memory: edit reverted");
}
static void assets_sync(void) {
    char error[256];
    if (!pe_assets_load(&app.map->assets, app.renderer, app.root, &app.map->scene, error,
                        sizeof error))
        message(error);
}
static void selection_sync(void) {
    if (app.layer >= app.map->scene.layer_count)
        app.layer = app.map->scene.layer_count - 1;
    if (!pe_entity_valid(&app.map->scene, pe_entity_find(&app.map->scene, app.selected)))
        app.selected = 0;
    assets_sync();
}
static bool save_scene(void) {
    char error[256];
    if (!pe_scene_save(&app.map->scene, app.scene_path, error, sizeof error)) {
        message(error);
        return false;
    }
    pe_history_saved(&app.map->history);
    message("Scene saved");
    return true;
}
static bool save(void) {
    if (scripts_active()) {
        bool ok = se_save(&(*app.script));
        message((*app.script).status);
        return ok;
    }
    return save_scene();
}
static void select_workspace(UiId id) {
    if (app.rendering && id != app.loaded_view) {
        app.pending_view = id;
        app.workspaces.selected = app.loaded_view;
        return;
    }
    if (edit_pending() || (app.playing && id != UI_MAP_VIEW)) {
        app.workspaces.selected = app.loaded_view;
        return;
    }
    if (!switch_editor(editor_view(id)->id)) {
        app.workspaces.selected = app.loaded_view;
        ui_focus(&app.ui, app.script ? UI_SCRIPT_DOCUMENT : 0);
        if (app.script)
            se_focus(app.script, app.window, true);
        return;
    }
    app.workspaces.selected = app.loaded_view;
    ui_focus(&app.ui, scripts_active() ? UI_SCRIPT_DOCUMENT : 0);
    if (app.script)
        se_focus(app.script, app.window, !app.playing);
}
static void tab_select(bool scripts) {
    select_workspace(scripts ? UI_SCRIPT_VIEW : UI_MAP_VIEW);
}
static bool request_dirty(Request r) {
    if (r == REQUEST_SCRIPT_OPEN)
        return script_dirty();
    if (r == REQUEST_QUIT)
        return map_dirty() || script_dirty();
    return map_dirty();
}
static bool request_save(void) {
    if (app.request == REQUEST_QUIT) {
        if (map_dirty()) {
            select_workspace(UI_MAP_VIEW);
            if (!map_active() || !save_scene())
                return false;
        }
        if (script_dirty()) {
            select_workspace(UI_SCRIPT_VIEW);
            if (!scripts_active() || !save())
                return false;
        }
        return true;
    }
    return save();
}
static void undo_active(bool redo) {
    if (scripts_active()) {
        bool ok =
            redo ? pe_text_redo(&(*app.script).document) : pe_text_undo(&(*app.script).document);
        if (ok)
            se_changed(&(*app.script), app.viewport, false);
    } else {
        if (redo)
            pe_redo(&app.map->history, &app.map->scene);
        else
            pe_undo(&app.map->history, &app.map->scene);
        selection_sync();
    }
}
static bool is_expanded(const char *p) {
    for (int i = 0; i < app.expanded_count; i++)
        if (!strcmp(p, app.expanded[i]))
            return true;
    return false;
}
typedef struct {
    const char *relative;
    int depth;
} ScanContext;
static void scan(const char *relative, int depth);
static SDL_EnumerationResult SDLCALL scan_entry(void *userdata, const char *dirname,
                                                const char *name) {
    (void)dirname;
    ScanContext *context = userdata;
    if (name[0] == '.' || app.tree_count == TREE_MAX)
        return SDL_ENUM_CONTINUE;
    char relative[PE_PATH], full[1024];
    int length = snprintf(relative, sizeof relative, "%s%s%s", context->relative,
                          *context->relative ? "/" : "", name);
    if (length >= (int)sizeof relative)
        return SDL_ENUM_CONTINUE;
    snprintf(full, sizeof full, "%s/%s", app.root, relative);
    SDL_PathInfo info;
    if (!SDL_GetPathInfo(full, &info))
        return SDL_ENUM_CONTINUE;
    TreeRow *row = &app.tree[app.tree_count++];
    snprintf(row->path, sizeof row->path, "%s", relative);
    snprintf(row->name, sizeof row->name, "%.127s", name);
    row->depth = context->depth;
    row->directory = info.type == SDL_PATHTYPE_DIRECTORY;
    row->expanded = is_expanded(relative);
    if (row->directory && row->expanded && context->depth < 8)
        scan(relative, context->depth + 1);
    return SDL_ENUM_CONTINUE;
}
static void scan(const char *relative, int depth) {
    char path[1024];
    snprintf(path, sizeof path, "%s/%s", app.root, relative);
    ScanContext context = {relative, depth};
    SDL_EnumerateDirectory(path, scan_entry, &context);
}
static void tree_refresh(void) {
    app.tree_count = 0;
    scan("", 0);
    if (app.tree_scroll >= app.tree_count)
        app.tree_scroll = 0;
}
static void tree_toggle(const char *path) {
    for (int i = 0; i < app.expanded_count; i++)
        if (!strcmp(app.expanded[i], path)) {
            memmove(app.expanded[i], app.expanded[i + 1],
                    (size_t)(app.expanded_count - i - 1) * PE_PATH);
            app.expanded_count--;
            tree_refresh();
            return;
        }
    if (app.expanded_count < 64) {
        snprintf(app.expanded[app.expanded_count++], PE_PATH, "%s", path);
        tree_refresh();
    }
}
static bool suffix(const char *p, const char *s) {
    size_t n = strlen(p), m = strlen(s);
    return n >= m && !SDL_strcasecmp(p + n - m, s);
}
static void load_scene(const char *path) {
    PeScene *next = malloc(sizeof *next);
    char error[256];
    if (!next) {
        message("Out of memory");
        return;
    }
    if (!pe_scene_load(next, path, error, sizeof error)) {
        message(error);
        free(next);
        return;
    }
    app.map->scene = *next;
    free(next);
    snprintf(app.scene_path, sizeof app.scene_path, "%s", path);
    pe_history_free(&app.map->history);
    app.layer = 0;
    app.selected = 0;
    app.camera = (PeCamera){-16, -16, 2};
    assets_sync();
    tab_select(false);
    message("Scene opened");
}
static void request(Request type, const char *path) {
    if (app.rendering) {
        app.deferred_request = type;
        snprintf(app.deferred_path, sizeof app.deferred_path, "%s", path ? path : "");
        return;
    }
    if (type == REQUEST_IMPORT) {
        import_png(path);
        return;
    }
    if (type == REQUEST_SCRIPT_OPEN) {
        select_workspace(UI_SCRIPT_VIEW);
        if (!scripts_active())
            return;
    } else if (type == REQUEST_OPEN) {
        select_workspace(UI_MAP_VIEW);
        if (!map_active())
            return;
    }
    if (app.playing)
        return;
    if (path) {
        if (strlen(path) >= sizeof app.open_path) {
            message("Path too long");
            return;
        }
        snprintf(app.open_path, sizeof app.open_path, "%s", path);
    }
    if (request_dirty(type)) {
        app.request = type;
        return;
    }
    if (type == REQUEST_QUIT)
        app.running = false;
    else if (type == REQUEST_SCRIPT_OPEN) {
        if (se_open(&(*app.script), app.open_path))
            tab_select(true);
        else
            message((*app.script).status);
    } else
        load_scene(app.open_path);
}
static void request_finish(void) {
    Request r = app.request;
    app.request = REQUEST_NONE;
    if (r == REQUEST_QUIT)
        app.running = false;
    else if (r == REQUEST_OPEN)
        load_scene(app.open_path);
    else if (r == REQUEST_SCRIPT_OPEN) {
        if (se_open(&(*app.script), app.open_path))
            tab_select(true);
        else
            message((*app.script).status);
    }
}
static bool set_tileset(const char *relative, int tile_size) {
    PeScene candidate = app.map->scene;
    snprintf(candidate.tileset, sizeof candidate.tileset, "%s", relative);
    candidate.tileset_tile_size = tile_size;
    PeAssets check = {0};
    char error[256];
    if (!pe_assets_load(&check, app.renderer, app.root, &candidate, error, sizeof error)) {
        message(error);
        return false;
    }
    if (!begin()) {
        pe_assets_free(&check);
        return false;
    }
    app.map->scene = candidate;
    commit();
    pe_assets_free(&app.map->assets);
    app.map->assets = check;
    app.tile = 1;
    app.palette_scroll = 0;
    message("Tileset selected (Ctrl+Z to undo)");
    return true;
}
static bool import_png(const char *source) {
    if (app.playing || !suffix(source, ".png")) {
        message("Import requires a PNG file; stop play mode first");
        return false;
    }
    SDL_PathInfo info;
    if (!SDL_GetPathInfo(source, &info) || info.type != SDL_PATHTYPE_FILE ||
        info.size > 64 * 1024 * 1024) {
        message("Cannot import file (max 64 MiB)");
        return false;
    }
    char folder[1024];
    snprintf(folder, sizeof folder, "%s/assets", app.root);
    if (!SDL_CreateDirectory(folder)) {
        message(SDL_GetError());
        return false;
    }
    char relative[PE_PATH], destination[1024];
    int counter = 0;
    do {
        snprintf(relative, sizeof relative, "assets/import_%llu_%d.png",
                 (unsigned long long)SDL_GetTicksNS(), counter++);
        snprintf(destination, sizeof destination, "%s/%s", app.root, relative);
    } while (SDL_GetPathInfo(destination, &info) && info.type != SDL_PATHTYPE_NONE);
    if (!SDL_CopyFile(source, destination)) {
        message(SDL_GetError());
        return false;
    }
    if (!set_tileset(relative, app.map->scene.tileset_tile_size)) {
        SDL_RemovePath(destination);
        return false;
    }
    tree_refresh();
    message("PNG imported; undo restores previous tileset, file stays available");
    return true;
}
static void field_start(Field f, const char *value) {
    app.field = f;
    ui_focus(&app.ui, ui_id("modal_text"));
    snprintf(app.input, sizeof app.input, "%s", value);
    SDL_StartTextInput(app.window);
}
static void field_stop(void) {
    app.field = FIELD_NONE;
    ui_focus(&app.ui, scripts_active() && (*app.script).focused ? UI_SCRIPT_DOCUMENT : 0);
    if (scripts_active() && (*app.script).focused)
        SDL_StartTextInput(app.window);
    else
        SDL_StopTextInput(app.window);
}
static void field_apply(void) {
    if (app.field == FIELD_IMPORT) {
        if (import_png(app.input))
            field_stop();
        return;
    }
    if (app.field == FIELD_LAYER) {
        if (!*app.input || strlen(app.input) >= 32) {
            message("Layer name must contain 1-31 bytes");
            return;
        }
        if (begin()) {
            memcpy(app.map->scene.layers[app.layer].name, app.input, strlen(app.input) + 1);
            commit();
            field_stop();
        }
        return;
    }
    char *end;
    errno = 0;
    float value = strtof(app.input, &end);
    if (errno || end == app.input || *end || !isfinite(value)) {
        message("Enter a valid finite number");
        return;
    }
    if (app.field == FIELD_TILESIZE) {
        if (value < 1 || value > 128 || floorf(value) != value) {
            message("Source tile size must be an integer from 1 to 128");
            return;
        }
        if (set_tileset(app.map->scene.tileset, (int)value))
            field_stop();
        return;
    }
    PeObject *o = pe_entity_get(&app.map->scene, pe_entity_find(&app.map->scene, app.selected));
    if (!o) {
        field_stop();
        return;
    }
    if (fabsf(value) > 8192 || ((app.field == FIELD_W || app.field == FIELD_H) && value <= 0)) {
        message("Position: -8192..8192; size: >0..8192");
        return;
    }
    if (begin()) {
        switch (app.field) {
        case FIELD_X:
            o->x = value;
            break;
        case FIELD_Y:
            o->y = value;
            break;
        case FIELD_W:
            o->w = value;
            break;
        case FIELD_H:
            o->h = value;
            break;
        default:
            break;
        }
        commit();
        field_stop();
    }
}
static void tile_coords(float sx, float sy, int *x, int *y) {
    float wx, wy;
    pe_screen_to_world(&app.camera, sx - app.viewport.x, sy - app.viewport.y, &wx, &wy);
    *x = (int)floorf(wx / app.map->scene.tile_size);
    *y = (int)floorf(wy / app.map->scene.tile_size);
}
static void paint_cell(int x, int y, bool erase) {
    if (x < 0 || y < 0 || x >= app.map->scene.width || y >= app.map->scene.height)
        return;
    if (app.collision)
        app.map->scene.solid[y * PE_MAP_MAX + x] = erase ? 0 : 1;
    else
        pe_tile_set(&app.map->scene, app.layer, x, y, erase ? 0 : (uint16_t)app.tile);
}
static void paint_line(int x0, int y0, int x1, int y1, bool erase) {
    int dx = abs(x1 - x0), sx = x0 < x1 ? 1 : -1, dy = -abs(y1 - y0), sy = y0 < y1 ? 1 : -1,
        e = dx + dy;
    for (;;) {
        paint_cell(x0, y0, erase);
        if (x0 == x1 && y0 == y1)
            break;
        int twice = 2 * e;
        if (twice >= dy) {
            e += dy;
            x0 += sx;
        }
        if (twice <= dx) {
            e += dx;
            y0 += sy;
        }
    }
}
static void flood_collision(int x, int y, bool erase) {
    if (x < 0 || y < 0 || x >= app.map->scene.width || y >= app.map->scene.height)
        return;
    uint8_t value = erase ? 0 : 1, old = app.map->scene.solid[y * PE_MAP_MAX + x];
    if (value == old)
        return;
    int queue[PE_MAP_MAX * PE_MAP_MAX], head = 0, tail = 0;
    queue[tail++] = y * PE_MAP_MAX + x;
    app.map->scene.solid[y * PE_MAP_MAX + x] = value;
    while (head < tail) {
        int p = queue[head++], px = p % PE_MAP_MAX, py = p / PE_MAP_MAX;
        int nx[] = {px - 1, px + 1, px, px}, ny[] = {py, py, py - 1, py + 1};
        for (int i = 0; i < 4; i++)
            if (nx[i] >= 0 && ny[i] >= 0 && nx[i] < app.map->scene.width &&
                ny[i] < app.map->scene.height &&
                app.map->scene.solid[ny[i] * PE_MAP_MAX + nx[i]] == old) {
                app.map->scene.solid[ny[i] * PE_MAP_MAX + nx[i]] = value;
                queue[tail++] = ny[i] * PE_MAP_MAX + nx[i];
            }
    }
}
static void edit_start(float sx, float sy, bool erase) {
    int x, y;
    tile_coords(sx, sy, &x, &y);
    app.start_x = app.last_x = x;
    app.start_y = app.last_y = y;
    if (app.tool == SELECT) {
        float wx, wy;
        pe_screen_to_world(&app.camera, sx - app.viewport.x, sy - app.viewport.y, &wx, &wy);
        app.selected = 0;
        for (int i = PE_ENTITIES - 1; i >= 0; i--) {
            PeObject *o = &app.map->scene.objects[i];
            if (o->alive && (o->mask & PE_TRANSFORM) && wx >= o->x && wy >= o->y &&
                wx < o->x + o->w && wy < o->y + o->h) {
                app.selected = o->stable_id;
                if (begin()) {
                    app.dragging = true;
                    app.drag_x = wx - o->x;
                    app.drag_y = wy - o->y;
                }
                break;
            }
        }
        return;
    }
    if (!begin())
        return;
    if (app.tool == FILL) {
        if (app.collision)
            flood_collision(x, y, erase);
        else
            pe_fill(&app.map->scene, app.layer, x, y, erase ? 0 : (uint16_t)app.tile);
        commit();
        return;
    }
    app.painting = true;
    if (app.tool != RECTANGLE)
        paint_cell(x, y, erase || app.tool == ERASE);
}
static void edit_end(bool erase) {
    if (app.painting && app.tool == RECTANGLE) {
        int x, y;
        tile_coords(app.mx, app.my, &x, &y);
        if (app.collision) {
            int x0 = app.start_x, x1 = x, y0 = app.start_y, y1 = y;
            if (x0 > x1) {
                int t = x0;
                x0 = x1;
                x1 = t;
            }
            if (y0 > y1) {
                int t = y0;
                y0 = y1;
                y1 = t;
            }
            if (x0 < 0)
                x0 = 0;
            if (y0 < 0)
                y0 = 0;
            if (x1 >= app.map->scene.width)
                x1 = app.map->scene.width - 1;
            if (y1 >= app.map->scene.height)
                y1 = app.map->scene.height - 1;
            for (int py = y0; py <= y1; py++)
                for (int px = x0; px <= x1; px++)
                    paint_cell(px, py, erase);
        } else
            pe_rectangle(&app.map->scene, app.layer, app.start_x, app.start_y, x, y,
                         erase ? 0 : (uint16_t)app.tile);
    }
    if (app.painting || app.dragging)
        commit();
    app.painting = false;
    app.dragging = false;
}
typedef struct {
    float left, right;
    PeCamera camera;
    float scroll[4];
    int layer, tile, tool, palette;
    uint32_t selected, flags;
    int counts[2];
    UiId tabs[2][UI_MAX_TABS], selected_tabs[2];
    char scene_path[1024], status[256];
    int widget_count;
    WidgetState widgets[64];
    int expanded_count;
    char expanded[64][PE_PATH];
} UiSnapshot;
static void snapshot_ui(UiSnapshot *s, bool load) {
    if (!load) {
        memset(s, 0, sizeof *s);
        s->expanded_count = app.expanded_count;
        memcpy(s->expanded, app.expanded, sizeof s->expanded);
        s->widget_count = app.widget_count;
        memcpy(s->widgets, app.widgets, sizeof s->widgets);
        s->left = app.left_width;
        s->right = app.right_width;
        s->camera = app.camera;
        s->scroll[0] = app.tree_view.offset;
        s->scroll[1] = app.scene_view.offset;
        s->scroll[2] = app.inspector_view.offset;
        s->scroll[3] = app.info_view.offset;
        s->layer = app.layer;
        s->tile = app.tile;
        s->tool = app.tool;
        s->palette = app.palette_scroll;
        s->selected = app.selected;
        s->flags = app.grid | (app.collision << 1) | (app.layers_open << 2) |
                   (app.entity_open << 3) | (app.palette_open << 4);
        UiTabs *tabs[] = {&app.left_tabs, &app.right_tabs};
        for (int k = 0; k < 2; k++) {
            s->counts[k] = tabs[k]->count;
            s->selected_tabs[k] = tabs[k]->selected;
            for (int i = 0; i < tabs[k]->count; i++)
                s->tabs[k][i] = tabs[k]->items[i].id;
        }
        snprintf(s->scene_path, sizeof s->scene_path, "%s", app.scene_path);
        snprintf(s->status, sizeof s->status, "%s", app.status);
    } else {
        app.expanded_count = s->expanded_count;
        memcpy(app.expanded, s->expanded, sizeof app.expanded);
        app.widget_count = s->widget_count;
        memcpy(app.widgets, s->widgets, sizeof app.widgets);
        app.left_width = s->left;
        app.right_width = s->right;
        app.camera = s->camera;
        app.tree_view = (UiScroll){s->scroll[0], 0, 0};
        app.scene_view = (UiScroll){s->scroll[1], 0, 0};
        app.inspector_view = (UiScroll){s->scroll[2], 0, 0};
        app.info_view = (UiScroll){s->scroll[3], 0, 0};
        app.layer = s->layer;
        app.tile = s->tile;
        app.tool = s->tool;
        app.palette_scroll = s->palette;
        app.selected = s->selected;
        app.grid = (s->flags & 1) != 0;
        app.collision = (s->flags & 2) != 0;
        app.layers_open = (s->flags & 4) != 0;
        app.entity_open = (s->flags & 8) != 0;
        app.palette_open = (s->flags & 16) != 0;
        UiTabs *tabs[] = {&app.left_tabs, &app.right_tabs};
        for (int k = 0; k < 2; k++) {
            UiTab items[UI_MAX_TABS];
            for (int i = 0; i < s->counts[k]; i++)
                items[i] = (UiTab){s->tabs[k][i], true, 0};
            ui_tabs_init(tabs[k], items, s->counts[k], s->selected_tabs[k]);
        }
        snprintf(app.scene_path, sizeof app.scene_path, "%s", s->scene_path);
        snprintf(app.status, sizeof app.status, "%s", s->status);
    }
}
static bool valid_snapshot(const UiSnapshot *s) {
    if (s->expanded_count < 0 || s->expanded_count > 64)
        return false;
    for (int i = 0; i < s->expanded_count; i++)
        if (!memchr(s->expanded[i], 0, PE_PATH))
            return false;
    if (s->widget_count < 0 || s->widget_count > 64)
        return false;
    for (int i = 0; i < s->widget_count; i++)
        if (s->widgets[i].count < 0 || s->widgets[i].count > UI_MAX_TABS ||
            !isfinite(s->widgets[i].scroll) || s->widgets[i].scroll < 0)
            return false;
    if (!isfinite(s->left) || !isfinite(s->right) || s->left < 64 || s->right < 64 ||
        !isfinite(s->camera.x) || !isfinite(s->camera.y) || !isfinite(s->camera.zoom) ||
        s->camera.zoom <= 0 || s->layer < 0 || s->layer >= PE_LAYERS || s->tool < BRUSH ||
        s->tool > SELECT || !memchr(s->scene_path, 0, sizeof s->scene_path) ||
        !memchr(s->status, 0, sizeof s->status))
        return false;
    for (int k = 0; k < 4; k++)
        if (!isfinite(s->scroll[k]) || s->scroll[k] < 0)
            return false;
    for (int k = 0; k < 2; k++) {
        UiTabs t;
        UiTab items[UI_MAX_TABS];
        if (s->counts[k] < 1 || s->counts[k] > UI_MAX_TABS)
            return false;
        for (int i = 0; i < s->counts[k]; i++)
            items[i] = (UiTab){s->tabs[k][i], true, 0};
        if (!ui_tabs_init(&t, items, s->counts[k], s->selected_tabs[k]))
            return false;
    }
    return true;
}
static bool session_scene(PeSession *io, PeScene *scene) {
    if (!pe_session_block(io, scene, sizeof *scene))
        return false;
    if (!io->reading)
        return true;
    if (scene->width < 1 || scene->width > PE_MAP_MAX || scene->height < 1 ||
        scene->height > PE_MAP_MAX || scene->layer_count < 1 || scene->layer_count > PE_LAYERS ||
        scene->tile_size < 1 || !memchr(scene->tileset, 0, sizeof scene->tileset))
        return false;
    for (int i = 0; i < PE_LAYERS; i++) {
        unsigned char b;
        memcpy(&b, &scene->layers[i].visible, 1);
        if (b > 1 || !memchr(scene->layers[i].name, 0, sizeof scene->layers[i].name))
            return false;
    }
    for (int i = 0; i < PE_ENTITIES; i++) {
        unsigned char b;
        memcpy(&b, &scene->objects[i].alive, 1);
        if (b > 1)
            return false;
        const PeObject *o = &scene->objects[i];
        if (!isfinite(o->x) || !isfinite(o->y) || !isfinite(o->w) || !isfinite(o->h) ||
            !isfinite(o->vx) || !isfinite(o->vy))
            return false;
    }
    return true;
}
typedef struct {
    uint64_t revision, next, saved;
    int count, cursor;
} HistoryHeader;
typedef struct {
    uint64_t revision, next, saved, cursor, anchor;
    int count, position;
} TextHeader;
static bool editor_codec(PeSession *io, void *opaque) {
    UiId id = *(UiId *)opaque;
    uint32_t abi[3] = {sizeof(PeScene), sizeof(UiSnapshot), sizeof(TextHeader)};
    uint32_t expected[3];
    memcpy(expected, abi, sizeof abi);
    if (!pe_session_block(io, abi, sizeof abi) || memcmp(abi, expected, sizeof abi))
        return false;
    UiSnapshot ui;
    if (!io->reading)
        snapshot_ui(&ui, false);
    if (!pe_session_block(io, &ui, sizeof ui) || !valid_snapshot(&ui))
        return false;
    if (id == UI_MAP_VIEW) {
        PeHistory *h = &app.map->history;
        HistoryHeader header = {h->revision, h->next_revision, h->saved_revision, h->count,
                                h->cursor};
        if (!session_scene(io, &app.map->scene) || !pe_session_block(io, &header, sizeof header) ||
            header.count < 0 || header.count > PE_HISTORY || header.cursor < 0 ||
            header.cursor > header.count || header.next <= header.revision)
            return false;
        if (io->reading) {
            h->count = header.count;
            h->cursor = header.cursor;
            h->revision = header.revision;
            h->next_revision = header.next;
            h->saved_revision = header.saved;
        }
        for (int i = 0; i < h->count; i++) {
            PeCommand *c = &h->commands[i];
            if (io->reading) {
                c->before = calloc(1, sizeof(PeScene));
                c->after = calloc(1, sizeof(PeScene));
            }
            if (!c->before || !c->after ||
                !pe_session_block(io, &c->before_revision, sizeof(uint64_t)) ||
                !pe_session_block(io, &c->after_revision, sizeof(uint64_t)) ||
                !session_scene(io, c->before) || !session_scene(io, c->after))
                return false;
        }
    } else {
        PeScriptEditor *e = app.script;
        char path[1024] = {0};
        int state[3] = {e->exists, e->top_line, e->left_column};
        if (!io->reading)
            snprintf(path, sizeof path, "%s", e->path);
        if (!pe_session_block(io, path, sizeof path) || !memchr(path, 0, sizeof path) ||
            !pe_session_block(io, state, sizeof state) || state[0] < 0 || state[0] > 1 ||
            state[1] < 0 || state[2] < 0)
            return false;
        PeText *d = &e->document;
        TextHeader header = {d->revision, d->next_revision, d->saved_revision, d->cursor,
                             d->anchor,   d->count,         d->position};
        char *text = d->text;
        if (!pe_session_block(io, &header, sizeof header) || header.count < 0 ||
            header.count > PE_TEXT_HISTORY || header.position < 0 ||
            header.position > header.count || header.next <= header.revision)
            return false;
        if (io->reading)
            text = NULL;
        bool ok = pe_session_string(io, &text, PE_TEXT_MAX);
        if (io->reading) {
            ok = ok && header.cursor <= strlen(text) && header.anchor <= strlen(text) &&
                 pe_text_init(d, text);
            free(text);
            if (!ok)
                return false;
            d->revision = header.revision;
            d->next_revision = header.next;
            d->saved_revision = header.saved;
            d->cursor = (size_t)header.cursor;
            d->anchor = (size_t)header.anchor;
            d->count = header.count;
            d->position = header.position;
        } else if (!ok)
            return false;
        for (int i = 0; i < d->count; i++) {
            PeTextEdit *edit = &d->edits[i];
            uint64_t values[5] = {edit->before_cursor, edit->before_anchor, edit->after_cursor,
                                  edit->before_revision, edit->after_revision};
            if (!pe_session_block(io, values, sizeof values) ||
                !pe_session_string(io, &edit->before, PE_TEXT_MAX) ||
                !pe_session_string(io, &edit->after, PE_TEXT_MAX) ||
                values[0] > strlen(edit->before) || values[1] > strlen(edit->before) ||
                values[2] > strlen(edit->after))
                return false;
            if (io->reading) {
                edit->before_cursor = (size_t)values[0];
                edit->before_anchor = (size_t)values[1];
                edit->after_cursor = (size_t)values[2];
                edit->before_revision = values[3];
                edit->after_revision = values[4];
            }
        }
        if (io->reading) {
            snprintf(e->path, sizeof e->path, "%s", path);
            snprintf(e->root, sizeof e->root, "%s", app.root);
            e->exists = state[0] != 0;
            e->top_line = state[1];
            e->left_column = state[2];
            e->window = app.window;
            e->highlighted_revision = e->extent_revision = e->checked_revision = UINT64_MAX;
        }
    }
    if (io->reading)
        snapshot_ui(&ui, true);
    return true;
}
static void release_editor(void) {
    if (app.script) {
        se_focus(app.script, app.window, false);
        se_free(app.script);
        free(app.script);
        app.script = NULL;
    }
    if (app.map) {
        pe_assets_free(&app.map->assets);
        pe_history_free(&app.map->history);
        free(app.map);
        app.map = NULL;
    }
    ui_cancel(&app.ui);
    ui_focus(&app.ui, 0);
    pe_ui_layout_free(app.active_layout);
    app.active_layout = NULL;
    app.widget_count = 0;
    memset(app.widgets, 0, sizeof app.widgets);
    app.loaded_view = 0;
    app.painting = app.dragging = app.panning = false;
    memset(app.tree, 0, sizeof app.tree);
    app.tree_count = 0;
}
static void default_editor_ui(bool scripts) {
    const cJSON *columns = pe_ui_get(pe_ui_get(app.shell_layout->root, "layout"), "columns");
    app.left_width = (float)pe_ui_get(pe_ui_get(columns, "left"), "initial")->valuedouble;
    app.right_width = (float)pe_ui_get(pe_ui_get(columns, "right"), "initial")->valuedouble;
    app.camera = (PeCamera){-16, -16, 1.4f};
    app.layer = 0;
    app.tile = 1;
    app.tool = BRUSH;
    app.selected = 0;
    app.expanded_count = 3;
    snprintf(app.expanded[0], PE_PATH, "assets");
    snprintf(app.expanded[1], PE_PATH, "scenes");
    snprintf(app.expanded[2], PE_PATH, "scripts");
    app.grid = true;
    app.collision = false;
    app.layers_open = app.entity_open = app.palette_open = true;
    app.tree_view = app.scene_view = app.inspector_view = app.info_view = (UiScroll){0};
    UiTab left[] = {{UI_EXPLORER_TAB, true, 0}, {UI_SCENE_TAB, true, 0}};
    UiTab right[] = {{UI_INSPECTOR_TAB, true, 0}, {UI_INFO_TAB, true, 0}};
    ui_tabs_init(&app.left_tabs, left, scripts ? 1 : 2, UI_EXPLORER_TAB);
    ui_tabs_init(&app.right_tabs, right, 2, UI_INSPECTOR_TAB);
}
static bool activate_editor(UiId id, char *error, size_t size) {
    if (!app.active_layout)
        app.active_layout = load_editor_layout(id, error, size);
    if (!app.active_layout)
        return false;
    int index = id == UI_MAP_VIEW ? 0 : 1;
    if (index == 0)
        app.map = calloc(1, sizeof *app.map);
    else
        app.script = calloc(1, sizeof *app.script);
    if ((index == 0 && !app.map) || (index == 1 && !app.script)) {
        snprintf(error, size, "Out of memory loading editor");
        return false;
    }
    bool ok;
    if (app.session_exists[index])
        ok = pe_session_load(app.session_paths[index], id, editor_codec, &id, error, size);
    else {
        default_editor_ui(index == 1);
        if (index == 0) {
            pe_history_init(&app.map->history);
            ok = pe_scene_load(&app.map->scene, app.scene_path, error, size);
        } else {
            ok = se_init(app.script, app.root);
            if (!ok)
                snprintf(error, size, "%s", app.script->status);
        }
    }
    if (!ok) {
        release_editor();
        return false;
    }
    app.loaded_view = id;
    app.needs_layout = true;
    tree_refresh();
    if (index == 0)
        assets_sync();
    else
        se_check(app.script);
    return true;
}
static bool switch_editor(UiId id) {
    if (id == app.loaded_view)
        return true;
    char error[256];
    PeUiLayout *candidate = load_editor_layout(id, error, sizeof error);
    if (!candidate) {
        message(error);
        return false;
    }
    UiId previous = app.loaded_view;
    if (previous) {
        int index = previous == UI_MAP_VIEW ? 0 : 1;
        if (!pe_session_save(app.session_paths[index], previous, editor_codec, &previous, error,
                             sizeof error)) {
            pe_ui_layout_free(candidate);
            message(error);
            return false;
        }
        app.session_dirty[index] = index == 0 ? map_dirty() : script_dirty();
        app.session_exists[index] = true;
        release_editor();
    }
    app.active_layout = candidate;
    if (activate_editor(id, error, sizeof error))
        return true;
    char original[256];
    snprintf(original, sizeof original, "%s", error);
    if (previous)
        activate_editor(previous, error, sizeof error);
    message(original);
    return false;
}

static void play_toggle(void) {
    if (edit_pending())
        return;
    app.playing = !app.playing;
    if (app.playing) {
        app.map->runtime = app.map->scene;
        app.accumulator = 0;
        message("Play: WASD / arrows. Stop restores edited scene.");
    } else
        message("Stopped; edited scene preserved");
}
static void draw_scene_tree(void) {
    section(app.panel_area.x + 11, app.panel_top + 18, ICON_ENTITY, "SCENE ENTITIES");
    int count = 0;
    for (int i = 0; i < PE_ENTITIES; i++)
        if (app.map->scene.objects[i].alive)
            count++;
    app.scene_view.content = count ? count * 26.f : 26;
    app.scene_view.viewport = (float)(app.panel_area.h - 46);
    ui_scroll_clamp(&app.scene_view);
    UiRect track = {app.panel_area.x + app.panel_area.w - 15, app.panel_top + 46, 8,
                    app.scene_view.viewport};
    ui_scrollbar(&app.ui, UI_SCENE_SCROLL, track, &app.scene_view, UI_AXIS_Y);
    ui_clip_push(&app.ui, (UiRect){app.panel_area.x, app.panel_top + 46, app.panel_area.w - 18,
                                   app.scene_view.viewport});
    ui_apply_clip(app.renderer, &app.ui);
    float y = app.panel_top + 46 - app.scene_view.offset;
    if (!count)
        text(app.panel_area.x + 11, y + 8, "No entities in scene.", UI_MUTED);
    for (int i = 0; i < PE_ENTITIES; i++) {
        const PeObject *o = &app.map->scene.objects[i];
        if (!o->alive)
            continue;
        char label[40];
        snprintf(label, sizeof label, "%s %u", o->mask & PE_PLAYER ? "Player" : "Entity",
                 o->stable_id);
        app.widget_scope = ui_id("scene_entity") ^ o->stable_id;
        if (icon_button(app.panel_area.x + 9, y, app.panel_area.w - 32, ICON_ENTITY, label,
                        "Select entity", app.selected == o->stable_id,
                        (app.map || app.script) && !app.playing && !edit_pending())) {
            app.selected = o->stable_id;
            app.tool = SELECT;
            app.right_tabs.selected = UI_INSPECTOR_TAB;
            tab_select(false);
        }
        y += 26;
    }
    app.widget_scope = 0;
    ui_clip_pop(&app.ui);
    ui_apply_clip(app.renderer, &app.ui);
    ui_draw_scrollbar(app.renderer, &app.ui, UI_SCENE_SCROLL, track, &app.scene_view, UI_AXIS_Y);
}
static void draw_info(void) {
    float x = (float)(app.panel_area.x + 9);
    app.info_view.content = 340;
    app.info_view.viewport = (float)(app.panel_area.h - 16);
    ui_scroll_clamp(&app.info_view);
    UiRect track = {(float)(app.panel_area.x + app.panel_area.w - 15), app.panel_top + 8, 8,
                    app.info_view.viewport};
    ui_scrollbar(&app.ui, UI_INFO_SCROLL, track, &app.info_view, UI_AXIS_Y);
    ui_clip_push(&app.ui,
                 (UiRect){x - 4, app.panel_top + 8, app.panel_area.w - 18, app.info_view.viewport});
    ui_apply_clip(app.renderer, &app.ui);
    float y = app.panel_top + 18 - app.info_view.offset;
    section(x, y, ICON_FILE, "SCENE INFO");
    char label[64];
    snprintf(label, sizeof label, "Map: %d x %d tiles", app.map->scene.width,
             app.map->scene.height);
    text(x, y + 32, label, UI_TEXT);
    snprintf(label, sizeof label, "Tile size: %d px", app.map->scene.tile_size);
    text(x, y + 56, label, UI_TEXT);
    snprintf(label, sizeof label, "Layers: %d", app.map->scene.layer_count);
    text(x, y + 80, label, UI_TEXT);
    int count = 0;
    for (int i = 0; i < PE_ENTITIES; i++)
        if (app.map->scene.objects[i].alive)
            count++;
    snprintf(label, sizeof label, "Entities: %d / %d", count, PE_ENTITIES);
    text(x, y + 104, label, UI_TEXT);
    section(x, y + 148, ICON_EDIT, "SHORTCUTS");
    const char *help[] = {"Ctrl+S  Save",    "Ctrl+Z  Undo",     "F5      Play / Stop",
                          "MMB     Pan map", "Wheel   Zoom map", "B/E/R/F/V  Map tools"};
    for (int i = 0; i < 6; i++)
        text(x, y + 180 + i * 24, help[i], UI_MUTED);
    ui_clip_pop(&app.ui);
    ui_apply_clip(app.renderer, &app.ui);
    ui_draw_scrollbar(app.renderer, &app.ui, UI_INFO_SCROLL, track, &app.info_view, UI_AXIS_Y);
}
static void draw_tree(void) {
    section(app.panel_area.x + 11, app.panel_top + 18, ICON_FOLDER, "PROJECT");
    if (icon_button(app.panel_area.x + app.panel_area.w - 43, app.panel_top + 10, 32, ICON_REFRESH,
                    "", "Refresh project files", false, true))
        tree_refresh();
    app.tree_view.content = app.tree_count * 23.f;
    app.tree_view.viewport = (float)(app.panel_area.h - 46);
    ui_scroll_clamp(&app.tree_view);
    UiRect track = {app.panel_area.x + app.panel_area.w - 15, app.panel_top + 46, 8,
                    app.tree_view.viewport};
    ui_scrollbar(&app.ui, UI_TREE_SCROLL, track, &app.tree_view, UI_AXIS_Y);
    app.tree_scroll = (int)(app.tree_view.offset / 23);
    UiRect tree_clip = {app.panel_area.x, app.panel_top + 46, app.panel_area.w - 18,
                        app.tree_view.viewport};
    ui_clip_push(&app.ui, tree_clip);
    ui_apply_clip(app.renderer, &app.ui);
    int visible = (int)(app.tree_view.viewport / 23) + 2;
    for (int i = app.tree_scroll; i < app.tree_count && i < app.tree_scroll + visible; i++) {
        TreeRow row = app.tree[i];
        float y = app.panel_top + 50 + i * 23 - app.tree_view.offset;
        char label[128];
        snprintf(label, sizeof label, "%.20s", row.name);
        SDL_FRect r = {app.panel_area.x + 5, y, app.panel_area.w - 18, 22};
        bool hover = ui_hovered(&app.ui, (UiRect){r.x, r.y, r.w, r.h});
        if (hover)
            box(r, UI_HOVER);
        float row_x = app.panel_area.x + 9 + row.depth * 12;
        if (row.directory)
            ui_icon(app.renderer, row.expanded ? ICON_DOWN : ICON_RIGHT, row_x, y + 5, 12,
                    UI_MUTED);
        ui_icon(app.renderer,
                row.directory              ? ICON_FOLDER
                : suffix(row.path, ".png") ? ICON_IMAGE
                                           : ICON_FILE,
                row_x + 16, y + 3, 16, row.directory ? UI_ACCENT : UI_MUTED);
        text(row_x + 40, y + 7, label, UI_TEXT);
        if (app.click && hover && !app.playing) {
            if (row.directory) {
                tree_toggle(row.path);
                break;
            }
            if (suffix(row.path, ".js")) {
                char path[1024];
                snprintf(path, sizeof path, "%s/%s", app.root, row.path);
                request(REQUEST_SCRIPT_OPEN, path);
            } else if (suffix(row.path, ".png")) {
                char path[1024];
                snprintf(path, sizeof path, "%s/%s", app.root, row.path);
                request(REQUEST_IMPORT, path);
            } else if (suffix(row.path, ".json") && strcmp(row.path, "project.json")) {
                char path[1024];
                snprintf(path, sizeof path, "%s/%s", app.root, row.path);
                request(REQUEST_OPEN, path);
            }
        }
    }
    ui_clip_pop(&app.ui);
    ui_apply_clip(app.renderer, &app.ui);
    ui_draw_scrollbar(app.renderer, &app.ui, UI_TREE_SCROLL, track, &app.tree_view, UI_AXIS_Y);
}
static void disclosure(float x, float y, float w, const char *title, bool *open) {
    UiRect rect = {x, y - 6, w, 26};
    ui_disclosure(&app.ui, ui_id(title), rect, open, true);
    ui_draw_button(app.renderer, (SDL_FRect){rect.x, rect.y, rect.w, rect.h},
                   *open ? ICON_DOWN : ICON_RIGHT, title, false, true, ui_hovered(&app.ui, rect));
}
static void draw_inspector(void) {
    float x = (float)(app.panel_area.x + 9);
    char label[128], value[64];
    bool enabled = !app.playing;
    float w = app.panel_area.w - 32, half = (w - 8) / 2;
    UiRect panel = {(float)(app.panel_area.x + 5), app.panel_top + 8,
                    (float)(app.panel_area.w - 18), (float)(app.panel_area.h - 16)};
    ui_scroll_clamp(&app.inspector_view);
    float y = app.panel_top + 18 - app.inspector_view.offset;
    ui_clip_push(&app.ui, panel);
    ui_apply_clip(app.renderer, &app.ui);
    disclosure(x, y, w, "Tilemap / Layers", &app.layers_open);
    y += 32;
    if (app.layers_open) {
        snprintf(label, sizeof label, "Map: %dx%d  tile: %d", app.map->scene.width,
                 app.map->scene.height, app.map->scene.tile_size);
        text(x, y, label, UI_TEXT);
        y += 22;
        snprintf(label, sizeof label, "PNG tile: %d (change)", app.map->scene.tileset_tile_size);
        if (button(x, y, w, label, false, enabled)) {
            snprintf(value, sizeof value, "%d", app.map->scene.tileset_tile_size);
            field_start(FIELD_TILESIZE, value);
        }
        y += 32;
        for (int l = 0; l < app.map->scene.layer_count; l++) {
            app.widget_scope = ui_id("layer_row") ^ (UiId)(l + 1);
            snprintf(label, sizeof label, "%d %.20s", l + 1, app.map->scene.layers[l].name);
            if (button(x, y, w - 50, label, l == app.layer, enabled))
                app.layer = l;
            if (icon_button(x + w - 44, y, 44,
                            app.map->scene.layers[l].visible ? ICON_EYE : ICON_EYE_OFF, "",
                            "Toggle layer visibility", false, enabled) &&
                begin()) {
                app.map->scene.layers[l].visible = !app.map->scene.layers[l].visible;
                commit();
            }
            y += 30;
        }
        app.widget_scope = 0;
        if (icon_button(x, y, half, ICON_PLUS, "Add layer", NULL, false,
                        enabled && app.map->scene.layer_count < PE_LAYERS) &&
            begin()) {
            app.layer = app.map->scene.layer_count++;
            memset(&app.map->scene.layers[app.layer], 0, sizeof(PeLayer));
            snprintf(app.map->scene.layers[app.layer].name, 32, "Layer %d", app.layer + 1);
            app.map->scene.layers[app.layer].visible = true;
            commit();
        }
        if (icon_button(x + half + 8, y, half, ICON_DELETE, "Delete", "Delete layer", false,
                        enabled && app.map->scene.layer_count > 1) &&
            begin()) {
            memmove(&app.map->scene.layers[app.layer], &app.map->scene.layers[app.layer + 1],
                    (size_t)(app.map->scene.layer_count - app.layer - 1) * sizeof(PeLayer));
            app.map->scene.layer_count--;
            memset(&app.map->scene.layers[app.map->scene.layer_count], 0, sizeof(PeLayer));
            if (app.layer >= app.map->scene.layer_count)
                app.layer--;
            commit();
        }
        y += 32;
        if (icon_button(x, y, half, ICON_EDIT, "Rename", NULL, false, enabled))
            field_start(FIELD_LAYER, app.map->scene.layers[app.layer].name);
        if (icon_button(x + half + 8, y, (half - 8) / 2, ICON_UP, "", "Move layer up", false,
                        enabled && app.layer > 0) &&
            begin()) {
            PeLayer temp = app.map->scene.layers[app.layer];
            app.map->scene.layers[app.layer] = app.map->scene.layers[app.layer - 1];
            app.map->scene.layers[--app.layer] = temp;
            commit();
        }
        if (icon_button(x + half + 8 + (half + 8) / 2, y, (half - 8) / 2, ICON_DOWN, "",
                        "Move layer down", false,
                        enabled && app.layer + 1 < app.map->scene.layer_count) &&
            begin()) {
            PeLayer temp = app.map->scene.layers[app.layer];
            app.map->scene.layers[app.layer] = app.map->scene.layers[app.layer + 1];
            app.map->scene.layers[++app.layer] = temp;
            commit();
        }
        y += 40;
    }
    disclosure(x, y, w, "Entity", &app.entity_open);
    y += 32;
    if (app.entity_open) {
        if (icon_button(x, y, half, ICON_PLUS, "New", "Create entity", false, enabled) && begin()) {
            PeEntity e = pe_entity_create(&app.map->scene, PE_TRANSFORM | PE_SPRITE | PE_BODY);
            PeObject *o = pe_entity_get(&app.map->scene, e);
            if (o) {
                o->x = 64;
                o->y = 64;
                app.selected = o->stable_id;
                app.tool = SELECT;
            } else
                message("Entity limit reached");
            commit();
        }
        PeObject *o = pe_entity_get(&app.map->scene, pe_entity_find(&app.map->scene, app.selected));
        if (icon_button(x + half + 8, y, half, ICON_DELETE, "Delete", "Delete entity", false,
                        enabled && o) &&
            begin()) {
            pe_entity_destroy(&app.map->scene, pe_entity_find(&app.map->scene, app.selected));
            app.selected = 0;
            commit();
            o = NULL;
        }
        y += 34;
        if (o) {
            snprintf(label, sizeof label, "Entity #%u", o->stable_id);
            text(x, y, label, UI_TEXT);
            y += 20;
            float values[] = {o->x, o->y, o->w, o->h};
            const char *names[] = {"X", "Y", "Width", "Height"};
            Field fields[] = {FIELD_X, FIELD_Y, FIELD_W, FIELD_H};
            for (int i = 0; i < 4; i++) {
                app.widget_scope = ui_id("entity_field") ^ o->stable_id * 31u ^ (UiId)i;
                snprintf(label, sizeof label, "%s: %.2f", names[i], values[i]);
                if (button(x, y, w, label, false, enabled)) {
                    snprintf(value, sizeof value, "%.2f", values[i]);
                    field_start(fields[i], value);
                }
                y += 29;
            }
            app.widget_scope = 0;
            if (button(x, y, w, o->mask & PE_PLAYER ? "Player control: ON" : "Player control: OFF",
                       false, enabled) &&
                begin()) {
                o->mask ^= PE_PLAYER;
                commit();
            }
            y += 35;
        } else {
            text(x, y, "Select an entity in the view.", UI_MUTED);
            y += 28;
        }
    }
    disclosure(x, y, w, "Tile palette", &app.palette_open);
    y += 32;
    if (app.palette_open) {
        int total = app.map->assets.tileset
                        ? (app.map->assets.width / app.map->assets.source_tile) *
                              (app.map->assets.height / app.map->assets.source_tile)
                        : 0,
            columns = (int)(w / 37);
        if (columns < 1)
            columns = 1;
        for (int i = 0; i < total; i++) {
            int row = i / columns, col = i % columns;
            SDL_FRect dst = {x + col * 37, y + row * 37, 32, 32};
            if (dst.y + dst.h < panel.y || dst.y > panel.y + panel.h)
                continue;
            int source_columns = app.map->assets.width / app.map->assets.source_tile;
            SDL_FRect src = {(float)(i % source_columns * app.map->assets.source_tile),
                             (float)(i / source_columns * app.map->assets.source_tile),
                             (float)app.map->assets.source_tile,
                             (float)app.map->assets.source_tile};
            SDL_RenderTexture(app.renderer, app.map->assets.tileset, &src, &dst);
            if (app.tile == i + 1) {
                draw_color(UI_ACCENT);
                SDL_RenderRect(app.renderer, &dst);
            }
            if (enabled && app.click && ui_hovered(&app.ui, (UiRect){dst.x, dst.y, dst.w, dst.h})) {
                app.tile = i + 1;
                if (app.tool == SELECT || app.tool == ERASE)
                    app.tool = BRUSH;
            }
        }
        y += ((total + columns - 1) / columns) * 37;
    }
    app.inspector_view.content = y + app.inspector_view.offset - app.panel_top + 8;
    app.inspector_view.viewport = (float)(app.panel_area.h - 16);
    ui_scroll_clamp(&app.inspector_view);
    ui_clip_pop(&app.ui);
    ui_apply_clip(app.renderer, &app.ui);
    UiRect track = {(float)(app.panel_area.x + app.panel_area.w - 15), app.panel_top + 8, 8,
                    app.inspector_view.viewport};
    ui_scrollbar(&app.ui, UI_INSPECTOR_SCROLL, track, &app.inspector_view, UI_AXIS_Y);
    ui_draw_scrollbar(app.renderer, &app.ui, UI_INSPECTOR_SCROLL, track, &app.inspector_view,
                      UI_AXIS_Y);
}
static void draw_script_inspector(void) {
    float x = (float)(app.panel_area.x + 9), y = app.panel_top + 18 - app.inspector_view.offset;
    UiRect clip = {(float)(app.panel_area.x + 5), app.panel_top + 8, (float)(app.panel_area.w - 18),
                   (float)(app.panel_area.h - 16)};
    ui_clip_push(&app.ui, clip);
    ui_apply_clip(app.renderer, &app.ui);
    section(x, y, ICON_FILE, "JAVASCRIPT");
    text(x, y + 28, "Parser: QuickJS-ng", UI_TEXT);
    text(x, y + 48, "Compile only / no execution", UI_MUTED);
    section(x, y + 90, ICON_EDIT, "EDITOR SHORTCUTS");
    const char *help[] = {
        "Ctrl+S     Save script",  "Ctrl+Z     Undo",       "Ctrl+Shift+Z  Redo",
        "Ctrl+Space  Suggestions", "Enter / Tab  Accept",   "F7         Check syntax",
        "Shift+arrows  Select",    "Ctrl+C/X/V  Clipboard", "Shift+wheel  Horizontal"};
    for (int i = 0; i < 9; i++)
        text(x, y + 116 + i * 24, help[i], UI_MUTED);
    text(x, y + 354, "Completion: keywords,", UI_TEXT);
    text(x, y + 374, "standard JS objects and", UI_TEXT);
    text(x, y + 394, "document identifiers.", UI_TEXT);
    text(x, y + 426, "Type analysis / LSP later.", UI_MUTED);
    app.inspector_view.content = 460;
    app.inspector_view.viewport = (float)(app.panel_area.h - 16);
    ui_scroll_clamp(&app.inspector_view);
    ui_clip_pop(&app.ui);
    ui_apply_clip(app.renderer, &app.ui);
    UiRect track = {(float)(app.panel_area.x + app.panel_area.w - 15), app.panel_top + 8, 8,
                    app.inspector_view.viewport};
    ui_scrollbar(&app.ui, UI_INSPECTOR_SCROLL, track, &app.inspector_view, UI_AXIS_Y);
    ui_draw_scrollbar(app.renderer, &app.ui, UI_INSPECTOR_SCROLL, track, &app.inspector_view,
                      UI_AXIS_Y);
}
static void draw_modal(void) {
    SDL_SetRenderDrawBlendMode(app.renderer, SDL_BLENDMODE_BLEND);
    box((SDL_FRect){0, 0, (float)app.width, (float)app.height}, 0x000000a0);
    SDL_SetRenderDrawBlendMode(app.renderer, SDL_BLENDMODE_NONE);
    float x = app.width / 2.f - 310, y = app.height / 2.f - 95;
    ui_panel(app.renderer, (SDL_FRect){x, y, 620, 190}, UI_SURFACE);
    if (app.request != REQUEST_NONE) {
        text(x + 20, y + 24, "Unsaved changes", UI_ACCENT);
        text(x + 20, y + 54, "Save before opening another scene or closing?", UI_TEXT);
        if (button(x + 20, y + 110, 170, "Save and continue", false, true))
            app.deferred_save = true;
        if (button(x + 210, y + 110, 170, "Discard changes", false, true))
            request_finish();
        if (button(x + 400, y + 110, 170, "Cancel", false, true))
            app.request = REQUEST_NONE;
    } else {
        const char *title = app.field == FIELD_IMPORT  ? "Import PNG: enter full file path"
                            : app.field == FIELD_LAYER ? "Layer name"
                                                       : "Enter value";
        text(x + 20, y + 24, title, UI_ACCENT);
        box((SDL_FRect){x + 20, y + 54, 580, 28}, UI_CANVAS);
        size_t n = strlen(app.input);
        text(x + 26, y + 64, app.input + (n > 69 ? n - 69 : 0), UI_TEXT);
        text(x + 20, y + 94, "Enter: apply   Escape: cancel   Ctrl+A: clear", UI_MUTED);
        if (button(x + 20, y + 125, 110, "Apply", false, true))
            field_apply();
        if (button(x + 145, y + 125, 110, "Cancel", false, true))
            field_stop();
    }
    text(x + 20, y + 164, app.status, UI_ACCENT);
}
static UiRect left_split(void) {
    return (UiRect){LEFT - 3, TOP, 6, (float)(app.height - TOP - BOTTOM)};
}
static UiRect right_split(void) {
    return (UiRect){(float)(app.width - RIGHT - 3), TOP, 6, (float)(app.height - TOP - BOTTOM)};
}
static void layout_columns(void) {
    ui_columns_fit((float)app.width, &app.left_width, &app.right_width, app.min_left, app.min_right,
                   app.min_center);
    ui_drag_value(&app.ui, UI_SPLIT_LEFT, left_split(), &app.left_width, app.min_left,
                  app.width - app.right_width - app.min_center, UI_AXIS_X, 1);
    ui_drag_value(&app.ui, UI_SPLIT_RIGHT, right_split(), &app.right_width, app.min_right,
                  app.width - app.left_width - app.min_center, UI_AXIS_X, -1);
}
static bool ui_controls_hit(float x, float y) {
    if (ui_contains(left_split(), x, y) || ui_contains(right_split(), x, y))
        return true;
    for (int i = 0; i < app.hit_count; i++)
        if (ui_contains(app.hits[i], x, y))
            return true;
    return app.script && se_scroll_hit(app.script, x, y, app.viewport);
}
static UiId yaml_id(const char *name) {
    if (!strcmp(name, "explorer_tab"))
        return UI_EXPLORER_TAB;
    if (!strcmp(name, "scene_tab"))
        return UI_SCENE_TAB;
    if (!strcmp(name, "inspector_tab"))
        return UI_INSPECTOR_TAB;
    if (!strcmp(name, "info_tab"))
        return UI_INFO_TAB;
    return ui_id(name);
}
static UiIcon yaml_icon(const char *name) {
    static const struct {
        const char *name;
        UiIcon icon;
    } names[] = {{"menu", ICON_MENU},           {"save", ICON_SAVE},     {"undo", ICON_UNDO},
                 {"redo", ICON_REDO},           {"play", ICON_PLAY},     {"stop", ICON_STOP},
                 {"import", ICON_IMPORT},       {"brush", ICON_BRUSH},   {"erase", ICON_ERASE},
                 {"rectangle", ICON_RECTANGLE}, {"fill", ICON_FILL},     {"select", ICON_SELECT},
                 {"collision", ICON_COLLISION}, {"grid", ICON_GRID},     {"refresh", ICON_REFRESH},
                 {"folder", ICON_FOLDER},       {"file", ICON_FILE},     {"image", ICON_IMAGE},
                 {"layers", ICON_LAYERS},       {"entity", ICON_ENTITY}, {"plus", ICON_PLUS},
                 {"delete", ICON_DELETE},       {"edit", ICON_EDIT},     {"up", ICON_UP},
                 {"down", ICON_DOWN},           {"right", ICON_RIGHT},   {"eye", ICON_EYE},
                 {"eye_off", ICON_EYE_OFF},     {"check", ICON_CHECK},   {"close", ICON_CLOSE}};
    for (size_t i = 0; i < sizeof names / sizeof names[0]; i++)
        if (!strcmp(names[i].name, name))
            return names[i].icon;
    return ICON_NONE;
}
static const cJSON *editor_entry(UiId id) {
    if (!app.shell_layout)
        return NULL;
    const char *module = id == UI_MAP_VIEW ? "map" : "scripts";
    const cJSON *entries = pe_ui_get(app.shell_layout->root, "editors");
    for (const cJSON *e = entries ? entries->child : NULL; e; e = e->next)
        if (!strcmp(pe_ui_string(e, "module", ""), module))
            return e;
    return NULL;
}
static PeUiLayout *load_editor_layout(UiId id, char *error, size_t size) {
    const cJSON *entry = editor_entry(id);
    if (!entry) {
        snprintf(error, size, "Editor is not listed in shell.yaml");
        return NULL;
    }
    char path[1400], directory[1024];
    snprintf(directory, sizeof directory, "%s", app.ui_path);
    char *last = strrchr(directory, '/'), *back = strrchr(directory, '\\');
    if (back && (!last || back > last))
        last = back;
    if (last)
        last[1] = 0;
    else
        directory[0] = 0;
    if (snprintf(path, sizeof path, "%s%s", directory, pe_ui_string(entry, "file", "")) >=
        (int)sizeof path)
        return NULL;
    PeUiLayout *layout = pe_ui_layout_load(path, "editor", error, size);
    if (layout && strcmp(pe_ui_string(layout->root, "id", ""), pe_ui_string(entry, "id", ""))) {
        pe_ui_layout_free(layout);
        snprintf(error, size, "%s:1:1: editor id does not match shell entry", path);
        return NULL;
    }
    return layout;
}
static void apply_shell_layout(void) {
    const cJSON *root = app.shell_layout->root, *layout = pe_ui_get(root, "layout"),
                *cols = pe_ui_get(layout, "columns");
    app.gap = pe_ui_get(layout, "gap")->valueint;
    app.tab_gap = pe_ui_get(layout, "tab_gap")->valueint;
    app.min_left = (float)pe_ui_get(pe_ui_get(cols, "left"), "minimum")->valuedouble;
    app.min_right = (float)pe_ui_get(pe_ui_get(cols, "right"), "minimum")->valuedouble;
    app.min_center = (float)pe_ui_get(pe_ui_get(cols, "center"), "minimum")->valuedouble;
    if (!app.loaded_view) {
        app.left_width = (float)pe_ui_get(pe_ui_get(cols, "left"), "initial")->valuedouble;
        app.right_width = (float)pe_ui_get(pe_ui_get(cols, "right"), "initial")->valuedouble;
    }
    UiTheme theme = ui_dark_theme();
    const cJSON *colors = pe_ui_get(root, "theme");
    const char *names[] = {"background", "panel", "surface", "hover", "accent", "text", "muted"};
    Uint32 *values[] = {&theme.canvas, &theme.panel, &theme.surface, &theme.hover,
                        &theme.accent, &theme.text,  &theme.muted};
    for (int i = 0; i < 7; i++) {
        const char *v = pe_ui_string(colors, names[i], NULL);
        if (v)
            *values[i] = ((Uint32)strtoul(v + 1, NULL, 16) << 8) | 255u;
    }
    ui_set_theme(&theme);
    UiTab items[UI_MAX_TABS];
    int count = 0;
    UiId selected = app.loaded_view;
    const cJSON *entries = pe_ui_get(root, "editors");
    for (const cJSON *e = entries->child; e; e = e->next)
        items[count++] = (UiTab){
            !strcmp(pe_ui_string(e, "module", ""), "map") ? UI_MAP_VIEW : UI_SCRIPT_VIEW, true, 0};
    bool found = false;
    for (int i = 0; i < count; i++)
        if (items[i].id == selected)
            found = true;
    if (!found)
        selected = items[0].id;
    ui_tabs_init(&app.workspaces, items, count, selected);
    app.workspaces.gap = (float)app.tab_gap;
}
static bool yaml_boolean(const cJSON *value, bool fallback) {
    if (!value)
        return fallback;
    if (cJSON_IsBool(value))
        return cJSON_IsTrue(value);
    const char *name = pe_ui_string(value, "binding", "");
    if (!strcmp(name, "document.can_save"))
        return (app.map || app.script) && !app.playing && !edit_pending();
    if (!strcmp(name, "document.can_undo"))
        return !app.playing && !edit_pending() &&
               (app.script ? app.script->document.position > 0
                           : app.map && app.map->history.cursor > 0);
    if (!strcmp(name, "document.can_redo"))
        return !app.playing && !edit_pending() &&
               (app.script ? app.script->document.position < app.script->document.count
                           : app.map && app.map->history.cursor < app.map->history.count);
    if (!strcmp(name, "map.grid_visible"))
        return app.map && app.grid;
    if (!strcmp(name, "map.collision_visible"))
        return app.map && app.collision;
    return false;
}
static const char *yaml_text(const cJSON *value, char *buffer, size_t size) {
    if (cJSON_IsString(value))
        return value->valuestring;
    const char *b = pe_ui_string(value, "binding", "");
    if (!strcmp(b, "editor.mode"))
        return app.playing ? "PLAY" : "EDIT";
    if (!strcmp(b, "document.save_state"))
        return (scripts_active() ? script_dirty() : map_dirty()) ? "Modified"
               : app.script && !app.script->exists               ? "Untitled"
                                                                 : "Saved";
    if (!strcmp(b, "editor.message"))
        return app.script ? app.script->status : app.status;
    if (!strcmp(b, "editor.details")) {
        if (app.script) {
            int l, c;
            pe_text_location(&app.script->document, app.script->document.cursor, &l, &c);
            snprintf(buffer, size, "JS | Ln %d, Col %d | Undo %d / %d", l + 1, c + 1,
                     app.script->document.position, app.script->document.count);
        } else if (app.map)
            snprintf(buffer, size, "Zoom %.0f%% | Tile %d | Undo %d / %d", app.camera.zoom * 100,
                     app.tile, app.map->history.cursor, app.map->history.count);
        else
            snprintf(buffer, size, "No editor loaded");
        return buffer;
    }
    return "";
}
static void yaml_action(const char *name) {
    if (!name)
        return;
    if (!strcmp(name, "document.save")) {
        if (!app.playing && !edit_pending() && (app.map || app.script))
            save();
        return;
    }
    if (!strcmp(name, "document.undo")) {
        if ((app.map || app.script) && !app.playing && !edit_pending())
            undo_active(false);
        return;
    }
    if (!strcmp(name, "document.redo")) {
        if ((app.map || app.script) && !app.playing && !edit_pending())
            undo_active(true);
        return;
    }
    if (!strcmp(name, "script.check")) {
        if (app.script) {
            se_check(app.script);
            ui_focus(&app.ui, UI_SCRIPT_DOCUMENT);
            se_focus(app.script, app.window, true);
        }
        return;
    }
    if (!app.map)
        return;
    if (!strcmp(name, "scene.toggle_play")) {
        play_toggle();
        return;
    }
    if (app.playing || edit_pending() || !app.loaded_view)
        return;
    if (!strcmp(name, "map.import_tileset"))
        field_start(FIELD_IMPORT, "");
    else if (!strcmp(name, "map.toggle_grid"))
        app.grid = !app.grid;
    else if (!strcmp(name, "map.toggle_collision"))
        app.collision = !app.collision;
    else {
        const char *names[] = {"map.select_brush", "map.select_eraser", "map.select_rectangle",
                               "map.select_fill", "map.select_entity"};
        for (int i = 0; i < 5; i++)
            if (!strcmp(name, names[i]))
                app.tool = i;
    }
}
static bool yaml_action_enabled(const char *action) {
    if (!action || !strcmp(action, "scene.toggle_play"))
        return !edit_pending();
    if (!strncmp(action, "map.", 4))
        return app.map && !app.playing && !edit_pending();
    return true;
}
static bool yaml_action_active(const char *action) {
    if (!action)
        return false;
    if (!strcmp(action, "scene.toggle_play"))
        return app.playing;
    const char *names[] = {"map.select_brush", "map.select_eraser", "map.select_rectangle",
                           "map.select_fill", "map.select_entity"};
    for (int i = 0; i < 5; i++)
        if (!strcmp(action, names[i]))
            return app.map && app.tool == i;
    return false;
}
static float yaml_button_width(const cJSON *n) {
    const char *label = pe_ui_string(n, "label", "");
    return *label ? ui_tab_content_width(yaml_icon(pe_ui_string(n, "icon", "")), label) : 32;
}
static void yaml_button(const cJSON *n, SDL_FRect area) {
    const char *action = pe_ui_string(n, "action", NULL);
    register_ui_hit((UiRect){area.x, area.y, area.w, area.h});
    bool enabled = yaml_boolean(pe_ui_get(n, "enabled"), true) && yaml_action_enabled(action);
    bool active = yaml_boolean(pe_ui_get(n, "active"), yaml_action_active(action));
    UiIcon icon = yaml_icon(pe_ui_string(n, "icon", ""));
    if (action && !strcmp(action, "scene.toggle_play") && app.playing)
        icon = ICON_STOP;
    bool hover = ui_hovered(&app.ui, (UiRect){area.x, area.y, area.w, area.h});
    char buffer[256];
    const char *label = yaml_text(pe_ui_get(n, "label"), buffer, sizeof buffer);
    ui_draw_button(app.renderer, area, icon, label, active, enabled, hover);
    if (hover)
        tooltip = pe_ui_string(n, "tooltip", NULL);
    bool shell = false;
    for (int i = 0; i < app.shell_layout->mark_count; i++)
        if (app.shell_layout->marks[i].value == n) {
            shell = true;
            break;
        }
    UiId id = ui_id(pe_ui_string(n, "id", "")) ^ ui_id(shell          ? "shell"
                                                       : map_active() ? "map"
                                                                      : "scripts");
    if (!id)
        id = 1;
    if (ui_button(&app.ui, id, (UiRect){area.x, area.y, area.w, area.h}, enabled))
        yaml_action(action);
}
static int yaml_toolbar_rows(float width) {
    const cJSON *list = pe_ui_get(app.active_layout->root, "toolbar");
    float x = 8;
    int rows = 1;
    for (const cJSON *n = list->child; n; n = n->next) {
        if (!yaml_boolean(pe_ui_get(n, "visible"), true))
            continue;
        float w = !strcmp(pe_ui_string(n, "type", ""), "separator")
                      ? 16
                      : fminf(yaml_button_width(n), fmaxf(32, width - 16));
        if (x + w > width - 8) {
            x = 8;
            rows++;
        }
        x += w + 6;
    }
    return rows;
}
static void draw_yaml_toolbar(SDL_FRect area) {
    const cJSON *list = pe_ui_get(app.active_layout->root, "toolbar");
    float x = area.x + 8, y = area.y + 6;
    for (const cJSON *n = list->child; n; n = n->next) {
        if (!yaml_boolean(pe_ui_get(n, "visible"), true))
            continue;
        bool separator = !strcmp(pe_ui_string(n, "type", ""), "separator");
        float w = separator ? 16 : fminf(yaml_button_width(n), fmaxf(32, area.w - 16));
        if (x + w > area.x + area.w - 8) {
            x = area.x + 8;
            y += 32;
        }
        if (separator)
            box((SDL_FRect){x + 7, y + 4, 1, 17}, UI_BORDER);
        else
            yaml_button(n, (SDL_FRect){x, y, w, 25});
        x += w + 6;
    }
}
static void draw_global_tools(void) {
    const cJSON *list = pe_ui_get(app.shell_layout->root, "toolbar");
    float x = 14;
    for (const cJSON *n = list->child; n; n = n->next) {
        if (!yaml_boolean(pe_ui_get(n, "visible"), true))
            continue;
        const char *type = pe_ui_string(n, "type", "");
        if (!strcmp(type, "editor_switcher")) {
            app.global_nav_x = (int)x;
            return;
        }
        if (!strcmp(type, "separator")) {
            box((SDL_FRect){x + 8, 16, 1, 17}, UI_BORDER);
            x += 22;
        } else {
            float w = yaml_button_width(n);
            yaml_button(n, (SDL_FRect){x, 12, w, 25});
            x += w + 6;
        }
    }
    app.global_nav_x = (int)x;
}
static int widget_state(UiId id) {
    for (int i = 0; i < app.widget_count; i++)
        if (app.widgets[i].id == id)
            return i;
    if (app.widget_count == 64)
        return -1;
    int i = app.widget_count++;
    memset(&app.widgets[i], 0, sizeof app.widgets[i]);
    app.widgets[i].id = id;
    app.widgets[i].open = -1;
    return i;
}
static void draw_yaml_node(const cJSON *n, SDL_FRect area, int column) {
    if (!n || area.w <= 0 || area.h <= 0 || !yaml_boolean(pe_ui_get(n, "visible"), true))
        return;
    const char *type = pe_ui_string(n, "type", "");
    UiId id = yaml_id(pe_ui_string(n, "id", ""));
    bool previous_blocked = app.ui.blocked;
    app.ui.blocked = previous_blocked || !yaml_boolean(pe_ui_get(n, "enabled"), true);
    ui_clip_push(&app.ui, (UiRect){area.x, area.y, area.w, area.h});
    ui_apply_clip(app.renderer, &app.ui);
    if (!strcmp(type, "tabs")) {
        int state = widget_state(id);
        if (state < 0)
            goto done;
        const cJSON *list = pe_ui_get(n, "tabs");
        int count = cJSON_GetArraySize(list);
        UiTabs temporary = {0};
        UiTabs *tabs = column == 0 ? &app.left_tabs : column == 2 ? &app.right_tabs : &temporary;
        if (column != 0 && column != 2) {
            tabs->selected = app.widgets[state].selected;
            tabs->count = app.widgets[state].count;
            tabs->scroll = app.widgets[state].scroll;
            tabs->dragging = app.widgets[state].dragging != 0;
            for (int i = 0; i < tabs->count; i++)
                tabs->items[i] = (UiTab){app.widgets[state].order[i], true, 0};
        }
        UiTab items[UI_MAX_TABS];
        int used = 0;
        for (int k = 0; k < tabs->count; k++)
            for (const cJSON *t = list->child; t; t = t->next)
                if (tabs->items[k].id == yaml_id(pe_ui_string(t, "id", "")))
                    items[used++] = (UiTab){tabs->items[k].id, true, 0};
        for (const cJSON *t = list->child; t; t = t->next) {
            UiId key = yaml_id(pe_ui_string(t, "id", ""));
            bool found = false;
            for (int k = 0; k < used; k++)
                if (items[k].id == key)
                    found = true;
            if (!found)
                items[used++] = (UiTab){key, true, 0};
        }
        bool found = false;
        for (int k = 0; k < count; k++)
            if (items[k].id == tabs->selected)
                found = true;
        UiId selected = found ? tabs->selected : items[0].id;
        float scroll = tabs->scroll;
        bool dragging = tabs->dragging;
        ui_tabs_init(tabs, items, count, selected);
        tabs->scroll = scroll;
        tabs->dragging = dragging;
        tabs->gap = (float)app.tab_gap;
        for (const cJSON *t = list->child; t; t = t->next)
            ui_tabs_set_width(tabs, yaml_id(pe_ui_string(t, "id", "")),
                              ui_tab_content_width(yaml_icon(pe_ui_string(t, "icon", "")),
                                                   pe_ui_string(t, "title", "")) +
                                  app.tab_gap);
        UiRect bar = {area.x, area.y, area.w, TABS - PANEL_GAP};
        register_ui_hit(bar);
        if (column != 0 && column != 2 && app.wheel_delta && ui_contains(bar, app.mx, app.my)) {
            ui_tabs_scroll(tabs, bar.w, 112, -app.wheel_delta * 56);
            app.wheel_delta = 0;
        }
        ui_tabs_update(&app.ui, tabs, bar, 112);
        for (int k = 0; k < count; k++)
            for (const cJSON *t = list->child; t; t = t->next)
                if (tabs->items[k].id == yaml_id(pe_ui_string(t, "id", ""))) {
                    UiRect r = ui_tab_rect(tabs, bar, 112, k);
                    ui_draw_tab(app.renderer, (SDL_FRect){r.x, r.y, r.w, r.h},
                                yaml_icon(pe_ui_string(t, "icon", "")),
                                pe_ui_string(t, "title", ""), tabs->selected == tabs->items[k].id,
                                true, ui_hovered(&app.ui, r));
                }
        app.widgets[state].scroll = tabs->scroll;
        app.widgets[state].dragging = tabs->dragging;
        app.widgets[state].count = count;
        app.widgets[state].selected = tabs->selected;
        for (int k = 0; k < count; k++)
            app.widgets[state].order[k] = tabs->items[k].id;
        for (const cJSON *t = list->child; t; t = t->next)
            if (tabs->selected == yaml_id(pe_ui_string(t, "id", "")))
                draw_yaml_node(pe_ui_get(t, "content"),
                               (SDL_FRect){area.x, area.y + bar.h, area.w, area.h - bar.h}, -1);
    } else if (!strcmp(type, "row") || !strcmp(type, "column") || !strcmp(type, "section")) {
        bool row = !strcmp(type, "row");
        float gap = (float)(pe_ui_get(n, "gap") ? pe_ui_get(n, "gap")->valuedouble : PANEL_GAP);
        const cJSON *children = pe_ui_get(n, "children");
        int count = 0;
        for (const cJSON *child = children->child; child; child = child->next)
            if (yaml_boolean(pe_ui_get(child, "visible"), true))
                count++;
        if (!strcmp(type, "section")) {
            int state = widget_state(id);
            if (state < 0)
                goto done;
            if (app.widgets[state].open < 0)
                app.widgets[state].open = cJSON_IsTrue(pe_ui_get(n, "open"));
            bool open = app.widgets[state].open != 0;
            ui_panel(app.renderer, area, UI_PANEL);
            ui_disclosure(&app.ui, id, (UiRect){area.x, area.y, area.w, 26}, &open, true);
            app.widgets[state].open = open;
            ui_draw_button(app.renderer, (SDL_FRect){area.x, area.y, area.w, 26},
                           open ? ICON_DOWN : ICON_RIGHT, pe_ui_string(n, "title", ""), false, true,
                           false);
            if (!open)
                goto done;
            area.y += 30;
            area.h -= 30;
        }
        float size = (fmaxf(0, (row ? area.w : area.h) - gap * (count - 1))) / (count ? count : 1);
        int i = 0;
        for (const cJSON *child = children->child; child; child = child->next) {
            if (!yaml_boolean(pe_ui_get(child, "visible"), true))
                continue;
            draw_yaml_node(child,
                           (SDL_FRect){area.x + (row ? i * (size + gap) : 0),
                                       area.y + (row ? 0 : i * (size + gap)), row ? size : area.w,
                                       row ? area.h : size},
                           -1);
            i++;
        }
    } else if (!strcmp(type, "label")) {
        char buffer[256];
        text(area.x + 8, area.y + 12, yaml_text(pe_ui_get(n, "label"), buffer, sizeof buffer),
             UI_TEXT);
    } else if (!strcmp(type, "button"))
        yaml_button(n, (SDL_FRect){area.x, area.y, fminf(area.w, yaml_button_width(n)), 25});
    else if (!strcmp(type, "separator"))
        box((SDL_FRect){area.x, area.y + 4, 1, fmaxf(0, area.h - 8)}, UI_BORDER);
    else if (strcmp(type, "spacer")) {
        ui_panel(app.renderer, area, UI_PANEL);
        if (strcmp(type, "map_canvas") && strcmp(type, "script_editor"))
            register_ui_hit((UiRect){area.x, area.y, area.w, area.h});
        app.panel_area = area;
        app.panel_top = area.y;
        if (!strcmp(type, "map_canvas") || !strcmp(type, "script_editor")) {
            app.viewport = area;
            app.canvas_enabled = !app.ui.blocked;
            if (!strcmp(type, "map_canvas"))
                draw_map_content();
            else
                draw_script_content();
        } else if (!strcmp(type, "file_tree"))
            draw_tree();
        else if (!strcmp(type, "entity_tree"))
            draw_scene_tree();
        else if (!strcmp(type, "inspector"))
            draw_inspector();
        else if (!strcmp(type, "scene_info"))
            draw_info();
        else if (!strcmp(type, "script_info"))
            draw_script_inspector();
    }
done:
    app.ui.blocked = previous_blocked;
    ui_clip_pop(&app.ui);
    ui_apply_clip(app.renderer, &app.ui);
}
static bool reload_layout(void) {
    if (edit_pending() || app.playing || app.request != REQUEST_NONE || app.field != FIELD_NONE)
        return false;
    char error[256];
    PeUiLayout *next = pe_ui_layout_load(app.ui_path, "shell", error, sizeof error);
    if (!next) {
        message(error);
        return false;
    }
    PeUiLayout *old_shell = app.shell_layout;
    app.shell_layout = next;
    UiId id = app.loaded_view;
    PeUiLayout *candidate = load_editor_layout(id, error, sizeof error);
    if (!candidate) {
        app.shell_layout = old_shell;
        pe_ui_layout_free(next);
        message(error);
        return false;
    }
    int index = id == UI_MAP_VIEW ? 0 : 1;
    if (!pe_session_save(app.session_paths[index], id, editor_codec, &id, error, sizeof error)) {
        app.shell_layout = old_shell;
        pe_ui_layout_free(next);
        pe_ui_layout_free(candidate);
        message(error);
        return false;
    }
    app.session_exists[index] = true;
    app.session_dirty[index] = index == 0 ? map_dirty() : script_dirty();
    PeUiLayout *old_layout = app.active_layout;
    app.active_layout = NULL;
    release_editor();
    app.active_layout = candidate;
    if (activate_editor(id, error, sizeof error)) {
        pe_ui_layout_free(old_layout);
        pe_ui_layout_free(old_shell);
        apply_shell_layout();
        message("UI reloaded (F6)");
        return true;
    }
    char original[256];
    snprintf(original, sizeof original, "%s", error);
    app.shell_layout = old_shell;
    pe_ui_layout_free(next);
    app.active_layout = old_layout;
    activate_editor(id, error, sizeof error);
    message(original);
    return false;
}

static void draw_map_tools(SDL_FRect area) {
    draw_yaml_toolbar(area);
}
static void draw_script_tools(SDL_FRect area) {
    draw_yaml_toolbar(area);
}
static void draw_map_content(void) {
    bool modal = app.field != FIELD_NONE || app.request != REQUEST_NONE;
    /* Distinguish the workspace from empty/transparent tiles inside the map. */
    box(app.viewport, 0x383838ffu);
    SDL_Rect map_clip = {(int)app.viewport.x, (int)app.viewport.y, (int)app.viewport.w,
                         (int)app.viewport.h};
    SDL_SetRenderClipRect(app.renderer, &map_clip);
    const PeScene *visible_scene = app.playing ? &app.map->runtime : &app.map->scene;
    box((SDL_FRect){app.viewport.x - app.camera.x * app.camera.zoom,
                    app.viewport.y - app.camera.y * app.camera.zoom,
                    visible_scene->width * visible_scene->tile_size * app.camera.zoom,
                    visible_scene->height * visible_scene->tile_size * app.camera.zoom},
        UI_CANVAS);
    SDL_SetRenderClipRect(app.renderer, NULL);
    pe_render_scene(app.renderer, app.playing ? &app.map->runtime : &app.map->scene,
                    &app.map->assets, &app.camera, app.viewport, app.grid && !app.playing,
                    app.collision && !app.playing);
    if (!app.playing) {
        PeObject *o = pe_entity_get(&app.map->scene, pe_entity_find(&app.map->scene, app.selected));
        if (o) {
            SDL_FRect rect = {app.viewport.x + (o->x - app.camera.x) * app.camera.zoom,
                              app.viewport.y + (o->y - app.camera.y) * app.camera.zoom,
                              o->w * app.camera.zoom, o->h * app.camera.zoom};
            draw_color(UI_ACCENT);
            SDL_RenderRect(app.renderer, &rect);
        }
        if (inside(app.mx, app.my, app.viewport) && !modal && app.tool != SELECT) {
            int x, y;
            tile_coords(app.mx, app.my, &x, &y);
            int sx = app.painting && app.tool == RECTANGLE ? app.start_x : x,
                sy = app.painting && app.tool == RECTANGLE ? app.start_y : y;
            int minx = sx < x ? sx : x, miny = sy < y ? sy : y;
            SDL_FRect rect = {
                app.viewport.x + (minx * app.map->scene.tile_size - app.camera.x) * app.camera.zoom,
                app.viewport.y + (miny * app.map->scene.tile_size - app.camera.y) * app.camera.zoom,
                (abs(x - sx) + 1) * app.map->scene.tile_size * app.camera.zoom,
                (abs(y - sy) + 1) * app.map->scene.tile_size * app.camera.zoom};
            SDL_Rect clip = {(int)app.viewport.x, (int)app.viewport.y, (int)app.viewport.w,
                             (int)app.viewport.h};
            SDL_SetRenderClipRect(app.renderer, &clip);
            draw_color(UI_ACCENT);
            SDL_RenderRect(app.renderer, &rect);
            SDL_SetRenderClipRect(app.renderer, NULL);
        }
    }
}
static void draw_script_content(void) {
    se_draw(&(*app.script), app.renderer, &app.ui, app.viewport, app.mx, app.my, app.click);
}
static void process_deferred_ui(void) {
    app.wheel_delta = 0;
    app.rendering = false;
    if (app.deferred_save) {
        app.deferred_save = false;
        if (request_save())
            request_finish();
    }
    if (app.deferred_request != REQUEST_NONE) {
        Request r = app.deferred_request;
        app.deferred_request = REQUEST_NONE;
        request(r, *app.deferred_path ? app.deferred_path : NULL);
    }
    if (app.pending_view) {
        UiId id = app.pending_view;
        app.pending_view = 0;
        select_workspace(id);
    }
}
static void draw(void) {
    app.needs_layout = false;
    app.rendering = true;
    SDL_GetWindowSize(app.window, &app.width, &app.height);
    app.hit_count = 0;
    app.canvas_enabled = false;
    app.ui.blocked = app.field != FIELD_NONE || app.request != REQUEST_NONE;
    layout_columns();
    draw_color(UI_CANVAS);
    SDL_RenderClear(app.renderer);
    /* Inset panels expose the dark application background as narrow gutters. */
    box((SDL_FRect){PANEL_GAP, PANEL_GAP, (float)(app.width - 2 * PANEL_GAP), TOP - 2 * PANEL_GAP},
        UI_PANEL);
    bool modal = app.field != FIELD_NONE || app.request != REQUEST_NONE;
    bool saved_click = app.click;
    if (modal)
        app.click = false;
    app.ui.blocked = modal;
    tooltip = NULL;
    draw_global_tools();
    UiRect nav = {(float)app.global_nav_x, 12, (float)(app.width - app.global_nav_x - 14), 25};
    const cJSON *toolbar = pe_ui_get(app.shell_layout->root, "toolbar");
    const cJSON *selector = cJSON_GetArrayItem(toolbar, cJSON_GetArraySize(toolbar) - 1);
    bool labels = cJSON_IsTrue(pe_ui_get(selector, "labels"));
    for (int i = 0; i < app.workspaces.count; i++) {
        const EditorView *item = editor_view(app.workspaces.items[i].id);
        const cJSON *entry = editor_entry(item->id);
        float width = labels ? ui_tab_content_width(yaml_icon(pe_ui_string(entry, "icon", "")),
                                                    pe_ui_string(entry, "title", item->label))
                             : 32;
        ui_tabs_set_width(&app.workspaces, item->id, width + app.tab_gap);
        app.workspaces.gap = (float)app.tab_gap;
        app.workspaces.items[i].enabled =
            !edit_pending() && (item->id == UI_MAP_VIEW || !app.playing);
    }
    if (ui_tabs_update(&app.ui, &app.workspaces, nav, 112))
        select_workspace(app.workspaces.selected);
    ui_clip_push(&app.ui, nav);
    ui_apply_clip(app.renderer, &app.ui);
    for (int i = 0; i < app.workspaces.count; i++) {
        const EditorView *item = editor_view(app.workspaces.items[i].id);
        const cJSON *entry = editor_entry(item->id);
        UiRect r = ui_tab_rect(&app.workspaces, nav, 112, i);
        bool hover = ui_hovered(&app.ui, r);
        ui_draw_button(app.renderer, (SDL_FRect){r.x, r.y, r.w, r.h},
                       yaml_icon(pe_ui_string(entry, "icon", "")),
                       labels ? pe_ui_string(entry, "title", item->label) : "",
                       app.workspaces.selected == item->id, app.workspaces.items[i].enabled, hover);
        if (hover)
            tooltip = pe_ui_string(entry, "title", item->label);
    }
    ui_clip_pop(&app.ui);
    ui_apply_clip(app.renderer, &app.ui);
    if (!app.active_layout || (!app.map && !app.script)) {
        text(LEFT + 16, TOP + 40, app.status, UI_TEXT);
        ui_end_frame(&app.ui);
        if (modal) {
            app.ui.blocked = false;
            app.click = saved_click;
            draw_modal();
        }
        SDL_RenderPresent(app.renderer);
        process_deferred_ui();
        return;
    }
    const EditorView *view = editor_view(app.workspaces.selected);
    float center_width = (float)(app.width - LEFT - RIGHT - 2 * PANEL_GAP);
    int rows = yaml_toolbar_rows(center_width);
    SDL_FRect tools = {LEFT + PANEL_GAP, TOP + PANEL_GAP, center_width, rows * 32.f + 6};
    app.viewport = (SDL_FRect){tools.x, tools.y + tools.h + PANEL_GAP, tools.w,
                               app.height - BOTTOM - PANEL_GAP - tools.y - tools.h - PANEL_GAP};
    box(tools, UI_PANEL);
    register_ui_hit((UiRect){tools.x, tools.y, tools.w, tools.h});
    ui_clip_push(&app.ui, (UiRect){tools.x, tools.y, tools.w, tools.h});
    ui_apply_clip(app.renderer, &app.ui);
    view->draw_tools(tools);
    ui_clip_pop(&app.ui);
    ui_apply_clip(app.renderer, &app.ui);
    const cJSON *columns = pe_ui_get(app.active_layout->root, "columns");
    draw_yaml_node(pe_ui_get(columns, "center"), app.viewport, 1);
    draw_yaml_node(pe_ui_get(columns, "left"),
                   (SDL_FRect){PANEL_GAP, TOP + PANEL_GAP, LEFT - 2 * PANEL_GAP,
                               app.height - TOP - BOTTOM - 2 * PANEL_GAP},
                   0);
    draw_yaml_node(pe_ui_get(columns, "right"),
                   (SDL_FRect){app.width - RIGHT + PANEL_GAP, TOP + PANEL_GAP,
                               RIGHT - 2 * PANEL_GAP, app.height - TOP - BOTTOM - 2 * PANEL_GAP},
                   2);
    box((SDL_FRect){0, (float)(app.height - BOTTOM), (float)app.width, BOTTOM}, UI_PANEL);
    box((SDL_FRect){0, (float)(app.height - BOTTOM), (float)app.width, 1}, UI_BORDER);
    float status_x = 14, status_y = (float)(app.height - 18);
    const cJSON *fields = pe_ui_get(pe_ui_get(app.shell_layout->root, "statusbar"), "fields");
    ui_clip_push(&app.ui, (UiRect){0, (float)(app.height - BOTTOM), (float)app.width, BOTTOM});
    ui_apply_clip(app.renderer, &app.ui);
    for (const cJSON *field = fields->child; field; field = field->next) {
        char buffer[256];
        const char *value = yaml_text(field, buffer, sizeof buffer);
        const char *binding = pe_ui_string(field, "binding", "");
        Uint32 color = !strcmp(binding, "editor.mode")      ? UI_ACCENT
                       : !strcmp(binding, "editor.message") ? UI_TEXT
                                                            : UI_MUTED;
        if (!strcmp(binding, "document.save_state") && !strcmp(value, "Modified"))
            color = UI_ACCENT;
        text(status_x, status_y, value, color);
        status_x += ui_text_width(value) + 16;
        if (field->next) {
            text(status_x, status_y, "|", UI_DISABLED);
            status_x += 16;
        }
    }
    ui_clip_pop(&app.ui);
    ui_apply_clip(app.renderer, &app.ui);
    if (tooltip && !modal) {
        float width = (float)strlen(tooltip) * 8 + 20;
        float tx = fminf(app.mx + 12, app.width - width - 8),
              ty = fminf(app.my + 30, app.height - 28.f);
        ui_panel(app.renderer, (SDL_FRect){tx, ty, width, 24}, UI_BORDER);
        text(tx + 10, ty + 8, tooltip, UI_TEXT);
    }
    UiRect ls = left_split(), rs = right_split();
    bool left_highlight = !modal && (ui_hovered(&app.ui, ls) || app.ui.active == UI_SPLIT_LEFT);
    bool right_highlight = !modal && (ui_hovered(&app.ui, rs) || app.ui.active == UI_SPLIT_RIGHT);
    draw_split_grip(ls, left_highlight);
    draw_split_grip(rs, right_highlight);
    bool resize = left_highlight || right_highlight;
    if (modal) {
        app.ui.blocked = false;
        app.click = saved_click;
        draw_modal();
    }
    app.ui.blocked = false;
    if (app.resize_cursor)
        SDL_SetCursor(resize ? app.resize_cursor : SDL_GetDefaultCursor());
    ui_end_frame(&app.ui);
    app.click = false;
    if (screenshot_path) {
        SDL_Surface *capture = SDL_RenderReadPixels(app.renderer, NULL);
        if (!capture || !SDL_SaveBMP(capture, screenshot_path))
            fprintf(stderr, "Screenshot failed: %s\n", SDL_GetError());
        SDL_DestroySurface(capture);
    }
    SDL_RenderPresent(app.renderer);
    process_deferred_ui();
}
static void keyboard(SDL_KeyboardEvent *key) {
    if (key->repeat)
        return;
    SDL_Keycode k = key->key;
    bool ctrl = (key->mod & (SDL_KMOD_CTRL | SDL_KMOD_GUI)) != 0,
         shift = (key->mod & SDL_KMOD_SHIFT) != 0;
    if (app.field != FIELD_NONE) {
        if (k == SDLK_ESCAPE)
            field_stop();
        else if (k == SDLK_RETURN)
            field_apply();
        else if (k == SDLK_BACKSPACE) {
            size_t n = strlen(app.input);
            if (n) {
                do {
                    n--;
                } while (n && ((unsigned char)app.input[n] & 0xc0) == 0x80);
                app.input[n] = 0;
            }
        } else if (ctrl && k == SDLK_A)
            app.input[0] = 0;
        return;
    }
    if (app.request != REQUEST_NONE) {
        if (k == SDLK_ESCAPE)
            app.request = REQUEST_NONE;
        return;
    }
    if (k == SDLK_ESCAPE) {
        if (edit_pending()) {
            pe_history_cancel(&app.map->history, &app.map->scene);
            app.painting = app.dragging = false;
        } else if (app.playing)
            play_toggle();
        return;
    }
    if (k == SDLK_F6) {
        reload_layout();
        return;
    }
    if (k == SDLK_F5) {
        if (map_active())
            play_toggle();
        return;
    }
    if (app.playing || edit_pending() || !app.loaded_view)
        return;
    if (ctrl && k == SDLK_S)
        save();
    else if (ctrl && (k == SDLK_Y || (k == SDLK_Z && shift))) {
        undo_active(true);
    } else if (ctrl && k == SDLK_Z) {
        undo_active(false);
    } else if (!ctrl && map_active()) {
        if (k == SDLK_B)
            app.tool = BRUSH;
        else if (k == SDLK_E)
            app.tool = ERASE;
        else if (k == SDLK_R)
            app.tool = RECTANGLE;
        else if (k == SDLK_F)
            app.tool = FILL;
        else if (k == SDLK_V)
            app.tool = SELECT;
        else if (k == SDLK_DELETE && app.selected && begin()) {
            pe_entity_destroy(&app.map->scene, pe_entity_find(&app.map->scene, app.selected));
            app.selected = 0;
            commit();
        }
    }
}
static void event(SDL_Event *e) {
    if (app.needs_layout)
        draw();
    bool modal = app.field != FIELD_NONE || app.request != REQUEST_NONE;
    if (e->type == SDL_EVENT_MOUSE_MOTION) {
        app.mx = e->motion.x;
        app.my = e->motion.y;
    } else if (e->type == SDL_EVENT_MOUSE_BUTTON_DOWN || e->type == SDL_EVENT_MOUSE_BUTTON_UP) {
        app.mx = e->button.x;
        app.my = e->button.y;
    }
    if (e->type == SDL_EVENT_MOUSE_MOTION)
        ui_pointer(&app.ui, app.mx, app.my, app.ui.down);
    if ((e->type == SDL_EVENT_MOUSE_BUTTON_DOWN || e->type == SDL_EVENT_MOUSE_BUTTON_UP) &&
        e->button.button == SDL_BUTTON_LEFT)
        ui_pointer(&app.ui, app.mx, app.my, e->type == SDL_EVENT_MOUSE_BUTTON_DOWN);
    bool captured = app.ui.active == UI_SPLIT_LEFT || app.ui.active == UI_SPLIT_RIGHT ||
                    app.ui.active == UI_TREE_SCROLL || app.ui.active == UI_INSPECTOR_SCROLL ||
                    app.ui.active == UI_SCENE_SCROLL || app.ui.active == UI_INFO_SCROLL ||
                    app.ui.active == UI_SCRIPT_VSCROLL || app.ui.active == UI_SCRIPT_HSCROLL;
    if (!modal && (e->type == SDL_EVENT_MOUSE_MOTION || e->type == SDL_EVENT_MOUSE_BUTTON_UP) &&
        captured)
        return;
    if (!modal && e->type == SDL_EVENT_MOUSE_BUTTON_DOWN && e->button.button == SDL_BUTTON_LEFT &&
        ui_controls_hit(app.mx, app.my)) {
        app.click = true;
        if (app.script)
            se_focus(app.script, app.window, false);
        return;
    }
    if (e->type == SDL_EVENT_KEY_DOWN && !e->key.repeat &&
        (e->key.key == SDLK_RETURN || e->key.key == SDLK_SPACE) && app.field == FIELD_NONE &&
        !(scripts_active() && (*app.script).focused))
        ui_activate(&app.ui);
    if (scripts_active() && app.canvas_enabled && !modal &&
        se_event(&(*app.script), app.window, e, app.viewport)) {
        if (e->type == SDL_EVENT_MOUSE_BUTTON_DOWN && (*app.script).focused)
            ui_focus(&app.ui, UI_SCRIPT_DOCUMENT);
        return;
    }
    switch (e->type) {
    case SDL_EVENT_QUIT:
        if (app.playing)
            play_toggle();
        if (edit_pending()) {
            pe_history_cancel(&app.map->history, &app.map->scene);
            app.painting = app.dragging = false;
        }
        request(REQUEST_QUIT, NULL);
        break;
    case SDL_EVENT_KEY_DOWN:
        keyboard(&e->key);
        break;
    case SDL_EVENT_TEXT_INPUT:
        if (app.field != FIELD_NONE && strlen(app.input) + strlen(e->text.text) < sizeof app.input)
            strcat(app.input, e->text.text);
        break;
    case SDL_EVENT_DROP_FILE:
        if (!modal && !edit_pending()) {
            if (suffix(e->drop.data, ".js"))
                request(REQUEST_SCRIPT_OPEN, e->drop.data);
            else if (map_active())
                import_png(e->drop.data);
        }
        break;
    case SDL_EVENT_WINDOW_FOCUS_LOST:
        ui_cancel(&app.ui);
        if (edit_pending()) {
            pe_history_cancel(&app.map->history, &app.map->scene);
            app.painting = app.dragging = false;
        }
        app.panning = false;
        break;
    case SDL_EVENT_MOUSE_WHEEL:
        app.wheel_delta = e->wheel.y;
        if (!modal && !edit_pending()) {
            if (app.my < TOP)
                ui_tabs_scroll(&app.workspaces, (float)(app.width - GLOBAL_NAV_X - 14), 112,
                               -e->wheel.y * 56);
            else if (inside(app.mx, app.my, app.viewport) && app.canvas_enabled && map_active())
                pe_camera_zoom(&app.camera, app.mx - app.viewport.x, app.my - app.viewport.y,
                               powf(1.15f, e->wheel.y));
            else if (app.mx < LEFT && app.my >= TOP && app.my < SIDE_TOP)
                ui_tabs_scroll(&app.left_tabs, LEFT - 2 * PANEL_GAP, 112, -e->wheel.y * 56);
            else if (app.mx > app.width - RIGHT && app.my >= TOP && app.my < SIDE_TOP)
                ui_tabs_scroll(&app.right_tabs, RIGHT - 2 * PANEL_GAP, 112, -e->wheel.y * 56);
            else if (app.mx < LEFT)
                ui_scroll_by(app.left_tabs.selected == UI_SCENE_TAB ? &app.scene_view
                                                                    : &app.tree_view,
                             -e->wheel.y * 69);
            else if (app.mx > app.width - RIGHT)
                ui_scroll_by(app.right_tabs.selected == UI_INFO_TAB ? &app.info_view
                                                                    : &app.inspector_view,
                             -e->wheel.y * 60);
        }
        break;
    case SDL_EVENT_MOUSE_BUTTON_DOWN:
        app.mx = e->button.x;
        app.my = e->button.y;
        if (e->button.button == SDL_BUTTON_LEFT) {
            if (modal)
                app.click = true;
            else if (inside(app.mx, app.my, app.viewport) && app.canvas_enabled && !app.playing &&
                     map_active())
                edit_start(app.mx, app.my, false);
            else if (!edit_pending())
                app.click = true;
        } else if (!modal && inside(app.mx, app.my, app.viewport) && app.canvas_enabled &&
                   map_active()) {
            if (e->button.button == SDL_BUTTON_MIDDLE)
                app.panning = true;
            else if (e->button.button == SDL_BUTTON_RIGHT && (app.map || app.script) &&
                     !app.playing && !edit_pending())
                edit_start(app.mx, app.my, true);
        }
        break;
    case SDL_EVENT_MOUSE_BUTTON_UP:
        app.mx = e->button.x;
        app.my = e->button.y;
        if (e->button.button == SDL_BUTTON_MIDDLE)
            app.panning = false;
        else if (e->button.button == SDL_BUTTON_LEFT || e->button.button == SDL_BUTTON_RIGHT)
            edit_end(e->button.button == SDL_BUTTON_RIGHT || app.tool == ERASE);
        break;
    case SDL_EVENT_MOUSE_MOTION:
        app.mx = e->motion.x;
        app.my = e->motion.y;
        if (app.panning) {
            app.camera.x -= e->motion.xrel / app.camera.zoom;
            app.camera.y -= e->motion.yrel / app.camera.zoom;
        }
        if (app.painting && app.tool != RECTANGLE && inside(app.mx, app.my, app.viewport)) {
            int x, y;
            tile_coords(app.mx, app.my, &x, &y);
            paint_line(app.last_x, app.last_y, x, y,
                       app.tool == ERASE || (e->motion.state & SDL_BUTTON_RMASK));
            app.last_x = x;
            app.last_y = y;
        }
        if (app.dragging) {
            PeObject *o =
                pe_entity_get(&app.map->scene, pe_entity_find(&app.map->scene, app.selected));
            if (o) {
                float x, y;
                pe_screen_to_world(&app.camera, app.mx - app.viewport.x, app.my - app.viewport.y,
                                   &x, &y);
                o->x = x - app.drag_x;
                o->y = y - app.drag_y;
            }
        }
        break;
    default:
        break;
    }
}

/* Headless integration check: use the same handlers as real mouse/keyboard input. */
static bool verify_editor(void) {
    draw();
    PeScene baseline = app.map->scene;
    app.tile = 4;
    app.tool = BRUSH;
    float sx = app.viewport.x + (2 * app.map->scene.tile_size + 2 - app.camera.x) * app.camera.zoom;
    float sy = app.viewport.y + (2 * app.map->scene.tile_size + 2 - app.camera.y) * app.camera.zoom;
    SDL_Event e = {0};
    e.type = SDL_EVENT_MOUSE_BUTTON_DOWN;
    e.button.button = SDL_BUTTON_LEFT;
    e.button.x = sx;
    e.button.y = sy;
    event(&e);
    e.type = SDL_EVENT_MOUSE_MOTION;
    e.motion.x = sx + app.map->scene.tile_size * app.camera.zoom;
    e.motion.y = sy;
    e.motion.state = SDL_BUTTON_LMASK;
    event(&e);
    e.type = SDL_EVENT_MOUSE_BUTTON_UP;
    e.button.button = SDL_BUTTON_LEFT;
    e.button.x = sx + app.map->scene.tile_size * app.camera.zoom;
    e.button.y = sy;
    event(&e);
    bool ok = app.map->history.count == 1 && pe_tile_get(&app.map->scene, 0, 2, 2) == 4 &&
              pe_tile_get(&app.map->scene, 0, 3, 2) == 4;
    e.type = SDL_EVENT_KEY_DOWN;
    e.key.key = SDLK_Z;
    e.key.mod = SDL_KMOD_CTRL;
    event(&e);
    ok = ok && !memcmp(&app.map->scene, &baseline, sizeof baseline);
    e.key.mod = SDL_KMOD_CTRL | SDL_KMOD_SHIFT;
    event(&e);
    ok = ok && pe_tile_get(&app.map->scene, 0, 3, 2) == 4;
    PeScene edited = app.map->scene;
    e.key.key = SDLK_F5;
    e.key.mod = 0;
    event(&e);
    pe_update(&app.map->runtime, 1, 0, 1.f / 60);
    ok = ok && app.playing && !memcmp(&app.map->scene, &edited, sizeof edited);
    event(&e);
    ok = ok && !app.playing;
    app.map->scene = baseline;
    pe_history_free(&app.map->history);
    app.tile = 1;
    if (!ok)
        fprintf(stderr, "Editor interaction smoke test failed\n");
    return ok;
}

static bool verify_script_editor(void) {
    PeScene scene_before = app.map->scene;
    tab_select(true);
    if (!app.script || app.map)
        return false;
    PeText original = (*app.script).document;
    pe_text_init(&(*app.script).document, "");
    tab_select(true);
    SDL_Event e = {0};
    e.type = SDL_EVENT_TEXT_INPUT;
    e.text.text = "cons";
    event(&e);
    memset(&e, 0, sizeof e);
    e.type = SDL_EVENT_KEY_DOWN;
    e.key.key = SDLK_SPACE;
    e.key.mod = SDL_KMOD_CTRL;
    event(&e);
    bool ok = (*app.script).show_completion;
    draw();
    ok = ok && (*app.script).popup.w > 0;
    e.key.key = SDLK_RETURN;
    e.key.mod = 0;
    event(&e);
    ok = ok && !strcmp((*app.script).document.text, "const");
    e.key.key = SDLK_Z;
    e.key.mod = SDL_KMOD_CTRL;
    event(&e);
    ok = ok && !strcmp((*app.script).document.text, "cons");
    e.key.mod = SDL_KMOD_CTRL | SDL_KMOD_SHIFT;
    event(&e);
    ok = ok && !strcmp((*app.script).document.text, "const");
    e.key.key = SDLK_BACKSPACE;
    e.key.mod = 0;
    event(&e);
    draw();
    memset(&e, 0, sizeof e);
    e.type = SDL_EVENT_MOUSE_BUTTON_DOWN;
    e.button.button = SDL_BUTTON_LEFT;
    e.button.x = (*app.script).popup.x + 8;
    e.button.y = (*app.script).popup.y + 8;
    event(&e);
    ok = ok && !strcmp((*app.script).document.text, "const");
    e.type = SDL_EVENT_MOUSE_BUTTON_UP;
    event(&e);
    memset(&e, 0, sizeof e);
    e.type = SDL_EVENT_TEXT_INPUT;
    e.text.text = " value = ;\n";
    event(&e);
    memset(&e, 0, sizeof e);
    e.type = SDL_EVENT_KEY_DOWN;
    e.key.key = SDLK_F7;
    event(&e);
    ok = ok && !(*app.script).diagnostic.valid && (*app.script).diagnostic.line == 1;
    request(REQUEST_QUIT, NULL);
    ok = ok && app.request == REQUEST_QUIT && app.running;
    app.request = REQUEST_NONE;
    se_free(&(*app.script));
    (*app.script).document = original;
    (*app.script).highlighted_revision = UINT64_MAX;
    (*app.script).checked_revision = UINT64_MAX;
    (*app.script).show_completion = false;
    (*app.script).top_line = (*app.script).left_column = 0;
    se_check(&(*app.script));
    tab_select(false);
    ok = ok && app.map && !app.script &&
         !memcmp(&scene_before, &app.map->scene, sizeof scene_before);
    if (!ok)
        fprintf(stderr, "Script editor interaction smoke test failed\n");
    return ok;
}

static void smoke_pointer(Uint32 type, float x, float y) {
    SDL_Event e = {0};
    e.type = type;
    if (type == SDL_EVENT_MOUSE_MOTION) {
        e.motion.x = x;
        e.motion.y = y;
        e.motion.state = SDL_BUTTON_LMASK;
    } else {
        e.button.button = SDL_BUTTON_LEFT;
        e.button.x = x;
        e.button.y = y;
    }
    event(&e);
    draw();
}
static bool verify_ui_layout(void) {
    PeScene original = app.map->scene;
    tab_select(false);
    ui_cancel(&app.ui);
    draw();
    float x = app.left_width, y = TOP + TABS + 90;
    smoke_pointer(SDL_EVENT_MOUSE_BUTTON_DOWN, x, y);
    smoke_pointer(SDL_EVENT_MOUSE_MOTION, x + 100, y);
    smoke_pointer(SDL_EVENT_MOUSE_BUTTON_UP, x + 100, y);
    bool ok = fabsf(app.left_width - 340) < .01f && app.viewport.x == 340 + PANEL_GAP;
    x = app.width - app.right_width;
    smoke_pointer(SDL_EVENT_MOUSE_BUTTON_DOWN, x, y);
    smoke_pointer(SDL_EVENT_MOUSE_MOTION, x - 80, y);
    smoke_pointer(SDL_EVENT_MOUSE_BUTTON_UP, x - 80, y);
    ok = ok && fabsf(app.right_width - 360) < .01f;
    float header_x = app.width - app.right_width + 30, header_y = SIDE_TOP + 20;
    smoke_pointer(SDL_EVENT_MOUSE_BUTTON_DOWN, header_x, header_y);
    smoke_pointer(SDL_EVENT_MOUSE_BUTTON_UP, header_x, header_y);
    ok = ok && !app.layers_open;
    float tab_x = GLOBAL_NAV_X + 16, tab_y = 24;
    smoke_pointer(SDL_EVENT_MOUSE_BUTTON_DOWN, tab_x, tab_y);
    smoke_pointer(SDL_EVENT_MOUSE_MOTION, tab_x + 60, tab_y);
    smoke_pointer(SDL_EVENT_MOUSE_BUTTON_UP, tab_x + 60, tab_y);
    ok = ok && app.workspaces.items[0].id == UI_SCRIPT_VIEW &&
         app.workspaces.items[1].id == UI_MAP_VIEW && map_active();
    /* Sidebar navigation is independent of workspace tabs and scene history. */
    float side_y = TOP + PANEL_GAP + 12;
    float scene_tab_x = PANEL_GAP + app.left_tabs.items[0].width + 20;
    float info_tab_x = app.width - RIGHT + PANEL_GAP + app.right_tabs.items[0].width + 20;
    smoke_pointer(SDL_EVENT_MOUSE_BUTTON_DOWN, scene_tab_x, side_y);
    smoke_pointer(SDL_EVENT_MOUSE_BUTTON_UP, scene_tab_x, side_y);
    smoke_pointer(SDL_EVENT_MOUSE_BUTTON_DOWN, info_tab_x, side_y);
    smoke_pointer(SDL_EVENT_MOUSE_BUTTON_UP, info_tab_x, side_y);
    ok = ok && app.left_tabs.selected == UI_SCENE_TAB && app.right_tabs.selected == UI_INFO_TAB &&
         map_active();
    uint32_t saved_selected = app.selected;
    int saved_tool = app.tool;
    for (int i = 0; i < PE_ENTITIES; i++) {
        if (!app.map->scene.objects[i].alive)
            continue;
        smoke_pointer(SDL_EVENT_MOUSE_BUTTON_DOWN, 60, SIDE_TOP + 58);
        smoke_pointer(SDL_EVENT_MOUSE_BUTTON_UP, 60, SIDE_TOP + 58);
        ok = ok && app.selected == app.map->scene.objects[i].stable_id &&
             app.right_tabs.selected == UI_INSPECTOR_TAB;
        break;
    }
    app.selected = saved_selected;
    app.tool = saved_tool;
    app.left_tabs.selected = UI_EXPLORER_TAB;
    app.right_tabs.selected = UI_INSPECTOR_TAB;
    tab_select(true);
    PeText saved = (*app.script).document;
    char lines[4096] = "";
    for (int i = 0; i < 100; i++)
        strcat(lines, "// scrollable line\n");
    pe_text_init(&(*app.script).document, lines);
    (*app.script).extent_revision = (*app.script).highlighted_revision =
        (*app.script).checked_revision = UINT64_MAX;
    (*app.script).top_line = 0;
    tab_select(true);
    draw();
    float scroll_x = app.viewport.x + app.viewport.w - 8, scroll_y = (*app.script).body.y + 8;
    smoke_pointer(SDL_EVENT_MOUSE_BUTTON_DOWN, scroll_x, scroll_y);
    smoke_pointer(SDL_EVENT_MOUSE_MOTION, scroll_x, scroll_y + 120);
    smoke_pointer(SDL_EVENT_MOUSE_BUTTON_UP, scroll_x, scroll_y + 120);
    ok = ok && (*app.script).top_line > 0 && !strcmp((*app.script).document.text, lines) &&
         (*app.script).document.count == 0;
    se_free(&(*app.script));
    (*app.script).document = saved;
    (*app.script).extent_revision = (*app.script).highlighted_revision =
        (*app.script).checked_revision = UINT64_MAX;
    (*app.script).top_line = (*app.script).left_column = 0;
    se_check(&(*app.script));
    tab_select(false);
    ok = ok && !memcmp(&original, &app.map->scene, sizeof original);
    app.left_width = 240;
    app.right_width = 280;
    app.layers_open = app.entity_open = app.palette_open = true;
    app.tree_view.offset = app.inspector_view.offset = 0;
    UiTab tabs[VIEW_COUNT];
    for (int i = 0; i < VIEW_COUNT; i++)
        tabs[i] = (UiTab){editor_views[i].id, true, 0};
    ui_tabs_init(&app.workspaces, tabs, VIEW_COUNT, UI_MAP_VIEW);
    ui_cancel(&app.ui);
    tab_select(false);
    draw();
    if (!ok)
        fprintf(stderr, "Shared UI layout / capture smoke test failed\n");
    return ok;
}
static void smoke_click(float x, float y) {
    smoke_pointer(SDL_EVENT_MOUSE_BUTTON_DOWN, x, y);
    smoke_pointer(SDL_EVENT_MOUSE_BUTTON_UP, x, y);
}
static bool verify_workspace_tools(void) {
    PeScene original = app.map->scene;
    int history_cursor = app.map->history.cursor;
    uint64_t revision = 0;
    bool grid = app.grid;
    PeCamera camera = app.camera;
    tab_select(false);
    draw();
    float action_y = TOP + PANEL_GAP + 18;
    smoke_click(30, 24);
    bool menu_unchanged = map_active() && app.request == REQUEST_NONE && app.field == FIELD_NONE;
    smoke_click(GLOBAL_NAV_X + app.workspaces.items[0].width + 16, 24);
    bool ok = menu_unchanged && scripts_active() && !app.map;
    if (!app.script)
        return false;
    revision = (*app.script).document.revision;
    (*app.script).checked_revision = UINT64_MAX;
    smoke_click(LEFT + PANEL_GAP + 8 + 16, action_y);
    ok = ok && scripts_active() && (*app.script).checked_revision == revision;
    /* Global Undo/Redo must operate on the active script, never the scene. */
    PeText saved_document = (*app.script).document;
    pe_text_init(&(*app.script).document, "// toolbar test");
    pe_text_insert(&(*app.script).document, "// edited\n");
    se_changed(&(*app.script), app.viewport, false);
    draw();
    smoke_click(150, 24);
    ok = ok && (*app.script).document.position == 0 &&
         !strcmp((*app.script).document.text, "// toolbar test");
    smoke_click(188, 24);
    ok = ok && (*app.script).document.position == 1 &&
         strstr((*app.script).document.text, "edited") != NULL;
    pe_text_free(&(*app.script).document);
    (*app.script).document = saved_document;
    (*app.script).extent_revision = (*app.script).highlighted_revision =
        (*app.script).checked_revision = UINT64_MAX;
    se_check(&(*app.script));
    SDL_Event f5 = {0};
    f5.type = SDL_EVENT_KEY_DOWN;
    f5.key.key = SDLK_F5;
    event(&f5);
    ok = ok && !app.playing;
    bool document_unchanged = (*app.script).document.revision == revision;
    smoke_click(GLOBAL_NAV_X + 16, 24);
    ok = ok && map_active() && !app.script && document_unchanged;
    /* Narrow the center until the last map actions wrap to a second row. */
    app.left_width = 400;
    app.right_width = app.width - app.left_width - app.min_center;
    draw();
    ok = ok && app.viewport.y == TOP + 2 * PANEL_GAP + 70;
    int columns = (int)((app.viewport.w - 10) / 38);
    int grid_index = editor_view(UI_MAP_VIEW)->action_count - 1;
    smoke_click(LEFT + PANEL_GAP + 8 + (grid_index % columns) * 38 + 16,
                action_y + (grid_index / columns) * 32);
    ok = ok && app.grid != grid && !memcmp(&original, &app.map->scene, sizeof original) &&
         app.map->history.cursor == history_cursor && !memcmp(&camera, &app.camera, sizeof camera);
    app.grid = grid;
    app.left_width = 240;
    app.right_width = 280;
    ui_cancel(&app.ui);
    tab_select(false);
    draw();
    if (!ok)
        fprintf(stderr, "Workspace navigation / contextual toolbar smoke test failed\n");
    return ok;
}
static bool verify_lifecycle(void) {
    char error[256], map_backup[1300], script_backup[1300], original_ui[1024], test_ui[1024];
    snprintf(map_backup, sizeof map_backup, "%s.baseline", app.session_paths[0]);
    snprintf(script_backup, sizeof script_backup, "%s.baseline", app.session_paths[1]);
    snprintf(original_ui, sizeof original_ui, "%s", app.ui_path);
    if (strlen(app.ui_path) + 7 >= sizeof test_ui)
        return false;
    memcpy(test_ui, app.ui_path, strlen(app.ui_path));
    strcpy(test_ui + strlen(app.ui_path), ".smoke");
    UiId id = UI_MAP_VIEW;
    tab_select(false);
    draw();
    if (!app.map || !pe_session_save(map_backup, id, editor_codec, &id, error, sizeof error))
        return false;
    bool ok = begin();
    if (ok) {
        pe_tile_set(&app.map->scene, 0, 1, 1, 7);
        commit();
    }
    app.camera = (PeCamera){11, -7, 2.5f};
    PeScene edited = app.map->scene;
    uint64_t map_revision = app.map->history.revision;
    char original_cache[1200];
    snprintf(original_cache, sizeof original_cache, "%s", app.session_paths[0]);
    snprintf(app.session_paths[0], sizeof app.session_paths[0],
             "missing-session-directory/map.cache");
    select_workspace(UI_SCRIPT_VIEW);
    ok = ok && map_active() && app.map && !app.script && app.map->history.revision == map_revision;
    snprintf(app.session_paths[0], sizeof app.session_paths[0], "%s", original_cache);
    select_workspace(UI_SCRIPT_VIEW);
    draw();
    if (!app.script || app.map)
        return false;
    ok = ok && app.session_dirty[0];
    id = UI_SCRIPT_VIEW;
    if (!pe_session_save(script_backup, id, editor_codec, &id, error, sizeof error))
        return false;
    pe_text_insert(&app.script->document, "// unsaved lifecycle test\n");
    se_changed(app.script, app.viewport, false);
    uint64_t script_revision = app.script->document.revision;
    int script_position = app.script->document.position;
    select_workspace(UI_MAP_VIEW);
    draw();
    if (!app.map || app.script)
        return false;
    ok = ok && app.session_dirty[1] && !memcmp(&edited, &app.map->scene, sizeof edited) &&
         app.map->history.revision == map_revision && app.camera.x == 11 && app.camera.zoom == 2.5f;
    undo_active(false);
    ok = ok && pe_tile_get(&app.map->scene, 0, 1, 1) != 7;
    undo_active(true);
    ok = ok && pe_tile_get(&app.map->scene, 0, 1, 1) == 7;
    /* Corrupt a target checkpoint: the source must be restored, never discarded. */
    FILE *f = fopen(app.session_paths[1], "r+b");
    if (!f)
        return false;
    fseek(f, -1, SEEK_END);
    int byte = fgetc(f);
    fseek(f, -1, SEEK_END);
    fputc(byte ^ 255, f);
    fclose(f);
    select_workspace(UI_SCRIPT_VIEW);
    ok = ok && map_active() && app.map && !app.script && app.map->history.revision == map_revision;
    f = fopen(app.session_paths[1], "r+b");
    if (!f)
        return false;
    fseek(f, -1, SEEK_END);
    fputc(byte, f);
    fclose(f);
    select_workspace(UI_SCRIPT_VIEW);
    draw();
    if (!app.script || app.map)
        return false;
    ok = ok && app.script->document.revision == script_revision &&
         app.script->document.position == script_position;
    undo_active(false);
    ok = ok && app.script->document.position == script_position - 1;
    undo_active(true);
    ok = ok && app.script->document.position == script_position;
    request(REQUEST_QUIT, NULL);
    ok = ok && app.request == REQUEST_QUIT && app.running;
    app.request = REQUEST_NONE;
    select_workspace(UI_MAP_VIEW);
    draw();
    /* A valid reload changes layout/theme and reinstantiates the same dirty document. */
    cJSON *root = cJSON_Duplicate(app.shell_layout->root, true);
    cJSON_ReplaceItemInObjectCaseSensitive((cJSON *)pe_ui_get(root, "theme"), "accent",
                                           cJSON_CreateString("#B08968"));
    cJSON_ReplaceItemInObjectCaseSensitive((cJSON *)pe_ui_get(root, "layout"), "gap",
                                           cJSON_CreateNumber(2));
    char *json = cJSON_PrintUnformatted(root);
    cJSON_Delete(root);
    f = fopen(test_ui, "wb");
    if (!f) {
        free(json);
        return false;
    }
    fwrite(json, 1, strlen(json), f);
    fclose(f);
    free(json);
    snprintf(app.ui_path, sizeof app.ui_path, "%s", test_ui);
    ok = reload_layout() && ok;
    ok = ok && UI_ACCENT == 0xb08968ffu && app.gap == 2 && app.map &&
         app.map->history.revision == map_revision;
    MapPayload *before = app.map;
    f = fopen(test_ui, "wb");
    if (!f)
        return false;
    fputs("broken: [", f);
    fclose(f);
    ok = ok && !reload_layout() && app.map == before && UI_ACCENT == 0xb08968ffu;
    snprintf(app.ui_path, sizeof app.ui_path, "%s", original_ui);
    ok = reload_layout() && ok;
    remove(test_ui);
    /* Restore the test baseline without writing any project files. */
    select_workspace(UI_SCRIPT_VIEW);
    if (!app.script)
        return false;
    se_free(app.script);
    memset(app.script, 0, sizeof *app.script);
    id = UI_SCRIPT_VIEW;
    ok = pe_session_load(script_backup, id, editor_codec, &id, error, sizeof error) && ok;
    se_check(app.script);
    select_workspace(UI_MAP_VIEW);
    if (!app.map)
        return false;
    pe_assets_free(&app.map->assets);
    pe_history_free(&app.map->history);
    memset(app.map, 0, sizeof *app.map);
    id = UI_MAP_VIEW;
    ok = pe_session_load(map_backup, id, editor_codec, &id, error, sizeof error) && ok;
    assets_sync();
    remove(map_backup);
    remove(script_backup);
    app.session_dirty[0] = map_dirty();
    app.mx = app.my = 0;
    ui_cancel(&app.ui);
    draw();
    if (!ok)
        fprintf(stderr, "Editor lifecycle / checkpoint / YAML reload smoke failed\n");
    return ok;
}
int main(int argc, char **argv) {
    const char *root = NULL;
    for (int i = 1; i < argc; i++) {
        if (!strcmp(argv[i], "--project") && i + 1 < argc)
            root = argv[++i];
        else if (!strcmp(argv[i], "--screenshot") && i + 1 < argc)
            screenshot_path = argv[++i];
        else if (!strcmp(argv[i], "--ui") && i + 1 < argc)
            snprintf(app.ui_path, sizeof app.ui_path, "%s", argv[++i]);
        else if (!strcmp(argv[i], "--scripts"))
            start_scripts = true;
        else if (!strcmp(argv[i], "--smoke"))
            app.smoke = true;
        else {
            fprintf(stderr, "Usage: penguin_editor [--project path] [--smoke] [--scripts] "
                            "[--ui shell.yaml] [--screenshot file.bmp]\n");
            return 1;
        }
    }
    char error[256];
    if (!pe_project_resolve(root, SDL_GetBasePath(), app.root, sizeof app.root, error,
                            sizeof error)) {
        fprintf(stderr, "%s\n", error);
        return 1;
    }
    root = app.root;
    if (!pe_project_load(root, app.scene_path, sizeof app.scene_path, error, sizeof error)) {
        fprintf(stderr, "%s\n", error);
        return 1;
    }
    if (!SDL_Init(SDL_INIT_VIDEO)) {
        fprintf(stderr, "%s\n", SDL_GetError());
        return 1;
    }
    app.window = SDL_CreateWindow("PenguinEngine - Editor", 1280, 800, SDL_WINDOW_RESIZABLE);
    app.renderer = app.window ? SDL_CreateRenderer(app.window, NULL) : NULL;
    if (!app.renderer) {
        fprintf(stderr, "%s\n", SDL_GetError());
        SDL_DestroyWindow(app.window);
        SDL_Quit();
        return 1;
    }
    SDL_SetWindowMinimumSize(app.window, 1100, 740);
    SDL_SetRenderVSync(app.renderer, 1);
    ui_init(&app.ui);
    if (!*app.ui_path) {
        snprintf(app.ui_path, sizeof app.ui_path, "%sui-layouts/shell.yaml", SDL_GetBasePath());
        FILE *check = fopen(app.ui_path, "rb");
        if (check)
            fclose(check);
        else
            snprintf(app.ui_path, sizeof app.ui_path, "%s../ui-layouts/shell.yaml",
                     SDL_GetBasePath());
    }
    app.shell_layout = pe_ui_layout_load(app.ui_path, "shell", error, sizeof error);
    if (!app.shell_layout) {
        fprintf(stderr, "%s\n", error);
        return 1;
    }
    app.gap = 3;
    app.tab_gap = 2;
    app.min_left = 192;
    app.min_right = 264;
    app.min_center = 320;
    apply_shell_layout();
    app.left_width = 240;
    app.right_width = 280;
    app.layers_open = app.entity_open = app.palette_open = true;
    apply_shell_layout();
    UiTab left_tabs[] = {{UI_EXPLORER_TAB, true, 0}, {UI_SCENE_TAB, true, 0}};
    UiTab right_tabs[] = {{UI_INSPECTOR_TAB, true, 0}, {UI_INFO_TAB, true, 0}};
    ui_tabs_init(&app.left_tabs, left_tabs, 2, UI_EXPLORER_TAB);
    ui_tabs_init(&app.right_tabs, right_tabs, 2, UI_INSPECTOR_TAB);
    app.resize_cursor = SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_EW_RESIZE);
    app.width = 1280;
    app.height = 800;
    app.viewport = (SDL_FRect){LEFT, TOP + TABS, 1280 - LEFT - RIGHT, 800 - TOP - TABS - BOTTOM};
    char *pref = app.smoke ? SDL_strdup(SDL_GetBasePath())
                           : SDL_GetPrefPath("PenguinEngine", "editor-sessions");
    if (!pref) {
        fprintf(stderr, "Cannot create session directory: %s\n", SDL_GetError());
        return 1;
    }
    for (int i = 0; i < 2; i++)
        snprintf(app.session_paths[i], sizeof app.session_paths[i], "%s%08x-%llx-%d.session", pref,
                 ui_id(app.root), (unsigned long long)SDL_GetPerformanceCounter(), i);
    SDL_free(pref);
    app.camera = (PeCamera){-16, -16, 1.4f};
    app.grid = true;
    app.tile = 1;
    app.running = true;
    app.last_time = SDL_GetTicksNS();
    strcpy(app.expanded[0], "assets");
    strcpy(app.expanded[1], "scenes");
    strcpy(app.expanded[2], "scripts");
    app.expanded_count = 3;
    tree_refresh();
    message("Welcome! Select a tile, paint a map, and press F5 to play.");
    if (!activate_editor(app.workspaces.selected, error, sizeof error)) {
        fprintf(stderr, "%s\n", error);
        return 1;
    }
    int exit_code = 0;
    if (app.smoke && (!verify_editor() || !verify_script_editor() || !verify_ui_layout() ||
                      !verify_workspace_tools() || !verify_lifecycle())) {
        app.running = false;
        exit_code = 1;
    }
    if (start_scripts)
        tab_select(true);
    int frames = 0;
    while (app.running) {
        SDL_Event e;
        while (SDL_PollEvent(&e))
            event(&e);
        Uint64 now = SDL_GetTicksNS();
        double elapsed = (now - app.last_time) / 1e9;
        app.last_time = now;
        if (elapsed > .1)
            elapsed = .1;
        if (app.playing) {
            app.accumulator += elapsed;
            const bool *keys = SDL_GetKeyboardState(NULL);
            float x = (keys[SDL_SCANCODE_D] || keys[SDL_SCANCODE_RIGHT]) -
                      (keys[SDL_SCANCODE_A] || keys[SDL_SCANCODE_LEFT]),
                  y = (keys[SDL_SCANCODE_S] || keys[SDL_SCANCODE_DOWN]) -
                      (keys[SDL_SCANCODE_W] || keys[SDL_SCANCODE_UP]);
            while (app.accumulator >= 1.0 / 60) {
                pe_update(&app.map->runtime, x, y, 1.f / 60);
                app.accumulator -= 1.0 / 60;
            }
        }
        draw();
        if (app.smoke && ++frames >= 3)
            app.running = false;
        SDL_Delay(1);
    }
    pe_ui_layout_free(app.active_layout);
    app.active_layout = NULL;
    pe_ui_layout_free(app.shell_layout);
    if (app.script) {
        se_free(app.script);
        free(app.script);
    }
    if (app.map) {
        pe_history_free(&app.map->history);
        pe_assets_free(&app.map->assets);
        free(app.map);
    }
    for (int i = 0; i < 2; i++)
        remove(app.session_paths[i]);
    SDL_DestroyCursor(app.resize_cursor);
    SDL_DestroyRenderer(app.renderer);
    SDL_DestroyWindow(app.window);
    SDL_Quit();
    return exit_code;
}
