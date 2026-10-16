#include "cJSON.h"
#include "editors/vector/document.h"
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static cJSON *numbers(const float *values, int count) {
    cJSON *a = cJSON_CreateArray();
    for (int i = 0; i < count; i++) {
        cJSON_AddItemToArray(a, cJSON_CreateNumber(values[i]));
    }
    return a;
}

char *ve_json(const VeDocument *d) {
    cJSON *j = cJSON_CreateObject();
    if (!j) {
        return NULL;
    }
    cJSON_AddNumberToObject(j, "version", 1);
    cJSON_AddNumberToObject(j, "next", d->next_id);
    cJSON *boards = cJSON_AddArrayToObject(j, "boards"),
          *shapes = cJSON_AddArrayToObject(j, "shapes");
    for (int i = 0; i < VE_BOARDS; i++) {
        const VeBoard *b = &d->boards[i];
        if (!b->used) {
            continue;
        }
        cJSON *o = cJSON_CreateObject();
        cJSON_AddItemToArray(boards, o);
        cJSON_AddNumberToObject(o, "slot", i);
        cJSON_AddStringToObject(o, "name", b->name);
        float values[] = {b->x, b->y, b->w, b->h};
        cJSON_AddItemToObject(o, "bounds", numbers(values, 4));
        cJSON_AddNumberToObject(o, "background", b->background);
    }
    for (int i = 0; i < VE_SHAPES; i++) {
        const VeShape *s = &d->shapes[i];
        if (!s->used) {
            continue;
        }
        cJSON *o = cJSON_CreateObject();
        cJSON_AddItemToArray(shapes, o);
        cJSON_AddNumberToObject(o, "slot", i);
        cJSON_AddNumberToObject(o, "id", s->id);
        cJSON_AddStringToObject(o, "name", s->name);
        cJSON_AddNumberToObject(o, "board", s->board);
        cJSON_AddNumberToObject(o, "kind", s->kind);
        float values[] = {s->x, s->y, s->w, s->h, s->stroke_width};
        cJSON_AddItemToObject(o, "bounds", numbers(values, 5));
        cJSON_AddNumberToObject(o, "fill", s->fill);
        cJSON_AddNumberToObject(o, "stroke", s->stroke);
        cJSON_AddBoolToObject(o, "filled", s->fill_enabled);
        cJSON_AddBoolToObject(o, "evenodd", s->evenodd);
        cJSON_AddBoolToObject(o, "stroked", s->stroke_enabled);
        cJSON *nodes = cJSON_AddArrayToObject(o, "nodes");
        for (int k = 0; k < s->count; k++) {
            const VeNode *n = &s->nodes[k];
            float v[] = {(float)n->command, n->p.x, n->p.y, n->c1.x, n->c1.y, n->c2.x, n->c2.y};
            cJSON_AddItemToArray(nodes, numbers(v, 7));
        }
    }
    char *text = cJSON_PrintUnformatted(j);
    cJSON_Delete(j);
    return text;
}

static bool number(const cJSON *o, const char *key, double low, double high, double *out) {
    const cJSON *v = cJSON_GetObjectItemCaseSensitive(o, key);
    if (!cJSON_IsNumber(v) || !isfinite(v->valuedouble) || v->valuedouble < low ||
        v->valuedouble > high) {
        return false;
    }
    *out = v->valuedouble;
    return true;
}

static bool integer(const cJSON *o, const char *key, double low, double high, double *out) {
    return number(o, key, low, high, out) && floor(*out) == *out;
}

static bool name(const cJSON *o, char *out) {
    const cJSON *v = cJSON_GetObjectItemCaseSensitive(o, "name");
    if (!cJSON_IsString(v) || strlen(v->valuestring) >= 64) {
        return false;
    }
    strcpy(out, v->valuestring);
    return true;
}

static bool array(const cJSON *a, float *out, int n) {
    if (!cJSON_IsArray(a) || cJSON_GetArraySize(a) != n) {
        return false;
    }
    for (int i = 0; i < n; i++) {
        const cJSON *v = cJSON_GetArrayItem(a, i);
        if (!cJSON_IsNumber(v) || !isfinite(v->valuedouble) || fabs(v->valuedouble) > 1000000) {
            return false;
        }
        out[i] = (float)v->valuedouble;
    }
    return true;
}

bool ve_from_json(VeDocument *d, const char *text, char *error, size_t size) {
    VeDocument *next = calloc(1, sizeof *next);
    const char *end = NULL;
    cJSON *j = cJSON_ParseWithOpts(text, &end, true);
    bool ok = false;
    double v;
    if (!next || !j || !integer(j, "version", 1, 1, &v) || !integer(j, "next", 1, UINT32_MAX, &v)) {
        goto done;
    }
    next->next_id = (uint32_t)v;
    cJSON *boards = cJSON_GetObjectItemCaseSensitive(j, "boards"),
          *shapes = cJSON_GetObjectItemCaseSensitive(j, "shapes");
    if (!cJSON_IsArray(boards) || !cJSON_IsArray(shapes) ||
        cJSON_GetArraySize(boards) > VE_BOARDS || cJSON_GetArraySize(shapes) > VE_SHAPES) {
        goto done;
    }
    for (cJSON *o = boards->child; o; o = o->next) {
        if (!integer(o, "slot", 0, VE_BOARDS - 1, &v)) {
            goto done;
        }
        VeBoard *b = &next->boards[(int)v];
        if (b->used || !name(o, b->name)) {
            goto done;
        }
        float values[4];
        if (!array(cJSON_GetObjectItemCaseSensitive(o, "bounds"), values, 4) ||
            !integer(o, "background", 0, UINT32_MAX, &v)) {
            goto done;
        }
        b->used = true;
        b->x = values[0];
        b->y = values[1];
        b->w = values[2];
        b->h = values[3];
        b->background = (uint32_t)v;
    }
    for (cJSON *o = shapes->child; o; o = o->next) {
        if (!integer(o, "slot", 0, VE_SHAPES - 1, &v)) {
            goto done;
        }
        VeShape *s = &next->shapes[(int)v];
        if (s->used || !name(o, s->name)) {
            goto done;
        }
        s->used = true;
        if (!integer(o, "id", 1, UINT32_MAX, &v)) {
            goto done;
        }
        s->id = (uint32_t)v;
        if (!integer(o, "board", 0, VE_BOARDS - 1, &v)) {
            goto done;
        }
        s->board = (int)v;
        if (!integer(o, "kind", 0, VE_PATH, &v)) {
            goto done;
        }
        s->kind = (int)v;
        float values[5];
        if (!array(cJSON_GetObjectItemCaseSensitive(o, "bounds"), values, 5)) {
            goto done;
        }
        s->x = values[0];
        s->y = values[1];
        s->w = values[2];
        s->h = values[3];
        s->stroke_width = values[4];
        if (!integer(o, "fill", 0, UINT32_MAX, &v)) {
            goto done;
        }
        s->fill = (uint32_t)v;
        if (!integer(o, "stroke", 0, UINT32_MAX, &v)) {
            goto done;
        }
        s->stroke = (uint32_t)v;
        cJSON *filled = cJSON_GetObjectItemCaseSensitive(o, "filled"),
              *stroked = cJSON_GetObjectItemCaseSensitive(o, "stroked");
        if (!cJSON_IsBool(filled) || !cJSON_IsBool(stroked)) {
            goto done;
        }
        cJSON *rule = cJSON_GetObjectItemCaseSensitive(o, "evenodd");
        if (rule && !cJSON_IsBool(rule)) {
            goto done;
        }
        s->evenodd = !rule || cJSON_IsTrue(rule);
        s->fill_enabled = cJSON_IsTrue(filled);
        s->stroke_enabled = cJSON_IsTrue(stroked);
        cJSON *nodes = cJSON_GetObjectItemCaseSensitive(o, "nodes");
        if (!cJSON_IsArray(nodes) || cJSON_GetArraySize(nodes) > VE_NODES) {
            goto done;
        }
        for (cJSON *n = nodes->child; n; n = n->next) {
            float p[7];
            if (!array(n, p, 7) || floorf(p[0]) != p[0]) {
                goto done;
            }
            s->nodes[s->count++] = (VeNode){(int)p[0], {p[1], p[2]}, {p[3], p[4]}, {p[5], p[6]}};
        }
    }
    if (ve_valid(next)) {
        *d = *next;
        ok = true;
    }
done:
    if (!ok) {
        snprintf(error, size, "Invalid vector metadata or capacity exceeded");
    }
    free(next);
    cJSON_Delete(j);
    return ok;
}
