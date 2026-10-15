#include "cJSON.h"
#include "penguin/engine.h"
#include <math.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#ifdef _WIN32
#include <windows.h>
#endif
static bool fail(char *error, size_t size, const char *fmt, ...) {
    if (error && size) {
        va_list args;
        va_start(args, fmt);
        vsnprintf(error, size, fmt, args);
        va_end(args);
    }
    return false;
}
static cJSON *read_json(const char *path, char *error, size_t size) {
    FILE *f = fopen(path, "rb");
    if (!f) {
        fail(error, size, "Cannot open %s", path);
        return NULL;
    }
    if (fseek(f, 0, SEEK_END)) {
        fclose(f);
        fail(error, size, "Cannot seek file: %s", path);
        return NULL;
    }
    long n = ftell(f);
    if (n < 0 || n > 4 * 1024 * 1024) {
        fclose(f);
        fail(error, size, "File too large or unreadable");
        return NULL;
    }
    rewind(f);
    char *text = malloc((size_t)n + 1);
    if (!text) {
        fclose(f);
        fail(error, size, "Out of memory");
        return NULL;
    }
    bool ok = fread(text, 1, (size_t)n, f) == (size_t)n;
    fclose(f);
    text[n] = 0;
    ok = ok && !memchr(text, 0, (size_t)n);
    cJSON *json = ok ? cJSON_ParseWithLengthOpts(text, (size_t)n + 1, NULL, true) : NULL;
    free(text);
    if (!json)
        fail(error, size, "Invalid JSON: %s", path);
    return json;
}
static bool integer(const cJSON *j, int min, int max, int *value) {
    if (!cJSON_IsNumber(j) || !isfinite(j->valuedouble) || j->valuedouble < min ||
        j->valuedouble > max || floor(j->valuedouble) != j->valuedouble)
        return false;
    *value = (int)j->valuedouble;
    return true;
}
static bool number(const cJSON *j, float *value) {
    if (!cJSON_IsNumber(j) || !isfinite(j->valuedouble) || fabs(j->valuedouble) > 1000000)
        return false;
    *value = (float)j->valuedouble;
    return true;
}
static bool relative_path(const char *p) {
    if (!p || !*p || strlen(p) >= PE_PATH || *p == '/' || *p == '\\' || strchr(p, ':') ||
        strchr(p, '\\'))
        return false;
    const char *part = p;
    for (const char *c = p;; c++) {
        if (*c == '/' || !*c) {
            size_t n = (size_t)(c - part);
            if (!n || (n == 2 && part[0] == '.' && part[1] == '.') || (n == 1 && part[0] == '.'))
                return false;
            part = c + 1;
            if (!*c)
                break;
        }
    }
    return true;
}
bool pe_scene_load(PeScene *scene, const char *path, char *error, size_t size) {
    cJSON *j = read_json(path, error, size);
    if (!j)
        return false;
    PeScene *s = malloc(sizeof *s);
    if (!s) {
        cJSON_Delete(j);
        return fail(error, size, "Out of memory");
    }
    int version, w, h, tile, source_size;
    bool ok =
        integer(cJSON_GetObjectItemCaseSensitive(j, "version"), 1, 1, &version) &&
        integer(cJSON_GetObjectItemCaseSensitive(j, "width"), 1, PE_MAP_MAX, &w) &&
        integer(cJSON_GetObjectItemCaseSensitive(j, "height"), 1, PE_MAP_MAX, &h) &&
        integer(cJSON_GetObjectItemCaseSensitive(j, "tile_size"), 1, 128, &tile) &&
        integer(cJSON_GetObjectItemCaseSensitive(j, "tileset_tile_size"), 1, 128, &source_size);
    if (!ok)
        goto done;
    pe_scene_init(s, w, h, tile);
    s->tileset_tile_size = source_size;
    cJSON *asset = cJSON_GetObjectItemCaseSensitive(j, "tileset");
    if (!cJSON_IsString(asset) || !relative_path(asset->valuestring)) {
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
        for (int y = 0; y < h; y++)
            for (int x = 0; x < w; x++) {
                int value;
                if (!integer(entry, 0, 65535, &value)) {
                    ok = false;
                    goto done;
                }
                s->layers[l].tiles[y * PE_MAP_MAX + x] = (uint16_t)value;
                entry = entry->next;
            }
    }
    cJSON *solid = cJSON_GetObjectItemCaseSensitive(j, "collision");
    if (!cJSON_IsArray(solid) || cJSON_GetArraySize(solid) != w * h) {
        ok = false;
        goto done;
    }
    cJSON *entry = solid->child;
    for (int y = 0; y < h; y++)
        for (int x = 0; x < w; x++) {
            int value;
            if (!integer(entry, 0, 1, &value)) {
                ok = false;
                goto done;
            }
            s->solid[y * PE_MAP_MAX + x] = (uint8_t)value;
            entry = entry->next;
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
        if (!integer(cJSON_GetObjectItemCaseSensitive(obj, "id"), 1, 1000000, &id) ||
            !integer(cJSON_GetObjectItemCaseSensitive(obj, "mask"), 0, 15, &mask) ||
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
        o->x = x;
        o->y = y;
        o->w = ow;
        o->h = oh;
        o->color = (uint32_t)color->valuedouble;
        if (s->next_id <= (uint32_t)id)
            s->next_id = (uint32_t)id + 1;
    }
    *scene = *s;
done:
    free(s);
    cJSON_Delete(j);
    return ok ? true : fail(error, size, "Invalid scene schema or unsupported version: %s", path);
}
bool pe_scene_save(const PeScene *s, const char *path, char *error, size_t size) {
    cJSON *j = cJSON_CreateObject();
    if (!j)
        return fail(error, size, "Out of memory");
    cJSON_AddNumberToObject(j, "version", 1);
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
        for (int y = 0; y < s->height; y++)
            for (int x = 0; x < s->width; x++)
                cJSON_AddItemToArray(tiles, cJSON_CreateNumber(pe_tile_get(s, l, x, y)));
    }
    cJSON *solid = cJSON_AddArrayToObject(j, "collision");
    for (int y = 0; y < s->height; y++)
        for (int x = 0; x < s->width; x++)
            cJSON_AddItemToArray(solid, cJSON_CreateNumber(s->solid[y * PE_MAP_MAX + x]));
    cJSON *objects = cJSON_AddArrayToObject(j, "entities");
    for (int i = 0; i < PE_ENTITIES; i++) {
        const PeObject *o = &s->objects[i];
        if (!o->alive)
            continue;
        cJSON *obj = cJSON_CreateObject();
        cJSON_AddItemToArray(objects, obj);
        cJSON_AddNumberToObject(obj, "id", o->stable_id);
        cJSON_AddNumberToObject(obj, "mask", o->mask);
        cJSON_AddNumberToObject(obj, "x", o->x);
        cJSON_AddNumberToObject(obj, "y", o->y);
        cJSON_AddNumberToObject(obj, "w", o->w);
        cJSON_AddNumberToObject(obj, "h", o->h);
        cJSON_AddNumberToObject(obj, "color", o->color);
    }
    char *text = cJSON_Print(j);
    cJSON_Delete(j);
    if (!text)
        return fail(error, size, "Out of memory");
    char temp[1024];
    if (snprintf(temp, sizeof temp, "%s.tmp", path) >= (int)sizeof temp) {
        free(text);
        return fail(error, size, "Path too long");
    }
    FILE *f = fopen(temp, "wb");
    if (!f) {
        free(text);
        return fail(error, size, "Cannot write %s", temp);
    }
    bool ok = fwrite(text, 1, strlen(text), f) == strlen(text);
    free(text);
    if (fclose(f))
        ok = false;
    if (ok) {
#ifdef _WIN32
        ok = MoveFileExA(temp, path, MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH) != 0;
#else
        ok = rename(temp, path) == 0;
#endif
    }
    if (!ok) {
        remove(temp);
        return fail(error, size, "Save failed; original file retained");
    }
    return true;
}
bool pe_project_load(const char *root, char *scene_path, size_t size, char *error,
                     size_t error_size) {
    char path[1024];
    if (snprintf(path, sizeof path, "%s/project.json", root) >= (int)sizeof path)
        return fail(error, error_size, "Project path too long");
    cJSON *j = read_json(path, error, error_size);
    if (!j)
        return false;
    int version;
    cJSON *scene = cJSON_GetObjectItemCaseSensitive(j, "start_scene");
    bool ok = integer(cJSON_GetObjectItemCaseSensitive(j, "version"), 1, 1, &version) &&
              cJSON_IsString(scene) && relative_path(scene->valuestring);
    if (ok)
        ok = snprintf(scene_path, size, "%s/%s", root, scene->valuestring) < (int)size;
    cJSON_Delete(j);
    return ok ? true : fail(error, error_size, "Invalid project manifest");
}
