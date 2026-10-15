#include "penguin/engine.h"
#include <math.h>
#include <stdlib.h>
#include <string.h>

static bool cell(const PeScene *s, int x, int y) {
    return x >= 0 && y >= 0 && x < s->width && y < s->height;
}
bool pe_tile_set(PeScene *s, int l, int x, int y, uint16_t tile) {
    if (l < 0 || l >= s->layer_count || !cell(s, x, y))
        return false;
    s->layers[l].tiles[y * PE_MAP_MAX + x] = tile;
    return true;
}
uint16_t pe_tile_get(const PeScene *s, int l, int x, int y) {
    return l >= 0 && l < s->layer_count && cell(s, x, y) ? s->layers[l].tiles[y * PE_MAP_MAX + x]
                                                         : 0;
}
bool pe_fill(PeScene *s, int l, int x, int y, uint16_t tile) {
    if (l < 0 || l >= s->layer_count || !cell(s, x, y))
        return false;
    uint16_t old = pe_tile_get(s, l, x, y);
    if (old == tile)
        return true;
    int queue[PE_MAP_MAX * PE_MAP_MAX], head = 0, tail = 0;
    queue[tail++] = y * PE_MAP_MAX + x;
    pe_tile_set(s, l, x, y, tile);
    while (head < tail) {
        int p = queue[head++], px = p % PE_MAP_MAX, py = p / PE_MAP_MAX;
        int nx[4] = {px - 1, px + 1, px, px}, ny[4] = {py, py, py - 1, py + 1};
        for (int i = 0; i < 4; i++)
            if (cell(s, nx[i], ny[i]) && pe_tile_get(s, l, nx[i], ny[i]) == old) {
                pe_tile_set(s, l, nx[i], ny[i], tile);
                queue[tail++] = ny[i] * PE_MAP_MAX + nx[i];
            }
    }
    return true;
}
void pe_rectangle(PeScene *s, int l, int x0, int y0, int x1, int y1, uint16_t tile) {
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
    if (x1 >= s->width)
        x1 = s->width - 1;
    if (y1 >= s->height)
        y1 = s->height - 1;
    for (int y = y0; y <= y1; y++)
        for (int x = x0; x <= x1; x++)
            pe_tile_set(s, l, x, y, tile);
}
