#include "penguin/render.h"
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
#define PANEL_GAP 3
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
typedef enum { REQUEST_NONE, REQUEST_QUIT, REQUEST_OPEN, REQUEST_SCRIPT_OPEN } Request;
typedef struct {
    char path[PE_PATH];
    char name[128];
    int depth;
    bool directory, expanded;
} TreeRow;
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
    PeScene scene, runtime;
    PeScriptEditor script;
    PeHistory history;
    PeAssets assets;
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
    return app.workspaces.selected == UI_MAP_VIEW;
}
static bool scripts_active(void) {
    return app.workspaces.selected == UI_SCRIPT_VIEW;
}
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
static bool icon_button_impl(UiId callsite, float x, float y, float w, UiIcon icon,
                             const char *label, const char *hint, bool active, bool enabled) {
    SDL_FRect r = {x, y, w, 25};
    UiRect hit = {x, y, w, 25};
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
    snprintf(app.status, sizeof app.status, "%s", s);
}
static bool begin(void) {
    if (!pe_history_begin(&app.history, &app.scene)) {
        message("Cannot begin edit (pending edit or out of memory)");
        return false;
    }
    return true;
}
static void commit(void) {
    if (!pe_history_commit(&app.history, &app.scene))
        message("Out of memory: edit reverted");
}
static void assets_sync(void) {
    char error[256];
    if (!pe_assets_load(&app.assets, app.renderer, app.root, &app.scene, error, sizeof error))
        message(error);
}
static void selection_sync(void) {
    if (app.layer >= app.scene.layer_count)
        app.layer = app.scene.layer_count - 1;
    if (!pe_entity_valid(&app.scene, pe_entity_find(&app.scene, app.selected)))
        app.selected = 0;
    assets_sync();
}
static bool save_scene(void) {
    char error[256];
    if (!pe_scene_save(&app.scene, app.scene_path, error, sizeof error)) {
        message(error);
        return false;
    }
    pe_history_saved(&app.history);
    message("Scene saved");
    return true;
}
static bool save(void) {
    if (scripts_active()) {
        bool ok = se_save(&app.script);
        message(app.script.status);
        return ok;
    }
    return save_scene();
}
static void select_workspace(UiId id) {
    if (app.history.pending || (app.playing && id != UI_MAP_VIEW))
        return;
    app.workspaces.selected = editor_view(id)->id;
    ui_focus(&app.ui, scripts_active() ? UI_SCRIPT_DOCUMENT : 0);
    se_focus(&app.script, app.window, scripts_active() && !app.playing);
}
static void tab_select(bool scripts) {
    select_workspace(scripts ? UI_SCRIPT_VIEW : UI_MAP_VIEW);
}
static bool request_dirty(Request r) {
    if (r == REQUEST_SCRIPT_OPEN)
        return se_dirty(&app.script);
    if (r == REQUEST_QUIT)
        return pe_history_dirty(&app.history) || se_dirty(&app.script);
    return pe_history_dirty(&app.history);
}
static bool request_save(void) {
    if (app.request == REQUEST_SCRIPT_OPEN) {
        bool ok = se_save(&app.script);
        if (!ok)
            message(app.script.status);
        return ok;
    }
    if (app.request == REQUEST_QUIT) {
        if (pe_history_dirty(&app.history) && !save_scene())
            return false;
        if (se_dirty(&app.script) && !se_save(&app.script)) {
            message(app.script.status);
            return false;
        }
        return true;
    }
    return save_scene();
}
static void undo_active(bool redo) {
    if (scripts_active()) {
        bool ok = redo ? pe_text_redo(&app.script.document) : pe_text_undo(&app.script.document);
        if (ok)
            se_changed(&app.script, app.viewport, false);
    } else {
        if (redo)
            pe_redo(&app.history, &app.scene);
        else
            pe_undo(&app.history, &app.scene);
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
    app.scene = *next;
    free(next);
    snprintf(app.scene_path, sizeof app.scene_path, "%s", path);
    pe_history_free(&app.history);
    app.layer = 0;
    app.selected = 0;
    app.camera = (PeCamera){-16, -16, 2};
    assets_sync();
    tab_select(false);
    message("Scene opened");
}
static void request(Request type, const char *path) {
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
        if (se_open(&app.script, app.open_path))
            tab_select(true);
        else
            message(app.script.status);
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
        if (se_open(&app.script, app.open_path))
            tab_select(true);
        else
            message(app.script.status);
    }
}
static bool set_tileset(const char *relative, int tile_size) {
    PeScene candidate = app.scene;
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
    app.scene = candidate;
    commit();
    pe_assets_free(&app.assets);
    app.assets = check;
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
    if (!set_tileset(relative, app.scene.tileset_tile_size)) {
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
    ui_focus(&app.ui, scripts_active() && app.script.focused ? UI_SCRIPT_DOCUMENT : 0);
    if (scripts_active() && app.script.focused)
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
            memcpy(app.scene.layers[app.layer].name, app.input, strlen(app.input) + 1);
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
        if (set_tileset(app.scene.tileset, (int)value))
            field_stop();
        return;
    }
    PeObject *o = pe_entity_get(&app.scene, pe_entity_find(&app.scene, app.selected));
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
    *x = (int)floorf(wx / app.scene.tile_size);
    *y = (int)floorf(wy / app.scene.tile_size);
}
static void paint_cell(int x, int y, bool erase) {
    if (x < 0 || y < 0 || x >= app.scene.width || y >= app.scene.height)
        return;
    if (app.collision)
        app.scene.solid[y * PE_MAP_MAX + x] = erase ? 0 : 1;
    else
        pe_tile_set(&app.scene, app.layer, x, y, erase ? 0 : (uint16_t)app.tile);
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
    if (x < 0 || y < 0 || x >= app.scene.width || y >= app.scene.height)
        return;
    uint8_t value = erase ? 0 : 1, old = app.scene.solid[y * PE_MAP_MAX + x];
    if (value == old)
        return;
    int queue[PE_MAP_MAX * PE_MAP_MAX], head = 0, tail = 0;
    queue[tail++] = y * PE_MAP_MAX + x;
    app.scene.solid[y * PE_MAP_MAX + x] = value;
    while (head < tail) {
        int p = queue[head++], px = p % PE_MAP_MAX, py = p / PE_MAP_MAX;
        int nx[] = {px - 1, px + 1, px, px}, ny[] = {py, py, py - 1, py + 1};
        for (int i = 0; i < 4; i++)
            if (nx[i] >= 0 && ny[i] >= 0 && nx[i] < app.scene.width && ny[i] < app.scene.height &&
                app.scene.solid[ny[i] * PE_MAP_MAX + nx[i]] == old) {
                app.scene.solid[ny[i] * PE_MAP_MAX + nx[i]] = value;
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
            PeObject *o = &app.scene.objects[i];
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
            pe_fill(&app.scene, app.layer, x, y, erase ? 0 : (uint16_t)app.tile);
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
            if (x1 >= app.scene.width)
                x1 = app.scene.width - 1;
            if (y1 >= app.scene.height)
                y1 = app.scene.height - 1;
            for (int py = y0; py <= y1; py++)
                for (int px = x0; px <= x1; px++)
                    paint_cell(px, py, erase);
        } else
            pe_rectangle(&app.scene, app.layer, app.start_x, app.start_y, x, y,
                         erase ? 0 : (uint16_t)app.tile);
    }
    if (app.painting || app.dragging)
        commit();
    app.painting = false;
    app.dragging = false;
}
static void play_toggle(void) {
    if (app.history.pending)
        return;
    app.playing = !app.playing;
    if (app.playing) {
        app.runtime = app.scene;
        app.accumulator = 0;
        message("Play: WASD / arrows. Stop restores edited scene.");
    } else
        message("Stopped; edited scene preserved");
}
static void draw_panel_tabs(UiTabs *tabs, UiRect bar, UiId first, const char *first_label,
                            const char *second_label, UiIcon first_icon, UiIcon second_icon) {
    float width = 112;
    for (int i = 0; i < tabs->count; i++) {
        bool is_first = tabs->items[i].id == first;
        ui_tabs_set_width(tabs, tabs->items[i].id,
                          ui_tab_content_width(is_first ? first_icon : second_icon,
                                               is_first ? first_label : second_label) +
                              4);
    }
    ui_tabs_update(&app.ui, tabs, bar, width);
    ui_clip_push(&app.ui, bar);
    ui_apply_clip(app.renderer, &app.ui);
    for (int i = 0; i < tabs->count; i++) {
        UiRect r = ui_tab_rect(tabs, bar, width, i);
        bool is_first = tabs->items[i].id == first;
        ui_draw_tab(app.renderer, (SDL_FRect){r.x, r.y, r.w, r.h},
                    is_first ? first_icon : second_icon, is_first ? first_label : second_label,
                    tabs->selected == tabs->items[i].id, true, ui_hovered(&app.ui, r));
    }
    ui_clip_pop(&app.ui);
    ui_apply_clip(app.renderer, &app.ui);
}
static void draw_scene_tree(void) {
    section(14, SIDE_TOP + 18, ICON_ENTITY, "SCENE ENTITIES");
    int count = 0;
    for (int i = 0; i < PE_ENTITIES; i++)
        if (app.scene.objects[i].alive)
            count++;
    app.scene_view.content = count ? count * 26.f : 26;
    app.scene_view.viewport = (float)(app.height - SIDE_TOP - 46 - BOTTOM - PANEL_GAP);
    ui_scroll_clamp(&app.scene_view);
    UiRect track = {LEFT - 18, SIDE_TOP + 46, 8, app.scene_view.viewport};
    ui_scrollbar(&app.ui, UI_SCENE_SCROLL, track, &app.scene_view, UI_AXIS_Y);
    ui_clip_push(&app.ui, (UiRect){PANEL_GAP, SIDE_TOP + 46, LEFT - 24, app.scene_view.viewport});
    ui_apply_clip(app.renderer, &app.ui);
    float y = SIDE_TOP + 46 - app.scene_view.offset;
    if (!count)
        text(14, y + 8, "No entities in scene.", UI_MUTED);
    for (int i = 0; i < PE_ENTITIES; i++) {
        const PeObject *o = &app.scene.objects[i];
        if (!o->alive)
            continue;
        char label[40];
        snprintf(label, sizeof label, "%s %u", o->mask & PE_PLAYER ? "Player" : "Entity",
                 o->stable_id);
        app.widget_scope = ui_id("scene_entity") ^ o->stable_id;
        if (icon_button(12, y, LEFT - 38, ICON_ENTITY, label, "Select entity",
                        app.selected == o->stable_id, !app.playing && !app.history.pending)) {
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
    float x = (float)(app.width - RIGHT + 12);
    app.info_view.content = 340;
    app.info_view.viewport = (float)(app.height - SIDE_TOP - BOTTOM - 16);
    ui_scroll_clamp(&app.info_view);
    UiRect track = {(float)(app.width - 18), SIDE_TOP + 8, 8, app.info_view.viewport};
    ui_scrollbar(&app.ui, UI_INFO_SCROLL, track, &app.info_view, UI_AXIS_Y);
    ui_clip_push(&app.ui, (UiRect){x - 4, SIDE_TOP + 8, RIGHT - 24, app.info_view.viewport});
    ui_apply_clip(app.renderer, &app.ui);
    float y = SIDE_TOP + 18 - app.info_view.offset;
    section(x, y, ICON_FILE, "SCENE INFO");
    char label[64];
    snprintf(label, sizeof label, "Map: %d x %d tiles", app.scene.width, app.scene.height);
    text(x, y + 32, label, UI_TEXT);
    snprintf(label, sizeof label, "Tile size: %d px", app.scene.tile_size);
    text(x, y + 56, label, UI_TEXT);
    snprintf(label, sizeof label, "Layers: %d", app.scene.layer_count);
    text(x, y + 80, label, UI_TEXT);
    int count = 0;
    for (int i = 0; i < PE_ENTITIES; i++)
        if (app.scene.objects[i].alive)
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
    section(14, SIDE_TOP + 18, ICON_FOLDER, "PROJECT");
    if (icon_button(LEFT - 46, SIDE_TOP + 10, 32, ICON_REFRESH, "", "Refresh project files", false,
                    true))
        tree_refresh();
    app.tree_view.content = app.tree_count * 23.f;
    app.tree_view.viewport = (float)(app.height - SIDE_TOP - 46 - BOTTOM - PANEL_GAP);
    ui_scroll_clamp(&app.tree_view);
    UiRect track = {LEFT - 18, SIDE_TOP + 46, 8, app.tree_view.viewport};
    ui_scrollbar(&app.ui, UI_TREE_SCROLL, track, &app.tree_view, UI_AXIS_Y);
    app.tree_scroll = (int)(app.tree_view.offset / 23);
    UiRect tree_clip = {PANEL_GAP, SIDE_TOP + 46, LEFT - 24, app.tree_view.viewport};
    ui_clip_push(&app.ui, tree_clip);
    ui_apply_clip(app.renderer, &app.ui);
    int visible = (int)(app.tree_view.viewport / 23) + 2;
    for (int i = app.tree_scroll; i < app.tree_count && i < app.tree_scroll + visible; i++) {
        TreeRow row = app.tree[i];
        float y = SIDE_TOP + 50 + i * 23 - app.tree_view.offset;
        char label[128];
        snprintf(label, sizeof label, "%.20s", row.name);
        SDL_FRect r = {8, y, LEFT - 24, 22};
        bool hover = ui_hovered(&app.ui, (UiRect){r.x, r.y, r.w, r.h});
        if (hover)
            box(r, UI_HOVER);
        float row_x = 12 + row.depth * 12;
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
                tab_select(false);
                set_tileset(row.path, app.scene.tileset_tile_size);
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
    float x = (float)(app.width - RIGHT + 12);
    char label[128], value[64];
    bool enabled = !app.playing;
    float w = RIGHT - 38, half = (w - 8) / 2;
    UiRect panel = {(float)(app.width - RIGHT + 8), SIDE_TOP + 8, (float)(RIGHT - 24),
                    (float)(app.height - SIDE_TOP - BOTTOM - 16)};
    ui_scroll_clamp(&app.inspector_view);
    float y = SIDE_TOP + 18 - app.inspector_view.offset;
    ui_clip_push(&app.ui, panel);
    ui_apply_clip(app.renderer, &app.ui);
    disclosure(x, y, w, "Tilemap / Layers", &app.layers_open);
    y += 32;
    if (app.layers_open) {
        snprintf(label, sizeof label, "Map: %dx%d  tile: %d", app.scene.width, app.scene.height,
                 app.scene.tile_size);
        text(x, y, label, UI_TEXT);
        y += 22;
        snprintf(label, sizeof label, "PNG tile: %d (change)", app.scene.tileset_tile_size);
        if (button(x, y, w, label, false, enabled)) {
            snprintf(value, sizeof value, "%d", app.scene.tileset_tile_size);
            field_start(FIELD_TILESIZE, value);
        }
        y += 32;
        for (int l = 0; l < app.scene.layer_count; l++) {
            app.widget_scope = ui_id("layer_row") ^ (UiId)(l + 1);
            snprintf(label, sizeof label, "%d %.20s", l + 1, app.scene.layers[l].name);
            if (button(x, y, w - 50, label, l == app.layer, enabled))
                app.layer = l;
            if (icon_button(x + w - 44, y, 44,
                            app.scene.layers[l].visible ? ICON_EYE : ICON_EYE_OFF, "",
                            "Toggle layer visibility", false, enabled) &&
                begin()) {
                app.scene.layers[l].visible = !app.scene.layers[l].visible;
                commit();
            }
            y += 30;
        }
        app.widget_scope = 0;
        if (icon_button(x, y, half, ICON_PLUS, "Add layer", NULL, false,
                        enabled && app.scene.layer_count < PE_LAYERS) &&
            begin()) {
            app.layer = app.scene.layer_count++;
            memset(&app.scene.layers[app.layer], 0, sizeof(PeLayer));
            snprintf(app.scene.layers[app.layer].name, 32, "Layer %d", app.layer + 1);
            app.scene.layers[app.layer].visible = true;
            commit();
        }
        if (icon_button(x + half + 8, y, half, ICON_DELETE, "Delete", "Delete layer", false,
                        enabled && app.scene.layer_count > 1) &&
            begin()) {
            memmove(&app.scene.layers[app.layer], &app.scene.layers[app.layer + 1],
                    (size_t)(app.scene.layer_count - app.layer - 1) * sizeof(PeLayer));
            app.scene.layer_count--;
            memset(&app.scene.layers[app.scene.layer_count], 0, sizeof(PeLayer));
            if (app.layer >= app.scene.layer_count)
                app.layer--;
            commit();
        }
        y += 32;
        if (icon_button(x, y, half, ICON_EDIT, "Rename", NULL, false, enabled))
            field_start(FIELD_LAYER, app.scene.layers[app.layer].name);
        if (icon_button(x + half + 8, y, (half - 8) / 2, ICON_UP, "", "Move layer up", false,
                        enabled && app.layer > 0) &&
            begin()) {
            PeLayer temp = app.scene.layers[app.layer];
            app.scene.layers[app.layer] = app.scene.layers[app.layer - 1];
            app.scene.layers[--app.layer] = temp;
            commit();
        }
        if (icon_button(x + half + 8 + (half + 8) / 2, y, (half - 8) / 2, ICON_DOWN, "",
                        "Move layer down", false,
                        enabled && app.layer + 1 < app.scene.layer_count) &&
            begin()) {
            PeLayer temp = app.scene.layers[app.layer];
            app.scene.layers[app.layer] = app.scene.layers[app.layer + 1];
            app.scene.layers[++app.layer] = temp;
            commit();
        }
        y += 40;
    }
    disclosure(x, y, w, "Entity", &app.entity_open);
    y += 32;
    if (app.entity_open) {
        if (icon_button(x, y, half, ICON_PLUS, "New", "Create entity", false, enabled) && begin()) {
            PeEntity e = pe_entity_create(&app.scene, PE_TRANSFORM | PE_SPRITE | PE_BODY);
            PeObject *o = pe_entity_get(&app.scene, e);
            if (o) {
                o->x = 64;
                o->y = 64;
                app.selected = o->stable_id;
                app.tool = SELECT;
            } else
                message("Entity limit reached");
            commit();
        }
        PeObject *o = pe_entity_get(&app.scene, pe_entity_find(&app.scene, app.selected));
        if (icon_button(x + half + 8, y, half, ICON_DELETE, "Delete", "Delete entity", false,
                        enabled && o) &&
            begin()) {
            pe_entity_destroy(&app.scene, pe_entity_find(&app.scene, app.selected));
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
        int total = app.assets.tileset ? (app.assets.width / app.assets.source_tile) *
                                             (app.assets.height / app.assets.source_tile)
                                       : 0,
            columns = (int)(w / 37);
        if (columns < 1)
            columns = 1;
        for (int i = 0; i < total; i++) {
            int row = i / columns, col = i % columns;
            SDL_FRect dst = {x + col * 37, y + row * 37, 32, 32};
            if (dst.y + dst.h < panel.y || dst.y > panel.y + panel.h)
                continue;
            int source_columns = app.assets.width / app.assets.source_tile;
            SDL_FRect src = {(float)(i % source_columns * app.assets.source_tile),
                             (float)(i / source_columns * app.assets.source_tile),
                             (float)app.assets.source_tile, (float)app.assets.source_tile};
            SDL_RenderTexture(app.renderer, app.assets.tileset, &src, &dst);
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
    app.inspector_view.content = y + app.inspector_view.offset - SIDE_TOP + 8;
    app.inspector_view.viewport = (float)(app.height - SIDE_TOP - BOTTOM - 16);
    ui_scroll_clamp(&app.inspector_view);
    ui_clip_pop(&app.ui);
    ui_apply_clip(app.renderer, &app.ui);
    UiRect track = {(float)(app.width - 18), SIDE_TOP + 8, 8, app.inspector_view.viewport};
    ui_scrollbar(&app.ui, UI_INSPECTOR_SCROLL, track, &app.inspector_view, UI_AXIS_Y);
    ui_draw_scrollbar(app.renderer, &app.ui, UI_INSPECTOR_SCROLL, track, &app.inspector_view,
                      UI_AXIS_Y);
}
static void draw_script_inspector(void) {
    float x = (float)(app.width - RIGHT + 12), y = SIDE_TOP + 18 - app.inspector_view.offset;
    UiRect clip = {(float)(app.width - RIGHT + 8), SIDE_TOP + 8, (float)(RIGHT - 24),
                   (float)(app.height - SIDE_TOP - BOTTOM - 16)};
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
    app.inspector_view.viewport = (float)(app.height - SIDE_TOP - BOTTOM - 16);
    ui_scroll_clamp(&app.inspector_view);
    ui_clip_pop(&app.ui);
    ui_apply_clip(app.renderer, &app.ui);
    UiRect track = {(float)(app.width - 18), SIDE_TOP + 8, 8, app.inspector_view.viewport};
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
        if (button(x + 20, y + 110, 170, "Save and continue", false, true) && request_save())
            request_finish();
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
    ui_columns_fit((float)app.width, &app.left_width, &app.right_width, 192, 264, 320);
    ui_drag_value(&app.ui, UI_SPLIT_LEFT, left_split(), &app.left_width, 192,
                  app.width - app.right_width - 320, UI_AXIS_X, 1);
    ui_drag_value(&app.ui, UI_SPLIT_RIGHT, right_split(), &app.right_width, 264,
                  app.width - app.left_width - 320, UI_AXIS_X, -1);
}
static bool ui_controls_hit(float x, float y) {
    if (ui_contains(left_split(), x, y) || ui_contains(right_split(), x, y))
        return true;
    if (y >= TOP && y < app.viewport.y && x >= LEFT && x < app.width - RIGHT)
        return true;
    if (ui_scroll_max(app.left_tabs.selected == UI_SCENE_TAB ? &app.scene_view : &app.tree_view) >
            0 &&
        x >= LEFT - 20 && x < LEFT - PANEL_GAP && y >= SIDE_TOP + 46 && y < app.height - BOTTOM)
        return true;
    if (ui_scroll_max(app.right_tabs.selected == UI_INFO_TAB ? &app.info_view
                                                             : &app.inspector_view) > 0 &&
        x >= app.width - 20 && x < app.width - PANEL_GAP && y >= TOP && y < app.height - BOTTOM)
        return true;
    if (scripts_active() && se_scroll_hit(&app.script, x, y, app.viewport))
        return true;
    return false;
}
typedef struct {
    float x, y, left, right;
} ToolLayout;
static bool context_button_impl(ToolLayout *layout, UiId id, UiIcon icon, const char *hint,
                                bool active, bool enabled) {
    if (layout->x + 32 > layout->right) {
        layout->x = layout->left;
        layout->y += 32;
    }
    bool clicked = icon_button_impl(id, layout->x, layout->y, 32, icon, "", hint, active, enabled);
    layout->x += 38;
    return clicked;
}
#define context_button(layout, ...) context_button_impl(layout, (UiId)__LINE__, __VA_ARGS__)
static ToolLayout tools_layout(SDL_FRect area) {
    return (ToolLayout){area.x + 8, area.y + 6, area.x + 8, area.x + area.w - 8};
}
static void toolbar_separator(float x) {
    box((SDL_FRect){x, 16, 1, 17}, UI_BORDER);
}
static void draw_global_tools(void) {
    /* Placeholder: the application menu intentionally has no action yet. */
    (void)icon_button(14, 12, 32, ICON_MENU, "", "Application menu (coming later)", false, true);
    toolbar_separator(60);
    if (icon_button(74, 12, 32, ICON_SAVE, "",
                    scripts_active() ? "Save script (Ctrl+S)" : "Save scene (Ctrl+S)", false,
                    !app.playing && !app.history.pending))
        save();
    toolbar_separator(120);
    if (icon_button(
            134, 12, 32, ICON_UNDO, "", "Undo (Ctrl+Z)", false,
            !app.playing && !app.history.pending &&
                (scripts_active() ? app.script.document.position > 0 : app.history.cursor > 0)))
        undo_active(false);
    if (icon_button(172, 12, 32, ICON_REDO, "", "Redo (Ctrl+Shift+Z)", false,
                    !app.playing && !app.history.pending &&
                        (scripts_active() ? app.script.document.position < app.script.document.count
                                          : app.history.cursor < app.history.count)))
        undo_active(true);
    toolbar_separator(218);
}
static void draw_map_tools(SDL_FRect area) {
    ToolLayout layout = tools_layout(area);
    if (context_button(&layout, app.playing ? ICON_STOP : ICON_PLAY, "Play / Stop (F5)",
                       app.playing, !app.history.pending))
        play_toggle();
    if (context_button(&layout, ICON_IMPORT, "Import PNG tileset", false, !app.playing))
        field_start(FIELD_IMPORT, "");
    const UiIcon icons[] = {ICON_BRUSH, ICON_ERASE, ICON_RECTANGLE, ICON_FILL, ICON_SELECT};
    const char *hints[] = {"Brush (B)", "Eraser (E)", "Rectangle (R)", "Fill (F)",
                           "Select entity (V)"};
    for (int i = 0; i < 5; i++) {
        app.widget_scope = ui_id("tile_tool") ^ (UiId)i;
        if (context_button(&layout, icons[i], hints[i], app.tool == i,
                           !app.playing && !app.history.pending))
            app.tool = i;
    }
    app.widget_scope = 0;
    if (context_button(&layout, ICON_COLLISION, "Edit collision grid", app.collision, !app.playing))
        app.collision = !app.collision;
    if (context_button(&layout, ICON_GRID, "Show tile grid", app.grid, true))
        app.grid = !app.grid;
}
static void draw_script_tools(SDL_FRect area) {
    ToolLayout layout = tools_layout(area);
    if (context_button(&layout, ICON_CHECK, "Check JavaScript syntax (F7)", false, true)) {
        se_check(&app.script);
        ui_focus(&app.ui, UI_SCRIPT_DOCUMENT);
        se_focus(&app.script, app.window, true);
    }
}
static void draw_map_content(void) {
    bool modal = app.field != FIELD_NONE || app.request != REQUEST_NONE;
    /* Distinguish the workspace from empty/transparent tiles inside the map. */
    box(app.viewport, 0x383838ffu);
    SDL_Rect map_clip = {(int)app.viewport.x, (int)app.viewport.y, (int)app.viewport.w,
                         (int)app.viewport.h};
    SDL_SetRenderClipRect(app.renderer, &map_clip);
    const PeScene *visible_scene = app.playing ? &app.runtime : &app.scene;
    box((SDL_FRect){app.viewport.x - app.camera.x * app.camera.zoom,
                    app.viewport.y - app.camera.y * app.camera.zoom,
                    visible_scene->width * visible_scene->tile_size * app.camera.zoom,
                    visible_scene->height * visible_scene->tile_size * app.camera.zoom},
        UI_CANVAS);
    SDL_SetRenderClipRect(app.renderer, NULL);
    pe_render_scene(app.renderer, app.playing ? &app.runtime : &app.scene, &app.assets, &app.camera,
                    app.viewport, app.grid && !app.playing, app.collision && !app.playing);
    if (!app.playing) {
        PeObject *o = pe_entity_get(&app.scene, pe_entity_find(&app.scene, app.selected));
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
                app.viewport.x + (minx * app.scene.tile_size - app.camera.x) * app.camera.zoom,
                app.viewport.y + (miny * app.scene.tile_size - app.camera.y) * app.camera.zoom,
                (abs(x - sx) + 1) * app.scene.tile_size * app.camera.zoom,
                (abs(y - sy) + 1) * app.scene.tile_size * app.camera.zoom};
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
    se_draw(&app.script, app.renderer, &app.ui, app.viewport, app.mx, app.my, app.click);
}
static void draw(void) {
    SDL_GetWindowSize(app.window, &app.width, &app.height);
    app.ui.blocked = app.field != FIELD_NONE || app.request != REQUEST_NONE;
    layout_columns();
    draw_color(UI_CANVAS);
    SDL_RenderClear(app.renderer);
    /* Inset panels expose the dark application background as narrow gutters. */
    box((SDL_FRect){PANEL_GAP, PANEL_GAP, (float)(app.width - 2 * PANEL_GAP), TOP - 2 * PANEL_GAP},
        UI_PANEL);
    box((SDL_FRect){PANEL_GAP, SIDE_TOP, LEFT - 2 * PANEL_GAP,
                    (float)(app.height - SIDE_TOP - BOTTOM - PANEL_GAP)},
        UI_PANEL);
    box((SDL_FRect){(float)(app.width - RIGHT + PANEL_GAP), SIDE_TOP, RIGHT - 2 * PANEL_GAP,
                    (float)(app.height - SIDE_TOP - BOTTOM - PANEL_GAP)},
        UI_PANEL);
    bool modal = app.field != FIELD_NONE || app.request != REQUEST_NONE;
    bool saved_click = app.click;
    if (modal)
        app.click = false;
    app.ui.blocked = modal;
    tooltip = NULL;
    draw_global_tools();
    UiRect nav = {GLOBAL_NAV_X, 12, (float)(app.width - GLOBAL_NAV_X - 14), 25};
    for (int i = 0; i < app.workspaces.count; i++) {
        const EditorView *item = editor_view(app.workspaces.items[i].id);
        ui_tabs_set_width(&app.workspaces, item->id, 36);
        app.workspaces.items[i].enabled =
            !app.history.pending && (item->id == UI_MAP_VIEW || !app.playing);
    }
    if (ui_tabs_update(&app.ui, &app.workspaces, nav, 112))
        select_workspace(app.workspaces.selected);
    ui_clip_push(&app.ui, nav);
    ui_apply_clip(app.renderer, &app.ui);
    for (int i = 0; i < app.workspaces.count; i++) {
        const EditorView *item = editor_view(app.workspaces.items[i].id);
        UiRect r = ui_tab_rect(&app.workspaces, nav, 112, i);
        bool hover = ui_hovered(&app.ui, r);
        ui_draw_button(app.renderer, (SDL_FRect){r.x, r.y, r.w, r.h}, item->icon, "",
                       app.workspaces.selected == item->id, app.workspaces.items[i].enabled, hover);
        if (hover)
            tooltip = item->label;
    }
    ui_clip_pop(&app.ui);
    ui_apply_clip(app.renderer, &app.ui);
    const EditorView *view = editor_view(app.workspaces.selected);
    float center_width = (float)(app.width - LEFT - RIGHT - 2 * PANEL_GAP);
    int columns = (int)((center_width - 10) / 38);
    if (columns < 1)
        columns = 1;
    int rows = (view->action_count + columns - 1) / columns;
    SDL_FRect tools = {LEFT + PANEL_GAP, TOP + PANEL_GAP, center_width, rows * 32.f + 6};
    app.viewport = (SDL_FRect){tools.x, tools.y + tools.h + PANEL_GAP, tools.w,
                               app.height - BOTTOM - PANEL_GAP - tools.y - tools.h - PANEL_GAP};
    box(tools, UI_PANEL);
    ui_clip_push(&app.ui, (UiRect){tools.x, tools.y, tools.w, tools.h});
    ui_apply_clip(app.renderer, &app.ui);
    view->draw_tools(tools);
    ui_clip_pop(&app.ui);
    ui_apply_clip(app.renderer, &app.ui);
    view->draw_content();
    draw_panel_tabs(&app.left_tabs,
                    (UiRect){PANEL_GAP, TOP + PANEL_GAP, LEFT - 2 * PANEL_GAP, TABS - PANEL_GAP},
                    UI_EXPLORER_TAB, "Explorer", "Scene", ICON_FOLDER, ICON_ENTITY);
    draw_panel_tabs(&app.right_tabs,
                    (UiRect){(float)(app.width - RIGHT + PANEL_GAP), TOP + PANEL_GAP,
                             RIGHT - 2 * PANEL_GAP, TABS - PANEL_GAP},
                    UI_INSPECTOR_TAB, "Inspector", "Info", ICON_ENTITY, ICON_FILE);
    if (app.left_tabs.selected == UI_SCENE_TAB)
        draw_scene_tree();
    else
        draw_tree();
    if (app.right_tabs.selected == UI_INFO_TAB)
        draw_info();
    else if (scripts_active())
        draw_script_inspector();
    else
        draw_inspector();
    box((SDL_FRect){0, (float)(app.height - BOTTOM), (float)app.width, BOTTOM}, UI_PANEL);
    box((SDL_FRect){0, (float)(app.height - BOTTOM), (float)app.width, 1}, UI_BORDER);
    char details[128];
    if (scripts_active()) {
        int line, column;
        pe_text_location(&app.script.document, app.script.document.cursor, &line, &column);
        snprintf(details, sizeof details, "JS | Ln %d, Col %d | Undo %d / %d", line + 1, column + 1,
                 app.script.document.position, app.script.document.count);
    } else
        snprintf(details, sizeof details, "Zoom %.0f%% | Tile %d | Undo %d / %d",
                 app.camera.zoom * 100, app.tile, app.history.cursor, app.history.count);
    float status_y = (float)(app.height - 18);
    text(14, status_y, app.playing ? "PLAY" : "EDIT", UI_ACCENT);
    text(54, status_y, "|", UI_DISABLED);
    bool active_dirty = scripts_active() ? se_dirty(&app.script) : pe_history_dirty(&app.history);
    text(70, status_y,
         active_dirty                             ? "Modified"
         : scripts_active() && !app.script.exists ? "Untitled"
                                                  : "Saved",
         active_dirty ? UI_ACCENT : UI_MUTED);
    text(142, status_y, "|", UI_DISABLED);
    text(158, status_y, details, UI_MUTED);
    float message_x = 158 + ui_text_width(details) + 16;
    text(message_x, status_y, "|", UI_DISABLED);
    message_x += 16;
    /* Long messages stay in their own portion of the single-line status bar. */
    ui_clip_push(&app.ui, (UiRect){message_x, (float)(app.height - BOTTOM),
                                   fmaxf(0, app.width - message_x - 14), BOTTOM});
    ui_apply_clip(app.renderer, &app.ui);
    text(message_x, status_y, scripts_active() ? app.script.status : app.status, UI_TEXT);
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
        if (app.history.pending) {
            pe_history_cancel(&app.history, &app.scene);
            app.painting = app.dragging = false;
        } else if (app.playing)
            play_toggle();
        return;
    }
    if (k == SDLK_F5) {
        if (map_active())
            play_toggle();
        return;
    }
    if (app.playing || app.history.pending)
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
            pe_entity_destroy(&app.scene, pe_entity_find(&app.scene, app.selected));
            app.selected = 0;
            commit();
        }
    }
}
static void event(SDL_Event *e) {
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
        ui_controls_hit(app.mx, app.my))
        return;
    if (e->type == SDL_EVENT_KEY_DOWN && !e->key.repeat &&
        (e->key.key == SDLK_RETURN || e->key.key == SDLK_SPACE) && app.field == FIELD_NONE &&
        !(scripts_active() && app.script.focused))
        ui_activate(&app.ui);
    if (scripts_active() && !modal && se_event(&app.script, app.window, e, app.viewport)) {
        if (e->type == SDL_EVENT_MOUSE_BUTTON_DOWN && app.script.focused)
            ui_focus(&app.ui, UI_SCRIPT_DOCUMENT);
        return;
    }
    switch (e->type) {
    case SDL_EVENT_QUIT:
        if (app.playing)
            play_toggle();
        if (app.history.pending) {
            pe_history_cancel(&app.history, &app.scene);
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
        if (!modal && !app.history.pending) {
            if (suffix(e->drop.data, ".js"))
                request(REQUEST_SCRIPT_OPEN, e->drop.data);
            else if (map_active())
                import_png(e->drop.data);
        }
        break;
    case SDL_EVENT_WINDOW_FOCUS_LOST:
        ui_cancel(&app.ui);
        if (app.history.pending) {
            pe_history_cancel(&app.history, &app.scene);
            app.painting = app.dragging = false;
        }
        app.panning = false;
        break;
    case SDL_EVENT_MOUSE_WHEEL:
        if (!modal && !app.history.pending) {
            if (app.my < TOP)
                ui_tabs_scroll(&app.workspaces, (float)(app.width - GLOBAL_NAV_X - 14), 112,
                               -e->wheel.y * 56);
            else if (inside(app.mx, app.my, app.viewport) && map_active())
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
            else if (inside(app.mx, app.my, app.viewport) && !app.playing && map_active())
                edit_start(app.mx, app.my, false);
            else if (!app.history.pending)
                app.click = true;
        } else if (!modal && inside(app.mx, app.my, app.viewport) && map_active()) {
            if (e->button.button == SDL_BUTTON_MIDDLE)
                app.panning = true;
            else if (e->button.button == SDL_BUTTON_RIGHT && !app.playing && !app.history.pending)
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
            PeObject *o = pe_entity_get(&app.scene, pe_entity_find(&app.scene, app.selected));
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
    PeScene baseline = app.scene;
    app.tile = 4;
    app.tool = BRUSH;
    float sx = app.viewport.x + (2 * app.scene.tile_size + 2 - app.camera.x) * app.camera.zoom;
    float sy = app.viewport.y + (2 * app.scene.tile_size + 2 - app.camera.y) * app.camera.zoom;
    SDL_Event e = {0};
    e.type = SDL_EVENT_MOUSE_BUTTON_DOWN;
    e.button.button = SDL_BUTTON_LEFT;
    e.button.x = sx;
    e.button.y = sy;
    event(&e);
    e.type = SDL_EVENT_MOUSE_MOTION;
    e.motion.x = sx + app.scene.tile_size * app.camera.zoom;
    e.motion.y = sy;
    e.motion.state = SDL_BUTTON_LMASK;
    event(&e);
    e.type = SDL_EVENT_MOUSE_BUTTON_UP;
    e.button.button = SDL_BUTTON_LEFT;
    e.button.x = sx + app.scene.tile_size * app.camera.zoom;
    e.button.y = sy;
    event(&e);
    bool ok = app.history.count == 1 && pe_tile_get(&app.scene, 0, 2, 2) == 4 &&
              pe_tile_get(&app.scene, 0, 3, 2) == 4;
    e.type = SDL_EVENT_KEY_DOWN;
    e.key.key = SDLK_Z;
    e.key.mod = SDL_KMOD_CTRL;
    event(&e);
    ok = ok && !memcmp(&app.scene, &baseline, sizeof baseline);
    e.key.mod = SDL_KMOD_CTRL | SDL_KMOD_SHIFT;
    event(&e);
    ok = ok && pe_tile_get(&app.scene, 0, 3, 2) == 4;
    PeScene edited = app.scene;
    e.key.key = SDLK_F5;
    e.key.mod = 0;
    event(&e);
    pe_update(&app.runtime, 1, 0, 1.f / 60);
    ok = ok && app.playing && !memcmp(&app.scene, &edited, sizeof edited);
    event(&e);
    ok = ok && !app.playing;
    app.scene = baseline;
    pe_history_free(&app.history);
    app.tile = 1;
    if (!ok)
        fprintf(stderr, "Editor interaction smoke test failed\n");
    return ok;
}

static bool verify_script_editor(void) {
    PeText original = app.script.document;
    PeScene scene_before = app.scene;
    pe_text_init(&app.script.document, "");
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
    bool ok = app.script.show_completion;
    draw();
    ok = ok && app.script.popup.w > 0;
    e.key.key = SDLK_RETURN;
    e.key.mod = 0;
    event(&e);
    ok = ok && !strcmp(app.script.document.text, "const");
    e.key.key = SDLK_Z;
    e.key.mod = SDL_KMOD_CTRL;
    event(&e);
    ok = ok && !strcmp(app.script.document.text, "cons");
    e.key.mod = SDL_KMOD_CTRL | SDL_KMOD_SHIFT;
    event(&e);
    ok = ok && !strcmp(app.script.document.text, "const");
    e.key.key = SDLK_BACKSPACE;
    e.key.mod = 0;
    event(&e);
    draw();
    memset(&e, 0, sizeof e);
    e.type = SDL_EVENT_MOUSE_BUTTON_DOWN;
    e.button.button = SDL_BUTTON_LEFT;
    e.button.x = app.script.popup.x + 8;
    e.button.y = app.script.popup.y + 8;
    event(&e);
    ok = ok && !strcmp(app.script.document.text, "const");
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
    ok = ok && !app.script.diagnostic.valid && app.script.diagnostic.line == 1;
    request(REQUEST_QUIT, NULL);
    ok = ok && app.request == REQUEST_QUIT && app.running;
    app.request = REQUEST_NONE;
    ok = ok && !memcmp(&scene_before, &app.scene, sizeof scene_before);
    se_free(&app.script);
    app.script.document = original;
    app.script.highlighted_revision = UINT64_MAX;
    app.script.checked_revision = UINT64_MAX;
    app.script.show_completion = false;
    app.script.top_line = app.script.left_column = 0;
    se_check(&app.script);
    tab_select(false);
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
    PeScene original = app.scene;
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
        if (!app.scene.objects[i].alive)
            continue;
        smoke_pointer(SDL_EVENT_MOUSE_BUTTON_DOWN, 60, SIDE_TOP + 58);
        smoke_pointer(SDL_EVENT_MOUSE_BUTTON_UP, 60, SIDE_TOP + 58);
        ok = ok && app.selected == app.scene.objects[i].stable_id &&
             app.right_tabs.selected == UI_INSPECTOR_TAB;
        break;
    }
    app.selected = saved_selected;
    app.tool = saved_tool;
    app.left_tabs.selected = UI_EXPLORER_TAB;
    app.right_tabs.selected = UI_INSPECTOR_TAB;
    PeText saved = app.script.document;
    char lines[4096] = "";
    for (int i = 0; i < 100; i++)
        strcat(lines, "// scrollable line\n");
    pe_text_init(&app.script.document, lines);
    app.script.extent_revision = app.script.highlighted_revision = app.script.checked_revision =
        UINT64_MAX;
    app.script.top_line = 0;
    tab_select(true);
    draw();
    float scroll_x = app.viewport.x + app.viewport.w - 8, scroll_y = app.script.body.y + 8;
    smoke_pointer(SDL_EVENT_MOUSE_BUTTON_DOWN, scroll_x, scroll_y);
    smoke_pointer(SDL_EVENT_MOUSE_MOTION, scroll_x, scroll_y + 120);
    smoke_pointer(SDL_EVENT_MOUSE_BUTTON_UP, scroll_x, scroll_y + 120);
    ok = ok && app.script.top_line > 0 && !strcmp(app.script.document.text, lines) &&
         app.script.document.count == 0;
    se_free(&app.script);
    app.script.document = saved;
    app.script.extent_revision = app.script.highlighted_revision = app.script.checked_revision =
        UINT64_MAX;
    app.script.top_line = app.script.left_column = 0;
    se_check(&app.script);
    ok = ok && !memcmp(&original, &app.scene, sizeof original);
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
    PeScene original = app.scene;
    int history_cursor = app.history.cursor;
    uint64_t revision = app.script.document.revision;
    bool grid = app.grid;
    PeCamera camera = app.camera;
    tab_select(false);
    draw();
    float action_y = TOP + PANEL_GAP + 18;
    smoke_click(30, 24);
    bool menu_unchanged = map_active() && app.request == REQUEST_NONE && app.field == FIELD_NONE;
    smoke_click(GLOBAL_NAV_X + app.workspaces.items[0].width + 16, 24);
    bool ok = menu_unchanged && scripts_active();
    app.script.checked_revision = UINT64_MAX;
    smoke_click(LEFT + PANEL_GAP + 8 + 16, action_y);
    ok = ok && scripts_active() && app.script.checked_revision == revision;
    /* Global Undo/Redo must operate on the active script, never the scene. */
    PeText saved_document = app.script.document;
    pe_text_init(&app.script.document, "// toolbar test");
    pe_text_insert(&app.script.document, "// edited\n");
    se_changed(&app.script, app.viewport, false);
    draw();
    smoke_click(150, 24);
    ok = ok && app.script.document.position == 0 &&
         !strcmp(app.script.document.text, "// toolbar test");
    smoke_click(188, 24);
    ok = ok && app.script.document.position == 1 &&
         strstr(app.script.document.text, "edited") != NULL;
    pe_text_free(&app.script.document);
    app.script.document = saved_document;
    app.script.extent_revision = app.script.highlighted_revision = app.script.checked_revision =
        UINT64_MAX;
    se_check(&app.script);
    SDL_Event f5 = {0};
    f5.type = SDL_EVENT_KEY_DOWN;
    f5.key.key = SDLK_F5;
    event(&f5);
    ok = ok && !app.playing;
    smoke_click(GLOBAL_NAV_X + 16, 24);
    ok = ok && map_active();
    /* Narrow the center until the last map actions wrap to a second row. */
    app.left_width = 400;
    app.right_width = app.width - app.left_width - 320;
    draw();
    ok = ok && app.viewport.y == TOP + 2 * PANEL_GAP + 70;
    int columns = (int)((app.viewport.w - 10) / 38);
    int grid_index = editor_view(UI_MAP_VIEW)->action_count - 1;
    smoke_click(LEFT + PANEL_GAP + 8 + (grid_index % columns) * 38 + 16,
                action_y + (grid_index / columns) * 32);
    ok = ok && app.grid != grid && !memcmp(&original, &app.scene, sizeof original) &&
         app.history.cursor == history_cursor && app.script.document.revision == revision &&
         !memcmp(&camera, &app.camera, sizeof camera);
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
int main(int argc, char **argv) {
    const char *root = NULL;
    for (int i = 1; i < argc; i++) {
        if (!strcmp(argv[i], "--project") && i + 1 < argc)
            root = argv[++i];
        else if (!strcmp(argv[i], "--screenshot") && i + 1 < argc)
            screenshot_path = argv[++i];
        else if (!strcmp(argv[i], "--scripts"))
            start_scripts = true;
        else if (!strcmp(argv[i], "--smoke"))
            app.smoke = true;
        else {
            fprintf(stderr, "Usage: penguin_editor [--project path] [--smoke] [--scripts] "
                            "[--screenshot file.bmp]\n");
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
    if (!pe_project_load(root, app.scene_path, sizeof app.scene_path, error, sizeof error) ||
        !pe_scene_load(&app.scene, app.scene_path, error, sizeof error)) {
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
    app.left_width = 240;
    app.right_width = 280;
    app.layers_open = app.entity_open = app.palette_open = true;
    UiTab tabs[VIEW_COUNT];
    for (int i = 0; i < VIEW_COUNT; i++)
        tabs[i] = (UiTab){editor_views[i].id, true, 0};
    ui_tabs_init(&app.workspaces, tabs, VIEW_COUNT, UI_MAP_VIEW);
    UiTab left_tabs[] = {{UI_EXPLORER_TAB, true, 0}, {UI_SCENE_TAB, true, 0}};
    UiTab right_tabs[] = {{UI_INSPECTOR_TAB, true, 0}, {UI_INFO_TAB, true, 0}};
    ui_tabs_init(&app.left_tabs, left_tabs, 2, UI_EXPLORER_TAB);
    ui_tabs_init(&app.right_tabs, right_tabs, 2, UI_INSPECTOR_TAB);
    app.resize_cursor = SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_EW_RESIZE);
    app.width = 1280;
    app.height = 800;
    app.viewport = (SDL_FRect){LEFT, TOP + TABS, 1280 - LEFT - RIGHT, 800 - TOP - TABS - BOTTOM};
    if (!se_init(&app.script, root)) {
        fprintf(stderr, "%s\n", app.script.status);
        se_free(&app.script);
        SDL_DestroyCursor(app.resize_cursor);
        SDL_DestroyRenderer(app.renderer);
        SDL_DestroyWindow(app.window);
        SDL_Quit();
        return 1;
    }
    pe_history_init(&app.history);
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
    assets_sync();
    int exit_code = 0;
    if (app.smoke && (!verify_editor() || !verify_script_editor() || !verify_ui_layout() ||
                      !verify_workspace_tools())) {
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
                pe_update(&app.runtime, x, y, 1.f / 60);
                app.accumulator -= 1.0 / 60;
            }
        }
        draw();
        if (app.smoke && ++frames >= 3)
            app.running = false;
        SDL_Delay(1);
    }
    se_free(&app.script);
    pe_history_free(&app.history);
    pe_assets_free(&app.assets);
    SDL_DestroyCursor(app.resize_cursor);
    SDL_DestroyRenderer(app.renderer);
    SDL_DestroyWindow(app.window);
    SDL_Quit();
    return exit_code;
}
