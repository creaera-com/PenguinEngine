#include "penguin/render.h"
#include <SDL3/SDL_main.h>
#include <errno.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define LEFT 232
#define RIGHT 256
#define TOP 92
#define BOTTOM 48
#define TREE_MAX 256
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
typedef enum { REQUEST_NONE, REQUEST_QUIT, REQUEST_OPEN } Request;
typedef struct {
    char path[PE_PATH];
    char name[128];
    int depth;
    bool directory, expanded;
} TreeRow;
typedef struct {
    SDL_Window *window;
    SDL_Renderer *renderer;
    PeScene scene, runtime;
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
static const char *screenshot_path;
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
static void text(float x, float y, const char *label, uint32_t color) {
    draw_color(color);
    SDL_RenderDebugText(app.renderer, x, y, label);
}
static bool button(float x, float y, float w, const char *label, bool active, bool enabled) {
    SDL_FRect r = {x, y, w, 25};
    bool hover = inside(app.mx, app.my, r);
    box(r, !enabled ? 0x222c38ff : active ? 0x347e83ff : hover ? 0x41556dff : 0x2a3c50ff);
    text(x + 7, y + 8, label, enabled ? 0xe5edf6ff : 0x65758aff);
    return enabled && app.click && hover;
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
static bool save(void) {
    char error[256];
    if (!pe_scene_save(&app.scene, app.scene_path, error, sizeof error)) {
        message(error);
        return false;
    }
    pe_history_saved(&app.history);
    message("Scene saved");
    return true;
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
    message("Scene opened");
}
static void request(Request type, const char *path) {
    if (app.playing)
        return;
    if (path)
        snprintf(app.open_path, sizeof app.open_path, "%s", path);
    if (pe_history_dirty(&app.history)) {
        app.request = type;
        return;
    }
    if (type == REQUEST_QUIT)
        app.running = false;
    else
        load_scene(app.open_path);
}
static void request_finish(void) {
    Request r = app.request;
    app.request = REQUEST_NONE;
    if (r == REQUEST_QUIT)
        app.running = false;
    else if (r == REQUEST_OPEN)
        load_scene(app.open_path);
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
    snprintf(app.input, sizeof app.input, "%s", value);
    SDL_StartTextInput(app.window);
}
static void field_stop(void) {
    app.field = FIELD_NONE;
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
static void draw_tree(void) {
    text(14, 110, "PROJECT FILES", 0x95b1caff);
    if (button(142, 102, 75, "Refresh", false, true))
        tree_refresh();
    SDL_Rect clip = {0, 138, LEFT, app.height - 138 - BOTTOM};
    SDL_SetRenderClipRect(app.renderer, &clip);
    int visible = (app.height - 138 - BOTTOM) / 23;
    for (int i = app.tree_scroll; i < app.tree_count && i < app.tree_scroll + visible; i++) {
        TreeRow row = app.tree[i];
        float y = 142 + (i - app.tree_scroll) * 23;
        char label[128];
        snprintf(label, sizeof label, "%s %.105s", row.directory ? (row.expanded ? "v" : ">") : " ",
                 row.name);
        SDL_FRect r = {8, y, LEFT - 16, 22};
        bool hover = inside(app.mx, app.my, r);
        if (hover)
            box(r, 0x2e4358ff);
        text(12 + row.depth * 12, y + 6, label, 0xd0dbe6ff);
        if (app.click && hover && !app.playing) {
            if (row.directory) {
                tree_toggle(row.path);
                break;
            }
            if (suffix(row.path, ".png"))
                set_tileset(row.path, app.scene.tileset_tile_size);
            else if (suffix(row.path, ".json") && strcmp(row.path, "project.json")) {
                char path[1024];
                snprintf(path, sizeof path, "%s/%s", app.root, row.path);
                request(REQUEST_OPEN, path);
            }
        }
    }
    SDL_SetRenderClipRect(app.renderer, NULL);
}
static void draw_inspector(void) {
    float x = (float)(app.width - RIGHT + 12);
    char label[128], value[64];
    bool enabled = !app.playing;
    float y = 110;
    text(x, y, "TILEMAP / LAYERS", 0x95b1caff);
    y += 22;
    snprintf(label, sizeof label, "Map: %dx%d  tile: %d", app.scene.width, app.scene.height,
             app.scene.tile_size);
    text(x, y, label, 0xd0dbe6ff);
    y += 22;
    snprintf(label, sizeof label, "PNG tile: %d (change)", app.scene.tileset_tile_size);
    if (button(x, y, 228, label, false, enabled)) {
        snprintf(value, sizeof value, "%d", app.scene.tileset_tile_size);
        field_start(FIELD_TILESIZE, value);
    }
    y += 32;
    for (int l = 0; l < app.scene.layer_count; l++) {
        snprintf(label, sizeof label, "%d %.20s", l + 1, app.scene.layers[l].name);
        if (button(x, y, 178, label, l == app.layer, enabled))
            app.layer = l;
        if (button(x + 184, y, 44, app.scene.layers[l].visible ? "Eye" : "Off", false, enabled) &&
            begin()) {
            app.scene.layers[l].visible = !app.scene.layers[l].visible;
            commit();
        }
        y += 30;
    }
    if (button(x, y, 108, "Add layer", false, enabled && app.scene.layer_count < PE_LAYERS) &&
        begin()) {
        app.layer = app.scene.layer_count++;
        memset(&app.scene.layers[app.layer], 0, sizeof(PeLayer));
        snprintf(app.scene.layers[app.layer].name, 32, "Layer %d", app.layer + 1);
        app.scene.layers[app.layer].visible = true;
        commit();
    }
    if (button(x + 116, y, 112, "Delete layer", false, enabled && app.scene.layer_count > 1) &&
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
    if (button(x, y, 108, "Rename", false, enabled))
        field_start(FIELD_LAYER, app.scene.layers[app.layer].name);
    if (button(x + 116, y, 52, "Up", false, enabled && app.layer > 0) && begin()) {
        PeLayer temp = app.scene.layers[app.layer];
        app.scene.layers[app.layer] = app.scene.layers[app.layer - 1];
        app.scene.layers[--app.layer] = temp;
        commit();
    }
    if (button(x + 176, y, 52, "Down", false, enabled && app.layer + 1 < app.scene.layer_count) &&
        begin()) {
        PeLayer temp = app.scene.layers[app.layer];
        app.scene.layers[app.layer] = app.scene.layers[app.layer + 1];
        app.scene.layers[++app.layer] = temp;
        commit();
    }
    y += 40;
    text(x, y, "ENTITY INSPECTOR", 0x95b1caff);
    y += 22;
    if (button(x, y, 108, "New entity", false, enabled) && begin()) {
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
    if (button(x + 116, y, 112, "Delete", false, enabled && o) && begin()) {
        pe_entity_destroy(&app.scene, pe_entity_find(&app.scene, app.selected));
        app.selected = 0;
        commit();
        o = NULL;
    }
    y += 34;
    if (o) {
        snprintf(label, sizeof label, "Entity #%u", o->stable_id);
        text(x, y, label, 0xd0dbe6ff);
        y += 20;
        float values[] = {o->x, o->y, o->w, o->h};
        const char *names[] = {"X", "Y", "Width", "Height"};
        Field fields[] = {FIELD_X, FIELD_Y, FIELD_W, FIELD_H};
        for (int i = 0; i < 4; i++) {
            snprintf(label, sizeof label, "%s: %.2f", names[i], values[i]);
            if (button(x, y, 228, label, false, enabled)) {
                snprintf(value, sizeof value, "%.2f", values[i]);
                field_start(fields[i], value);
            }
            y += 29;
        }
        if (button(x, y, 228, o->mask & PE_PLAYER ? "Player control: ON" : "Player control: OFF",
                   false, enabled) &&
            begin()) {
            o->mask ^= PE_PLAYER;
            commit();
        }
        y += 35;
    } else {
        text(x, y, "Select an entity in the view.", 0x7c93aaff);
        y += 28;
    }
    text(x, y, "TILE PALETTE", 0x95b1caff);
    y += 22;
    int total = app.assets.tileset ? (app.assets.width / app.assets.source_tile) *
                                         (app.assets.height / app.assets.source_tile)
                                   : 0,
        columns = 6;
    for (int i = app.palette_scroll; i < total; i++) {
        int row = (i - app.palette_scroll) / columns, col = (i - app.palette_scroll) % columns;
        SDL_FRect dst = {x + col * 37, y + row * 37, 32, 32};
        if (dst.y + dst.h > app.height - BOTTOM - 4)
            break;
        int source_columns = app.assets.width / app.assets.source_tile;
        SDL_FRect src = {(float)(i % source_columns * app.assets.source_tile),
                         (float)(i / source_columns * app.assets.source_tile),
                         (float)app.assets.source_tile, (float)app.assets.source_tile};
        SDL_RenderTexture(app.renderer, app.assets.tileset, &src, &dst);
        if (app.tile == i + 1) {
            draw_color(0xffd277ff);
            SDL_RenderRect(app.renderer, &dst);
        }
        if (enabled && app.click && inside(app.mx, app.my, dst)) {
            app.tile = i + 1;
            if (app.tool == SELECT || app.tool == ERASE)
                app.tool = BRUSH;
        }
    }
}
static void draw_modal(void) {
    box((SDL_FRect){0, 0, (float)app.width, (float)app.height}, 0x101923ff);
    float x = app.width / 2.f - 310, y = app.height / 2.f - 95;
    box((SDL_FRect){x, y, 620, 190}, 0x293c51ff);
    if (app.request != REQUEST_NONE) {
        text(x + 20, y + 24, "Unsaved changes", 0xffd277ff);
        text(x + 20, y + 54, "Save before opening another scene or closing?", 0xe5edf6ff);
        if (button(x + 20, y + 110, 170, "Save and continue", false, true) && save())
            request_finish();
        if (button(x + 210, y + 110, 170, "Discard changes", false, true))
            request_finish();
        if (button(x + 400, y + 110, 170, "Cancel", false, true))
            app.request = REQUEST_NONE;
    } else {
        const char *title = app.field == FIELD_IMPORT  ? "Import PNG: enter full file path"
                            : app.field == FIELD_LAYER ? "Layer name"
                                                       : "Enter value";
        text(x + 20, y + 24, title, 0xffd277ff);
        box((SDL_FRect){x + 20, y + 54, 580, 28}, 0x122334ff);
        size_t n = strlen(app.input);
        text(x + 26, y + 64, app.input + (n > 69 ? n - 69 : 0), 0xe5edf6ff);
        text(x + 20, y + 94, "Enter: apply   Escape: cancel   Ctrl+A: clear", 0x95b1caff);
        if (button(x + 20, y + 125, 110, "Apply", false, true))
            field_apply();
        if (button(x + 145, y + 125, 110, "Cancel", false, true))
            field_stop();
    }
    text(x + 20, y + 164, app.status, 0xffc692ff);
}
static void draw(void) {
    SDL_GetWindowSize(app.window, &app.width, &app.height);
    app.viewport = (SDL_FRect){LEFT, TOP, (float)(app.width - LEFT - RIGHT),
                               (float)(app.height - TOP - BOTTOM)};
    draw_color(0x111e2bff);
    SDL_RenderClear(app.renderer);
    box((SDL_FRect){0, 0, (float)app.width, TOP}, 0x1d2d40ff);
    box((SDL_FRect){0, TOP, LEFT, (float)app.height}, 0x192939ff);
    box((SDL_FRect){(float)(app.width - RIGHT), TOP, RIGHT, (float)app.height}, 0x192939ff);
    bool modal = app.field != FIELD_NONE || app.request != REQUEST_NONE;
    bool saved_click = app.click;
    if (modal)
        app.click = false;
    text(14, 14, "PENGUIN ENGINE", 0x66d8ceff);
    char heading[160];
    snprintf(heading, sizeof heading, "%s  |  %s",
             pe_history_dirty(&app.history) ? "Modified" : "Saved", app.playing ? "PLAY" : "EDIT");
    text(180, 14, heading, 0xa8bfd3ff);
    if (button(14, 40, 72, "Save", false, !app.playing && !app.history.pending))
        save();
    if (button(94, 40, 72, "Undo", false,
               !app.playing && app.history.cursor > 0 && !app.history.pending)) {
        pe_undo(&app.history, &app.scene);
        selection_sync();
    }
    if (button(174, 40, 72, "Redo", false,
               !app.playing && app.history.cursor < app.history.count && !app.history.pending)) {
        pe_redo(&app.history, &app.scene);
        selection_sync();
    }
    if (button(254, 40, 102, app.playing ? "Stop" : "Play", app.playing, !app.history.pending))
        play_toggle();
    if (button(364, 40, 102, "Import PNG", false, !app.playing))
        field_start(FIELD_IMPORT, "");
    const char *tools[] = {"Brush", "Erase", "Rect", "Fill", "Select"};
    for (int i = 0; i < 5; i++)
        if (button(474 + i * 68, 40, 62, tools[i], app.tool == i,
                   !app.playing && !app.history.pending))
            app.tool = i;
    if (button(822, 40, 98, "Collision", app.collision, !app.playing))
        app.collision = !app.collision;
    if (button(928, 40, 65, "Grid", app.grid, true))
        app.grid = !app.grid;
    text(14, 77,
         "Drop PNG to import | B/E/R/F/V tools | Ctrl+S save | Ctrl+Z undo | F5 play | MMB pan | "
         "wheel zoom",
         0x90a7bbff);
    pe_render_scene(app.renderer, app.playing ? &app.runtime : &app.scene, &app.assets, &app.camera,
                    app.viewport, app.grid && !app.playing, app.collision && !app.playing);
    if (!app.playing) {
        PeObject *o = pe_entity_get(&app.scene, pe_entity_find(&app.scene, app.selected));
        if (o) {
            SDL_FRect rect = {app.viewport.x + (o->x - app.camera.x) * app.camera.zoom,
                              app.viewport.y + (o->y - app.camera.y) * app.camera.zoom,
                              o->w * app.camera.zoom, o->h * app.camera.zoom};
            draw_color(0xffd277ff);
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
            draw_color(0x7ae3d6ff);
            SDL_RenderRect(app.renderer, &rect);
            SDL_SetRenderClipRect(app.renderer, NULL);
        }
    }
    draw_tree();
    draw_inspector();
    box((SDL_FRect){0, (float)(app.height - BOTTOM), (float)app.width, BOTTOM}, 0x203348ff);
    text(14, (float)(app.height - 34), app.status, 0xc9d9e7ff);
    char details[128];
    snprintf(details, sizeof details, "Zoom %.0f%% | Tile %d | Undo %d / %d", app.camera.zoom * 100,
             app.tile, app.history.cursor, app.history.count);
    text(14, (float)(app.height - 18), details, 0x8eacc5ff);
    if (modal) {
        app.click = saved_click;
        draw_modal();
    }
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
        play_toggle();
        return;
    }
    if (app.playing || app.history.pending)
        return;
    if (ctrl && k == SDLK_S)
        save();
    else if (ctrl && (k == SDLK_Y || (k == SDLK_Z && shift))) {
        pe_redo(&app.history, &app.scene);
        selection_sync();
    } else if (ctrl && k == SDLK_Z) {
        pe_undo(&app.history, &app.scene);
        selection_sync();
    } else if (!ctrl) {
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
        if (!modal && !app.history.pending)
            import_png(e->drop.data);
        break;
    case SDL_EVENT_WINDOW_FOCUS_LOST:
        if (app.history.pending) {
            pe_history_cancel(&app.history, &app.scene);
            app.painting = app.dragging = false;
        }
        app.panning = false;
        break;
    case SDL_EVENT_MOUSE_WHEEL:
        if (!modal && !app.history.pending) {
            if (inside(app.mx, app.my, app.viewport))
                pe_camera_zoom(&app.camera, app.mx - app.viewport.x, app.my - app.viewport.y,
                               powf(1.15f, e->wheel.y));
            else if (app.mx < LEFT) {
                app.tree_scroll -= (int)e->wheel.y * 3;
                if (app.tree_scroll < 0)
                    app.tree_scroll = 0;
                if (app.tree_scroll >= app.tree_count)
                    app.tree_scroll = app.tree_count ? app.tree_count - 1 : 0;
            } else if (app.mx > app.width - RIGHT) {
                app.palette_scroll -= (int)e->wheel.y * 6;
                int count = app.assets.tileset ? (app.assets.width / app.assets.source_tile) *
                                                     (app.assets.height / app.assets.source_tile)
                                               : 0;
                if (app.palette_scroll < 0)
                    app.palette_scroll = 0;
                if (app.palette_scroll >= count)
                    app.palette_scroll = count > 6 ? ((count - 1) / 6) * 6 : 0;
            }
        }
        break;
    case SDL_EVENT_MOUSE_BUTTON_DOWN:
        app.mx = e->button.x;
        app.my = e->button.y;
        if (e->button.button == SDL_BUTTON_LEFT) {
            if (modal)
                app.click = true;
            else if (inside(app.mx, app.my, app.viewport) && !app.playing)
                edit_start(app.mx, app.my, false);
            else if (!app.history.pending)
                app.click = true;
        } else if (!modal && inside(app.mx, app.my, app.viewport)) {
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
int main(int argc, char **argv) {
    const char *root = "example";
    for (int i = 1; i < argc; i++) {
        if (!strcmp(argv[i], "--project") && i + 1 < argc)
            root = argv[++i];
        else if (!strcmp(argv[i], "--screenshot") && i + 1 < argc)
            screenshot_path = argv[++i];
        else if (!strcmp(argv[i], "--smoke"))
            app.smoke = true;
        else {
            fprintf(stderr,
                    "Usage: penguin_editor [--project path] [--smoke] [--screenshot file.bmp]\n");
            return 1;
        }
    }
    if (strlen(root) >= sizeof app.root) {
        fprintf(stderr, "Project path too long\n");
        return 1;
    }
    strcpy(app.root, root);
    char error[256];
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
    app.width = 1280;
    app.height = 800;
    app.viewport = (SDL_FRect){LEFT, TOP, 1280 - LEFT - RIGHT, 800 - TOP - BOTTOM};
    pe_history_init(&app.history);
    app.camera = (PeCamera){-16, -16, 1.4f};
    app.grid = true;
    app.tile = 1;
    app.running = true;
    app.last_time = SDL_GetTicksNS();
    strcpy(app.expanded[0], "assets");
    strcpy(app.expanded[1], "scenes");
    app.expanded_count = 2;
    tree_refresh();
    message("Welcome! Select a tile, paint a map, and press F5 to play.");
    assets_sync();
    int exit_code = 0;
    if (app.smoke && !verify_editor()) {
        app.running = false;
        exit_code = 1;
    }
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
    pe_history_free(&app.history);
    pe_assets_free(&app.assets);
    SDL_DestroyRenderer(app.renderer);
    SDL_DestroyWindow(app.window);
    SDL_Quit();
    return exit_code;
}
