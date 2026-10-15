#include "penguin/engine.h"
#include <math.h>
#include <stdlib.h>
#include <string.h>

void pe_scene_init(PeScene *s, int w, int h, int tile) {
    memset(s, 0, sizeof *s);
    s->width = w > 0 && w <= PE_MAP_MAX ? w : 16;
    s->height = h > 0 && h <= PE_MAP_MAX ? h : 16;
    s->tile_size = tile > 0 && tile <= 128 ? tile : 16;
    s->tileset_tile_size = s->tile_size;
    s->layer_count = 1;
    s->next_id = 1;
    strcpy(s->layers[0].name, "Ground");
    s->layers[0].visible = true;
    for (int i = 0; i < PE_ENTITIES; i++)
        s->objects[i].generation = 1;
}
