#include "editors/game/map/checkpoint.h"
#include <math.h>
#include <stdlib.h>
#include <string.h>

typedef struct {
    uint64_t revision, next, saved;
    int count, cursor;
} HistoryHeader;

static bool session_scene(PeSession *io, PeScene *scene) {
    if (!pe_session_block(io, scene, sizeof *scene)) {
        return false;
    }
    if (!io->reading) {
        return true;
    }
    if (scene->width < 1 || scene->width > PE_MAP_MAX || scene->height < 1 ||
        scene->height > PE_MAP_MAX || scene->layer_count < 1 || scene->layer_count > PE_LAYERS ||
        scene->tile_size < 1 || !memchr(scene->tileset, 0, sizeof scene->tileset)) {
        return false;
    }
    for (int i = 0; i < PE_LAYERS; i++) {
        unsigned char b;
        memcpy(&b, &scene->layers[i].visible, 1);
        if (b > 1 || !memchr(scene->layers[i].name, 0, sizeof scene->layers[i].name)) {
            return false;
        }
    }
    for (int i = 0; i < PE_ENTITIES; i++) {
        unsigned char b;
        memcpy(&b, &scene->objects[i].alive, 1);
        if (b > 1) {
            return false;
        }
        const PeObject *o = &scene->objects[i];
        if (b && !pe_entity_name_valid(o->name)) {
            return false;
        }
        if (!isfinite(o->x) || !isfinite(o->y) || !isfinite(o->w) || !isfinite(o->h) ||
            !isfinite(o->vx) || !isfinite(o->vy)) {
            return false;
        }
    }

    return pe_scene_hierarchy_valid(scene);
}

static bool session_view(PeSession *io, PeMapViewState *target) {
    PeMapViewState view = *target;
    if (!io->reading) {
        view.playing = view.painting = view.panning = view.dragging = false;
        view.accumulator = 0;
    }
    if (!pe_session_block(io, &view, sizeof view)) {
        return false;
    }
    const bool *flags[] = {&view.layers_open, &view.palette_open, &view.playing, &view.grid,
                           &view.collision,   &view.painting,     &view.panning, &view.dragging};
    for (size_t i = 0; i < sizeof flags / sizeof flags[0]; i++) {
        unsigned char byte;
        memcpy(&byte, flags[i], 1);
        if (byte > 1) {
            return false;
        }
    }
    if (!isfinite(view.camera.x) || !isfinite(view.camera.y) || !isfinite(view.camera.zoom) ||
        view.camera.zoom <= 0 || view.layer < 0 || view.layer >= PE_LAYERS || view.tool < BRUSH ||
        view.tool > SELECT || view.tile < 0 || view.tile > UINT16_MAX ||
        !isfinite(view.map_view.offset) || view.map_view.offset < 0 ||
        !isfinite(view.tiles_view.offset) || view.tiles_view.offset < 0 || view.playing ||
        view.painting || view.panning || view.dragging) {
        return false;
    }
    if (io->reading) {
        view.map_view.content = view.map_view.viewport = 0;
        view.tiles_view.content = view.tiles_view.viewport = 0;
        *target = view;
    }
    return true;
}

bool me_checkpoint(MapPayload *map, PeSession *io) {
    PeHistory *h = &map->history;
    HistoryHeader header = {h->revision, h->next_revision, h->saved_revision, h->count, h->cursor};
    if (!session_scene(io, &map->scene) || !session_view(io, &map->view) ||
        !pe_session_block(io, &map->collapsed_count, sizeof map->collapsed_count) ||
        map->collapsed_count < 0 || map->collapsed_count > PE_ENTITIES ||
        !pe_session_block(io, map->collapsed, (size_t)map->collapsed_count * sizeof(uint32_t)) ||
        !pe_session_block(io, &header, sizeof header) || header.count < 0 ||
        header.count > PE_HISTORY || header.cursor < 0 || header.cursor > header.count ||
        header.next <= header.revision) {
        return false;
    }
    for (int i = 0; i < map->collapsed_count; i++) {
        if (!map->collapsed[i]) {
            return false;
        }
        for (int j = 0; j < i; j++) {
            if (map->collapsed[i] == map->collapsed[j]) {
                return false;
            }
        }
    }
    if (!pe_session_block(io, map->component_ids, sizeof map->component_ids) ||
        !pe_session_block(io, map->component_closed, sizeof map->component_closed)) {
        return false;
    }
    for (int i = 0; i < PE_ENTITIES; i++) {
        if (map->component_closed[i] & ~15u) {
            return false;
        }
    }
    if (io->reading) {
        map->component_add_open = false;
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
            !session_scene(io, c->before) || !session_scene(io, c->after)) {
            return false;
        }
    }

    return true;
}
