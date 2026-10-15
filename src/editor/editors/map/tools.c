#include "app/editor_internal.h"

static void paint_cell(int x, int y, bool erase);
static void flood_collision(int x, int y, bool erase);

void editor_tile_coords(float sx, float sy, int *x, int *y) {
    float wx, wy;
    pe_screen_to_world(&app.camera, sx - app.viewport.x, sy - app.viewport.y, &wx, &wy);
    *x = (int)floorf(wx / app.map->scene.tile_size);
    *y = (int)floorf(wy / app.map->scene.tile_size);
}

static void paint_cell(int x, int y, bool erase) {
    if (x < 0 || y < 0 || x >= app.map->scene.width || y >= app.map->scene.height) {
        return;
    }
    if (app.collision) {
        app.map->scene.solid[y * PE_MAP_MAX + x] = erase ? 0 : 1;
    } else {
        pe_tile_set(&app.map->scene, app.layer, x, y, erase ? 0 : (uint16_t)app.tile);
    }
}

void editor_paint_line(int x0, int y0, int x1, int y1, bool erase) {
    int dx = abs(x1 - x0), sx = x0 < x1 ? 1 : -1, dy = -abs(y1 - y0), sy = y0 < y1 ? 1 : -1,
        e = dx + dy;
    for (;;) {
        paint_cell(x0, y0, erase);
        if (x0 == x1 && y0 == y1) {
            break;
        }
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
    if (x < 0 || y < 0 || x >= app.map->scene.width || y >= app.map->scene.height) {
        return;
    }
    uint8_t value = erase ? 0 : 1, old = app.map->scene.solid[y * PE_MAP_MAX + x];
    if (value == old) {
        return;
    }

    int queue[PE_MAP_MAX * PE_MAP_MAX], head = 0, tail = 0;
    queue[tail++] = y * PE_MAP_MAX + x;
    app.map->scene.solid[y * PE_MAP_MAX + x] = value;
    while (head < tail) {
        int p = queue[head++], px = p % PE_MAP_MAX, py = p / PE_MAP_MAX;
        int nx[] = {px - 1, px + 1, px, px}, ny[] = {py, py, py - 1, py + 1};
        for (int i = 0; i < 4; i++) {
            if (nx[i] >= 0 && ny[i] >= 0 && nx[i] < app.map->scene.width &&
                ny[i] < app.map->scene.height &&
                app.map->scene.solid[ny[i] * PE_MAP_MAX + nx[i]] == old) {
                app.map->scene.solid[ny[i] * PE_MAP_MAX + nx[i]] = value;
                queue[tail++] = ny[i] * PE_MAP_MAX + nx[i];
            }
        }
    }
}

void editor_edit_start(float sx, float sy, bool erase) {
    int x, y;
    editor_tile_coords(sx, sy, &x, &y);
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
                if (editor_begin()) {
                    app.dragging = true;
                    app.drag_x = wx - o->x;
                    app.drag_y = wy - o->y;
                }
                break;
            }
        }
        return;
    }
    if (!editor_begin()) {
        return;
    }
    if (app.tool == FILL) {
        if (app.collision) {
            flood_collision(x, y, erase);
        } else {
            pe_fill(&app.map->scene, app.layer, x, y, erase ? 0 : (uint16_t)app.tile);
        }
        editor_commit();
        return;
    }
    app.painting = true;
    if (app.tool != RECTANGLE) {
        paint_cell(x, y, erase || app.tool == ERASE);
    }
}

void editor_edit_end(bool erase) {
    if (app.painting && app.tool == RECTANGLE) {
        int x, y;
        editor_tile_coords(app.mx, app.my, &x, &y);
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
            if (x0 < 0) {
                x0 = 0;
            }
            if (y0 < 0) {
                y0 = 0;
            }
            if (x1 >= app.map->scene.width) {
                x1 = app.map->scene.width - 1;
            }
            if (y1 >= app.map->scene.height) {
                y1 = app.map->scene.height - 1;
            }
            for (int py = y0; py <= y1; py++) {
                for (int px = x0; px <= x1; px++) {
                    paint_cell(px, py, erase);
                }
            }
        } else {
            pe_rectangle(&app.map->scene, app.layer, app.start_x, app.start_y, x, y,
                         erase ? 0 : (uint16_t)app.tile);
        }
    }
    if (app.painting || app.dragging) {
        editor_commit();
    }
    app.painting = false;
    app.dragging = false;
}
