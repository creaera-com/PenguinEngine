#include "editors/game/map/internal.h"

static void paint_cell(int x, int y, bool erase);
static void flood_collision(int x, int y, bool erase);

void editor_tile_coords(float sx, float sy, int *x, int *y) {
    float wx, wy;
    pe_screen_to_world(&pe_map_active_instance()->view.camera, sx - app.shell.viewport.x,
                       sy - app.shell.viewport.y, &wx, &wy);
    *x = (int)floorf(wx / pe_map_active_instance()->scene.tile_size);
    *y = (int)floorf(wy / pe_map_active_instance()->scene.tile_size);
}

static void paint_cell(int x, int y, bool erase) {
    if (x < 0 || y < 0 || x >= pe_map_active_instance()->scene.width ||
        y >= pe_map_active_instance()->scene.height) {
        return;
    }
    if (pe_map_active_instance()->view.collision) {
        pe_map_active_instance()->scene.solid[y * PE_MAP_MAX + x] = erase ? 0 : 1;
    } else {
        pe_tile_set(&pe_map_active_instance()->scene, pe_map_active_instance()->view.layer, x, y,
                    erase ? 0 : (uint16_t)pe_map_active_instance()->view.tile);
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
    if (x < 0 || y < 0 || x >= pe_map_active_instance()->scene.width ||
        y >= pe_map_active_instance()->scene.height) {
        return;
    }
    uint8_t value = erase ? 0 : 1, old = pe_map_active_instance()->scene.solid[y * PE_MAP_MAX + x];
    if (value == old) {
        return;
    }

    int queue[PE_MAP_MAX * PE_MAP_MAX], head = 0, tail = 0;
    queue[tail++] = y * PE_MAP_MAX + x;
    pe_map_active_instance()->scene.solid[y * PE_MAP_MAX + x] = value;
    while (head < tail) {
        int p = queue[head++], px = p % PE_MAP_MAX, py = p / PE_MAP_MAX;
        int nx[] = {px - 1, px + 1, px, px}, ny[] = {py, py, py - 1, py + 1};
        for (int i = 0; i < 4; i++) {
            if (nx[i] >= 0 && ny[i] >= 0 && nx[i] < pe_map_active_instance()->scene.width &&
                ny[i] < pe_map_active_instance()->scene.height &&
                pe_map_active_instance()->scene.solid[ny[i] * PE_MAP_MAX + nx[i]] == old) {
                pe_map_active_instance()->scene.solid[ny[i] * PE_MAP_MAX + nx[i]] = value;
                queue[tail++] = ny[i] * PE_MAP_MAX + nx[i];
            }
        }
    }
}

void editor_edit_start(float sx, float sy, bool erase) {
    int x, y;
    editor_tile_coords(sx, sy, &x, &y);
    pe_map_active_instance()->view.start_x = pe_map_active_instance()->view.last_x = x;
    pe_map_active_instance()->view.start_y = pe_map_active_instance()->view.last_y = y;
    if (pe_map_active_instance()->view.tool == SELECT) {
        float wx, wy;
        pe_screen_to_world(&pe_map_active_instance()->view.camera, sx - app.shell.viewport.x,
                           sy - app.shell.viewport.y, &wx, &wy);
        app.shell.selected = 0;
        for (int i = PE_ENTITIES - 1; i >= 0; i--) {
            PeObject *o = &pe_map_active_instance()->scene.objects[i];
            float ox, oy;
            if (o->alive &&
                pe_entity_world_position(&pe_map_active_instance()->scene,
                                         (PeEntity){(uint32_t)i, o->generation}, &ox, &oy) &&
                wx >= ox && wy >= oy && wx < ox + o->w && wy < oy + o->h) {
                app.shell.selected = o->stable_id;
                editor_inspect_entity();
                /* Never move the camera between pressing and dragging an entity. */
                pe_map_active_instance()->reveal_selected = false;
                if (editor_begin()) {
                    pe_map_active_instance()->view.dragging = true;
                    pe_map_active_instance()->view.drag_x = wx - ox;
                    pe_map_active_instance()->view.drag_y = wy - oy;
                }
                break;
            }
        }
        return;
    }
    if (!editor_begin()) {
        return;
    }
    if (pe_map_active_instance()->view.tool == FILL) {
        if (pe_map_active_instance()->view.collision) {
            flood_collision(x, y, erase);
        } else {
            pe_fill(&pe_map_active_instance()->scene, pe_map_active_instance()->view.layer, x, y,
                    erase ? 0 : (uint16_t)pe_map_active_instance()->view.tile);
        }
        editor_commit();
        return;
    }
    pe_map_active_instance()->view.painting = true;
    if (pe_map_active_instance()->view.tool != RECTANGLE) {
        paint_cell(x, y, erase || pe_map_active_instance()->view.tool == ERASE);
    }
}

void editor_edit_end(bool erase) {
    if (pe_map_active_instance()->view.painting &&
        pe_map_active_instance()->view.tool == RECTANGLE) {
        int x, y;
        editor_tile_coords(app.shell.mx, app.shell.my, &x, &y);
        if (pe_map_active_instance()->view.collision) {
            int x0 = pe_map_active_instance()->view.start_x, x1 = x,
                y0 = pe_map_active_instance()->view.start_y, y1 = y;
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
            if (x1 >= pe_map_active_instance()->scene.width) {
                x1 = pe_map_active_instance()->scene.width - 1;
            }
            if (y1 >= pe_map_active_instance()->scene.height) {
                y1 = pe_map_active_instance()->scene.height - 1;
            }
            for (int py = y0; py <= y1; py++) {
                for (int px = x0; px <= x1; px++) {
                    paint_cell(px, py, erase);
                }
            }
        } else {
            pe_rectangle(&pe_map_active_instance()->scene, pe_map_active_instance()->view.layer,
                         pe_map_active_instance()->view.start_x,
                         pe_map_active_instance()->view.start_y, x, y,
                         erase ? 0 : (uint16_t)pe_map_active_instance()->view.tile);
        }
    }
    if (pe_map_active_instance()->view.painting || pe_map_active_instance()->view.dragging) {
        editor_commit();
    }
    pe_map_active_instance()->view.painting = false;
    pe_map_active_instance()->view.dragging = false;
}
