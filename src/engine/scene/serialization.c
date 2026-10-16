#include "penguin/engine.h"
#include "shared/json/json_io.h"
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#ifdef _WIN32
#include <windows.h>
#endif

static bool number(const cJSON *j, float *value) {
    if (!cJSON_IsNumber(j) || !isfinite(j->valuedouble) || fabs(j->valuedouble) > 1000000) {
        return false;
    }
    *value = (float)j->valuedouble;
    return true;
}

bool pe_scene_load(PeScene *scene, const char *path, char *error, size_t size) {
    cJSON *j = pe_json_read(path, error, size);
    if (!j) {
        return false;
    }

    PeScene *s = malloc(sizeof *s);
    if (!s) {
        cJSON_Delete(j);
        return pe_json_fail(error, size, "Out of memory");
    }

    int version, w, h, tile, source_size;
    bool ok = pe_json_integer(cJSON_GetObjectItemCaseSensitive(j, "version"), 1, 2, &version) &&
              pe_json_integer(cJSON_GetObjectItemCaseSensitive(j, "width"), 1, PE_MAP_MAX, &w) &&
              pe_json_integer(cJSON_GetObjectItemCaseSensitive(j, "height"), 1, PE_MAP_MAX, &h) &&
              pe_json_integer(cJSON_GetObjectItemCaseSensitive(j, "tile_size"), 1, 128, &tile) &&
              pe_json_integer(cJSON_GetObjectItemCaseSensitive(j, "tileset_tile_size"), 1, 128,
                              &source_size);
    if (!ok) {
        goto done;
    }
    pe_scene_init(s, w, h, tile);
    s->tileset_tile_size = source_size;
    cJSON *asset = cJSON_GetObjectItemCaseSensitive(j, "tileset");
    if (!cJSON_IsString(asset) || !pe_json_relative_path(asset->valuestring, PE_PATH)) {
        ok = false;
        goto done;
    }
    strcpy(s->tileset, asset->valuestring);
    cJSON *layers = cJSON_GetObjectItemCaseSensitive(j, "layers");
    int count = cJSON_GetArraySize(layers);
    if (!cJSON_IsArray(layers) || count < 1 || count > PE_LAYERS) {
        ok = false;
        goto done;
    }
    s->layer_count = count;
    for (int l = 0; l < count; l++) {
        cJSON *layer = cJSON_GetArrayItem(layers, l),
              *name = cJSON_GetObjectItemCaseSensitive(layer, "name"),
              *visible = cJSON_GetObjectItemCaseSensitive(layer, "visible"),
              *tiles = cJSON_GetObjectItemCaseSensitive(layer, "tiles");
        if (!cJSON_IsString(name) || strlen(name->valuestring) >= sizeof s->layers[l].name ||
            !cJSON_IsBool(visible) || !cJSON_IsArray(tiles) || cJSON_GetArraySize(tiles) != w * h) {
            ok = false;
            goto done;
        }
        strcpy(s->layers[l].name, name->valuestring);
        s->layers[l].visible = cJSON_IsTrue(visible);
        cJSON *entry = tiles->child;
        for (int y = 0; y < h; y++) {
            for (int x = 0; x < w; x++) {
                int value;
                if (!pe_json_integer(entry, 0, 65535, &value)) {
                    ok = false;
                    goto done;
                }
                s->layers[l].tiles[y * PE_MAP_MAX + x] = (uint16_t)value;
                entry = entry->next;
            }
        }
    }
    cJSON *solid = cJSON_GetObjectItemCaseSensitive(j, "collision");
    if (!cJSON_IsArray(solid) || cJSON_GetArraySize(solid) != w * h) {
        ok = false;
        goto done;
    }
    cJSON *entry = solid->child;
    for (int y = 0; y < h; y++) {
        for (int x = 0; x < w; x++) {
            int value;
            if (!pe_json_integer(entry, 0, 1, &value)) {
                ok = false;
                goto done;
            }
            s->solid[y * PE_MAP_MAX + x] = (uint8_t)value;
            entry = entry->next;
        }
    }
    cJSON *objects = cJSON_GetObjectItemCaseSensitive(j, "entities");
    count = cJSON_GetArraySize(objects);
    if (!cJSON_IsArray(objects) || count > PE_ENTITIES) {
        ok = false;
        goto done;
    }
    for (int i = 0; i < count; i++) {
        cJSON *obj = cJSON_GetArrayItem(objects, i);
        int id, mask;
        float x, y, ow, oh;
        if (!pe_json_integer(cJSON_GetObjectItemCaseSensitive(obj, "id"), 1, 1000000, &id) ||
            !pe_json_integer(cJSON_GetObjectItemCaseSensitive(obj, "mask"), 0, 15, &mask) ||
            pe_entity_valid(s, pe_entity_find(s, (uint32_t)id)) ||
            !number(cJSON_GetObjectItemCaseSensitive(obj, "x"), &x) ||
            !number(cJSON_GetObjectItemCaseSensitive(obj, "y"), &y) ||
            !number(cJSON_GetObjectItemCaseSensitive(obj, "w"), &ow) ||
            !number(cJSON_GetObjectItemCaseSensitive(obj, "h"), &oh) || ow <= 0 || oh <= 0 ||
            ow > 8192 || oh > 8192) {
            ok = false;
            goto done;
        }
        cJSON *color = cJSON_GetObjectItemCaseSensitive(obj, "color");
        if (!cJSON_IsNumber(color) || color->valuedouble < 0 || color->valuedouble > UINT32_MAX ||
            floor(color->valuedouble) != color->valuedouble) {
            ok = false;
            goto done;
        }
        PeObject *o = pe_entity_get(s, pe_entity_create(s, (uint32_t)mask));
        o->stable_id = (uint32_t)id;
        cJSON *name = cJSON_GetObjectItemCaseSensitive(obj, "name");
        if (name) {
            if (!cJSON_IsString(name) || !pe_entity_name_valid(name->valuestring)) {
                ok = false;
                goto done;
            }
            pe_entity_set_name(s, (PeEntity){(uint32_t)(o - s->objects), o->generation},
                               name->valuestring);
        } else {
            snprintf(o->name, sizeof o->name, "%s %u", mask & PE_PLAYER ? "Player" : "Entity", id);
        }
        o->x = x;
        o->y = y;
        o->w = ow;
        o->h = oh;
        o->color = (uint32_t)color->valuedouble;
        cJSON *parent = cJSON_GetObjectItemCaseSensitive(obj, "parent");
        int parent_id = 0;
        if (((parent || version == 2) && !pe_json_integer(parent, 0, 1000000, &parent_id)) ||
            (version == 1 && parent_id != 0)) {
            ok = false;
            goto done;
        }
        o->parent_id = (uint32_t)parent_id;
        cJSON *vx = cJSON_GetObjectItemCaseSensitive(obj, "vx");
        cJSON *vy = cJSON_GetObjectItemCaseSensitive(obj, "vy");
        if ((vx && !number(vx, &o->vx)) || (vy && !number(vy, &o->vy))) {
            ok = false;
            goto done;
        }
        if (s->next_id <= (uint32_t)id) {
            s->next_id = (uint32_t)id + 1;
        }
    }
    if (!pe_scene_hierarchy_valid(s)) {
        ok = false;
        goto done;
    }
    *scene = *s;
done:
    free(s);
    cJSON_Delete(j);
    return ok ? true
              : pe_json_fail(error, size, "Invalid scene schema or unsupported version: %s", path);
}

bool pe_scene_save(const PeScene *s, const char *path, char *error, size_t size) {
    if (!pe_scene_hierarchy_valid(s)) {
        return pe_json_fail(error, size, "Cannot save invalid scene hierarchy");
    }
    for (int i = 0; i < PE_ENTITIES; i++) {
        const PeObject *o = &s->objects[i];
        if (o->alive && !pe_entity_name_valid(o->name)) {
            return pe_json_fail(error, size, "Cannot save invalid entity name");
        }
        if (o->alive && (!isfinite(o->vx) || !isfinite(o->vy) || fabsf(o->vx) > 1000000 ||
                         fabsf(o->vy) > 1000000)) {
            return pe_json_fail(error, size, "Cannot save invalid entity velocity");
        }
    }
    cJSON *j = cJSON_CreateObject();
    if (!j) {
        return pe_json_fail(error, size, "Out of memory");
    }
    cJSON_AddNumberToObject(j, "version", 2);
    cJSON_AddNumberToObject(j, "width", s->width);
    cJSON_AddNumberToObject(j, "height", s->height);
    cJSON_AddNumberToObject(j, "tile_size", s->tile_size);
    cJSON_AddNumberToObject(j, "tileset_tile_size", s->tileset_tile_size);
    cJSON_AddStringToObject(j, "tileset", s->tileset);
    cJSON *layers = cJSON_AddArrayToObject(j, "layers");
    for (int l = 0; l < s->layer_count; l++) {
        cJSON *layer = cJSON_CreateObject();
        cJSON_AddItemToArray(layers, layer);
        cJSON_AddStringToObject(layer, "name", s->layers[l].name);
        cJSON_AddBoolToObject(layer, "visible", s->layers[l].visible);
        cJSON *tiles = cJSON_AddArrayToObject(layer, "tiles");
        for (int y = 0; y < s->height; y++) {
            for (int x = 0; x < s->width; x++) {
                cJSON_AddItemToArray(tiles, cJSON_CreateNumber(pe_tile_get(s, l, x, y)));
            }
        }
    }
    cJSON *solid = cJSON_AddArrayToObject(j, "collision");
    for (int y = 0; y < s->height; y++) {
        for (int x = 0; x < s->width; x++) {
            cJSON_AddItemToArray(solid, cJSON_CreateNumber(s->solid[y * PE_MAP_MAX + x]));
        }
    }
    cJSON *objects = cJSON_AddArrayToObject(j, "entities");
    for (int i = 0; i < PE_ENTITIES; i++) {
        const PeObject *o = &s->objects[i];
        if (!o->alive) {
            continue;
        }
        cJSON *obj = cJSON_CreateObject();
        cJSON_AddItemToArray(objects, obj);
        cJSON_AddNumberToObject(obj, "id", o->stable_id);
        cJSON_AddStringToObject(obj, "name", o->name);
        cJSON_AddNumberToObject(obj, "parent", o->parent_id);
        cJSON_AddNumberToObject(obj, "mask", o->mask);
        cJSON_AddNumberToObject(obj, "x", o->x);
        cJSON_AddNumberToObject(obj, "y", o->y);
        cJSON_AddNumberToObject(obj, "w", o->w);
        cJSON_AddNumberToObject(obj, "h", o->h);
        cJSON_AddNumberToObject(obj, "color", o->color);
        cJSON_AddNumberToObject(obj, "vx", o->vx);
        cJSON_AddNumberToObject(obj, "vy", o->vy);
    }

    char *text = cJSON_Print(j);
    cJSON_Delete(j);
    if (!text) {
        return pe_json_fail(error, size, "Out of memory");
    }

    char temp[1024];
    if (snprintf(temp, sizeof temp, "%s.tmp", path) >= (int)sizeof temp) {
        free(text);
        return pe_json_fail(error, size, "Path too long");
    }
    FILE *f = fopen(temp, "wb");
    if (!f) {
        free(text);
        return pe_json_fail(error, size, "Cannot write %s", temp);
    }

    bool ok = fwrite(text, 1, strlen(text), f) == strlen(text);
    free(text);
    if (fclose(f)) {
        ok = false;
    }
    if (ok) {
#ifdef _WIN32
        ok = MoveFileExA(temp, path, MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH) != 0;
#else
        ok = rename(temp, path) == 0;
#endif
    }
    if (!ok) {
        remove(temp);
        return pe_json_fail(error, size, "Save failed; original file retained");
    }

    return true;
}
